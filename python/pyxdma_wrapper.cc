#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <systemc.h>
#include "memory.h"
#include "pybind11/cast.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/tlm_quantumkeeper.h"
#include "xdma_top.h"

#define SC_INCLUDE_DYNAMIC_PROCESSES

namespace py = pybind11;

/// The global top module pointer
Top* top = nullptr;

/// Set the time resolution and initiate the top module
/// Called by python
void init() {
  sc_set_time_resolution(1, SC_PS);
  tlm_utils::tlm_quantumkeeper::set_global_quantum(
      sc_time(static_cast<double>(10000), SC_NS));
  top = new Top("top");
  // auto *trace_fp = sc_create_vcd_trace_file("trace");
  // trace(trace_fp, *top, top->name());
}

/// Return the pointer to the top module
/// Called by python
Top* get_top() {
  return top;
}

PYBIND11_MODULE(libxdma, m) {
  /// `memory` is used to provide memory address for TLM transaction.
  py::class_<memory>(m, "memory")
      .def(py::init<uint64_t,bool>(), py::return_value_policy::take_ownership)
      .def("read32", &memory::read32)
      .def("write32", &memory::write32)
      .def("read64", &memory::read64)
      .def("write64", &memory::write64)
      .def("read8", &memory::read8)
      .def("write8", &memory::write8)
      .def("read16", &memory::read16)
      .def("write16", &memory::write16)
      .def("read", &memory::read)
      .def("get_raw_addr", &memory::get_raw_addr);

  /// `tlm_generic_payload` is a simple binding of the SystemC TLM generic_payload
  py::class_<tlm::tlm_generic_payload>(m, "tlm_generic_payload")
      .def(py::init<>())
      .def("set_read", &tlm::tlm_generic_payload::set_read)
      .def("set_write", &tlm::tlm_generic_payload::set_write)
      .def("set_address", &tlm::tlm_generic_payload::set_address)
      .def("set_data_ptr",
           [](tlm::tlm_generic_payload& payload, uint64_t addr) {
             auto* ptr = reinterpret_cast<unsigned char*>(addr);
             payload.set_data_ptr(ptr);
           })
      .def("set_data_ptr_with_memory",
           [](tlm::tlm_generic_payload& payload, memory& mem, uint64_t addr) {
             auto* ptr = mem.get_ptr(addr);
             payload.set_data_ptr(ptr);
           })
      .def("set_streaming_width",
           &tlm::tlm_generic_payload::set_streaming_width)
      .def("set_data_length", &tlm::tlm_generic_payload::set_data_length)
      .def("get_response_status",
           &tlm::tlm_generic_payload::get_response_status)
      .def("get_data_length", &tlm::tlm_generic_payload::get_data_length)
      .def("get_data_ptr",
           [](tlm::tlm_generic_payload& payload) {
             return reinterpret_cast<uint64_t>(payload.get_data_ptr());
           })
      .def("get_address", &tlm::tlm_generic_payload::get_address)
      .def("get_streaming_width",
           &tlm::tlm_generic_payload::get_streaming_width)
      .def("get_xdma_bypass_descriptor_extension",[](tlm::tlm_generic_payload&tran){
        xdma_bypass_descriptor_extension* ext = nullptr;
        tran.get_extension(ext);
        return ext;
      },py::return_value_policy::reference)
      .def("set_eop",[](tlm::tlm_generic_payload&tran){
        auto* genattr = new genattr_extension();
        tran.set_extension(genattr);
        genattr->set_eop(true);
      });

  py::class_<xdma_bypass_descriptor_extension>(
      m, "xdma_bypass_descriptor_extension")
      .def("get_len", &xdma_bypass_descriptor_extension::get_len);

  /// Initiator is a wrapper of the SystemC TLM initiator socket
  /// Python use `put_input` to trigger a TLM transaction
  py::class_<Initiator>(m, "Initiator")
      .def(py::init<const char*>())
      .def("put_input", &Initiator::put_input, "put the input to a initiator",
           py::arg("trans"), py::arg("stop_flag"), py::arg("callback"))
      .def("bind", &Initiator::bind);

  /// Target can be used to register a callback function for TLM transaction
  py::class_<Target>(m, "Target")
      .def(py::init<const char*>())
      .def("register_b_transport", &Target::register_b_transport);

  // py::class_<tlm_utils::simple_target_socket<Top>>(
  //     m, "simple_target_socket");

  /// `sc_time` is a wrapper of the SystemC sc_time
  py::class_<sc_core::sc_time>(m, "sc_time")
      .def(py::init<double, sc_time_unit>())
      .def(py::init<sc_core::sc_time>());
  py::enum_<sc_time_unit>(m, "sc_time_unit")
      .value("SC_NS", sc_time_unit::SC_NS)
      .value("SC_MS", sc_time_unit::SC_MS)
      .export_values();
  m.attr("SC_ZERO_TIME") = SC_ZERO_TIME;

  /// enum `tlm_response_status` is a wrapper of the SystemC TLM response status
  py::enum_<tlm::tlm_response_status>(m, "tlm_response_status")
      .value("TLM_OK_RESPONSE", tlm::tlm_response_status::TLM_OK_RESPONSE)
      .value("TLM_INCOMPLETE_RESPONSE",
             tlm::tlm_response_status::TLM_INCOMPLETE_RESPONSE)
      .value("TLM_GENERIC_ERROR_RESPONSE",
             tlm::tlm_response_status::TLM_GENERIC_ERROR_RESPONSE)
      .value("TLM_ADDRESS_ERROR_RESPONSE",
             tlm::tlm_response_status::TLM_ADDRESS_ERROR_RESPONSE)
      .value("TLM_COMMAND_ERROR_RESPONSE",
             tlm::tlm_response_status::TLM_COMMAND_ERROR_RESPONSE)
      .value("TLM_BURST_ERROR_RESPONSE",
             tlm::tlm_response_status::TLM_BURST_ERROR_RESPONSE)
      .value("TLM_BYTE_ENABLE_ERROR_RESPONSE",
             tlm::tlm_response_status::TLM_BYTE_ENABLE_ERROR_RESPONSE)
      .export_values();

  /// `Top` is the top module of the XDMA design
  /// It is used to expose the internal `Initiator` and `Target` to python
  py::class_<Top>(m, "Top", py::dynamic_attr())
      .def_property("py_user_bar", &Top::get_py_user_bar, nullptr)
      .def_property("py_h2c_data", &Top::get_py_h2c_data, nullptr)
      .def_property("py_c2h_data", &Top::get_py_c2h_data, nullptr)
      .def_property("py_dsc_bypass_h2c", &Top::get_py_dsc_bypass_h2c, nullptr)
      .def_property("py_dsc_bypass_c2h", &Top::get_py_dsc_bypass_c2h, nullptr);

  /// `init`,`get_top`,`start` and `stop` are helpers functions.
  m.def("init", &init);
  m.def("get_top", &get_top);
  m.def("start", []() { sc_start(); });
  m.def("stop", []() { sc_stop(); });
}

extern "C" {

int sc_main(int argc, char* argv[]) {
  return 0;
}
}
