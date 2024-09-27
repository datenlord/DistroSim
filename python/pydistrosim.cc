#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <systemc.h>
#include <thread>
#include "distrosim.h"
#include "generic_initiator.h"
#include "generic_target.h"
#include "memory.h"
#include "pybind11/cast.h"
#include "pybind11/pytypes.h"
#include "python_async_event.h"
#include "tlm_core/tlm_2/tlm_2_interfaces/tlm_dmi.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/tlm_quantumkeeper.h"
#include "xdma_top.h"

namespace py = pybind11;

/// The global top module pointer
py::object default_loop;

/// Set the time resolution and initiate the top module
/// Called by python
void init(py::object& loop) {
  sc_set_time_resolution(1, SC_PS);
  tlm_utils::tlm_quantumkeeper::set_global_quantum(
      sc_time(static_cast<double>(10000), SC_NS));
  default_loop = loop;
}

PYBIND11_MODULE(libdistrosim, module) {
  /// `memory` is used to provide memory address for TLM transaction.
  py::class_<memory>(module, "memory")
      .def(py::init<uint64_t, bool>(), py::return_value_policy::take_ownership)
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
  py::class_<tlm::tlm_generic_payload>(module, "tlm_generic_payload")
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
           &tlm::tlm_generic_payload::get_streaming_width);

  /// generic_initiator is a wrapper of the SystemC TLM initiator socket
  /// Python use `put_stimuli` to trigger a TLM transaction
  py::class_<generic_initiator>(module, "generic_initiator")
      .def(py::init<const char*>())
      .def(
          "put_stimuli",
          [](generic_initiator& initiator, tlm::tlm_generic_payload& trans,
             bool stop_flag) {
            return initiator.put_stimuli(default_loop, trans, stop_flag);
          },
          "put the input to a initiator", py::arg("trans"),
          py::arg("stop_flag"))
      .def("bind", &generic_initiator::bind)
      .def("get_direct_mem_ptr", &generic_initiator::get_direct_mem_ptr);

  /// generic_target can be used to register a callback function for TLM transaction
  py::class_<generic_target>(module, "generic_target")
      .def(py::init<const char*>())
      .def("register_b_transport", &generic_target::register_b_transport);

  /// The `tlm_dmi` is a wrapper of the SystemC TLM DMI
  py::class_<tlm::tlm_dmi>(module, "tlm_dmi").def(py::init<>());

  /// `sc_time` is a wrapper of the SystemC sc_time
  py::class_<sc_core::sc_time>(module, "sc_time")
      .def(py::init<double, sc_time_unit>())
      .def(py::init<sc_core::sc_time>());
  py::enum_<sc_time_unit>(module, "sc_time_unit")
      .value("SC_NS", sc_time_unit::SC_NS)
      .value("SC_MS", sc_time_unit::SC_MS)
      .export_values();
  module.attr("SC_ZERO_TIME") = SC_ZERO_TIME;

  /// enum `tlm_response_status` is a wrapper of the SystemC TLM response status
  py::enum_<tlm::tlm_response_status>(module, "tlm_response_status")
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

  /// `init`,`get_top`,`start` and `stop` are helpers functions.
  module.def("init", &init);
  module.def("start", []() { new std::thread([]() { sc_start(); }); });
  module.def("stop", []() { sc_stop(); });

  /// python use `python_async_event` to wait for the TLM transaction to finish
  py::class_<python_async_event>(module, "python_async_event")
      .def("__await__", &python_async_event::await);

  xdma_top::register_to_pybind11(module);
}
extern "C" {

int sc_main(int /*argc*/, char* /*argv*/[]) {
  return 0;
}
}
