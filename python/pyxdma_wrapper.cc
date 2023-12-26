#include <pybind11/pybind11.h>
#include <unistd.h>
#include <cstring>
#include <deque>
#include <thread>
#include "VmkBsvTop.h"
#include "pybind11/attr.h"
#include "simple_external_initiator.h"
#include "soc/pci/xilinx/xdma_signal.h"
#include "sysc/communication/sc_clock.h"
#include "sysc/kernel/sc_event.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_simcontext.h"
#include "sysc/kernel/sc_time.h"
#include "sysc/utils/sc_vector.h"
#include "systemc.h"
#include "thread_event.h"
#include "tlm-bridges/axis2tlm-bridge.h"
#include "tlm-bridges/tlm2axis-bridge.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/tlm_quantumkeeper.h"
#define SC_INCLUDE_DYNAMIC_PROCESSES
#define XDMA_CHANNEL_NUM 1

// struct BarVisitRegister {
//   uint32_t addr;
//   uint32_t data;
//   bool is_write;
//   int status;
// };

// class xdma_wrapper_descriptor_bypass : public sc_module {
//  public:
//   tlm2xdma_desc_bypass_bridge dsc_bypass_bridge_c2h;  // send
//   tlm2xdma_desc_bypass_bridge dsc_bypass_bridge_h2c;  // send
//   tlm2axis_bridge<DMA_DATA_WIDTH> h2c_bridge;         // send
//   axis2tlm_bridge<DMA_DATA_WIDTH> c2h_bridge;         // receive
//   explicit xdma_wrapper_descriptor_bypass(const sc_module_name& name)
//       : sc_module(name),
//         dsc_bypass_bridge_c2h("dsc_bypass_bridge_c2h", false),
//         dsc_bypass_bridge_h2c("dsc_bypass_bridge_h2c", true),
//         h2c_bridge("h2c_bridge"),
//         c2h_bridge("c2h_bridge") {}
// };

// class xdma_wrapper : public sc_module {
//  public:
//   tlm2axilite_bridge<32, 32> user_bar;
//   sc_vector<xdma_wrapper_descriptor_bypass> descriptor_bypass_channels;
//   explicit xdma_wrapper(const sc_module_name& name)
//       : sc_module(name),
//         user_bar("user_bar"),
//         descriptor_bypass_channels("channel", XDMA_CHANNEL_NUM) {}
// };

// sc_event read_event;
// sc_event write_event;

// class py_wrapper : public sc_module {
//  public:
//   sc_clock clock_signal;
//   sc_clock slow_clock_signal;
//   xdma_wrapper* xdma;
//   xdma_signal xdma_signals;
//   VmkBsvTop* user_logic;
//   sc_signal<bool> rst_n;

//   // python initiator
//   tlm_utils::simple_initiator_socket<py_wrapper> py_user_bar;

//   tlm_utils::simple_initiator_socket<py_wrapper> py_h2c_data;
//   tlm_utils::simple_target_socket<py_wrapper> py_c2h_data;
//   tlm_utils::simple_target_socket<py_wrapper> py_dsc_bypass_h2c;
//   tlm_utils::simple_target_socket<py_wrapper> py_dsc_bypass_c2h;

//   BarVisitRegister bar_reg;

//   SC_HAS_PROCESS(py_wrapper);
//   explicit py_wrapper(const sc_module_name& name)
//       : sc_module(name),
//         clock_signal("clock", 10, SC_NS),
//         slow_clock_signal("slow_clock", 20, SC_NS),
//         xdma(new xdma_wrapper("xdma")),
//         xdma_signals("signal"),
//         user_logic(new VmkBsvTop("user_logic")) {
//     xdma_signals.connect_user_logic(user_logic);
//     xdma_signals.connect_xdma(xdma);

//     // setup clk
//     for (int i = 0; i < XDMA_CHANNEL_NUM; i++) {
//       xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_h2c.clk(
//           slow_clock_signal);
//       xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_c2h.clk(
//           slow_clock_signal);
//       xdma->descriptor_bypass_channels[i].h2c_bridge.clk(slow_clock_signal);
//       xdma->descriptor_bypass_channels[i].c2h_bridge.clk(slow_clock_signal);
//     }
//     xdma->user_bar.clk(slow_clock_signal);
//     user_logic->CLK(clock_signal);
//     user_logic->CLK_slowClock(slow_clock_signal);

//     // set TLM bridge
//     py_user_bar.bind(xdma->user_bar.tgt_socket);
//     py_h2c_data.bind(xdma->descriptor_bypass_channels[0].h2c_bridge.tgt_socket);
//     xdma->descriptor_bypass_channels[0].c2h_bridge.socket.bind(py_c2h_data);
//     xdma->descriptor_bypass_channels[0].dsc_bypass_bridge_h2c.init_socket.bind(
//         py_dsc_bypass_h2c);
//     xdma->descriptor_bypass_channels[0].dsc_bypass_bridge_c2h.init_socket.bind(
//         py_dsc_bypass_c2h);
//     py_dsc_bypass_h2c.register_b_transport(this, &py_wrapper::ignore);
//     py_dsc_bypass_c2h.register_b_transport(this, &py_wrapper::ignore);
//     py_c2h_data.register_b_transport(this, &py_wrapper::ignore);

//     // reset
//     user_logic->RST_N(rst_n);
//     user_logic->RST_N_slowReset(rst_n);
//     for (int i = 0; i < XDMA_CHANNEL_NUM; i++) {
//       xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_c2h.resetn(rst_n);
//       xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_h2c.resetn(rst_n);
//       xdma->descriptor_bypass_channels[i].h2c_bridge.resetn(rst_n);
//       xdma->descriptor_bypass_channels[i].c2h_bridge.resetn(rst_n);
//     }
//     xdma->user_bar.resetn(rst_n);

//     memset(&bar_reg, 0, sizeof(bar_reg));

//     SC_THREAD(pull_reset);
//     SC_METHOD(event_triggered);
//     sensitive << polling_event;
//     SC_THREAD(read_user_bar)
//     dont_initialize();
//   }
//   void ignore(tlm::tlm_generic_payload& trans, sc_time& delay) {
//     trans.set_response_status(tlm::TLM_OK_RESPONSE);
//   }
//   void pull_reset() {
//     /* Pull the reset signal.  */
//     rst_n.write(false);
//     wait(1, SC_US);
//     rst_n.write(true);
//   }

//   void read_user_bar() {
//     tlm::tlm_generic_payload trans;
//     while(true){
//     sc_time delay = SC_ZERO_TIME;
//       wait(read_event);
//       printf("bbbb\n");
//       trans.set_read();
//       trans.set_address(bar_reg.addr);
//       trans.set_data_ptr(reinterpret_cast<unsigned char*>(&bar_reg.data));
//       trans.set_streaming_width(sizeof(int));
//       trans.set_data_length(sizeof(int));
//       bar_reg.status = static_cast<int>(trans.get_response_status());
//       py_user_bar->b_transport(trans, delay);
//       bar_reg.status = static_cast<int>(trans.get_response_status());
//     }
//   }

//   void event_triggered(){
//     printf("aaaa\n");
//     if (is_read) {
//       read_event.notify();
//     } else {
//       write_event.notify();
//     }
//   }

//   thread_safe_event polling_event;
//   bool is_read = false;
// };
SC_MODULE(Top) {
 public:
  sc_clock clock_signal;
  sc_clock slow_clock_signal;
  tlm_utils::simple_target_socket<Top> target_socket;
  simple_external_socket<Top> init_socket;
  SC_HAS_PROCESS(Top);
  explicit Top(const sc_module_name& name)
      : sc_module(name),
        clock_signal("clock", 10, SC_NS),
        slow_clock_signal("slow_clock", 20, SC_NS),
        target_socket("target_socket"),
        init_socket("init_socket") {
    init_socket.bind(target_socket);
    target_socket.register_b_transport(this, &Top::init_b_transport);
  }
  void init_b_transport(tlm::tlm_generic_payload & payload, sc_time & delay) {
    printf("init_b_transport\n");
    payload.set_response_status(tlm::TLM_OK_RESPONSE);
  }
  simple_external_socket<Top>& get_init_socket() {
    return init_socket;
  }
};

Top* top = nullptr;

Top* get_top() {
  return top;
}

void start() {
  auto* thread = new std::thread([]() { sc_start(); });
  // thread->detach();
  std::cout << "call sc_start()" << std::endl;
}
inline void init() {
  sc_set_time_resolution(1, SC_PS);
  tlm_utils::tlm_quantumkeeper::set_global_quantum(
      sc_time(static_cast<double>(10000), SC_NS));
  top = new Top("top");
}

PYBIND11_MODULE(libxdma, m) {
  pybind11::class_<tlm::tlm_generic_payload>(m, "tlm_generic_payload")
      .def(pybind11::init<>())
      .def("set_read", &tlm::tlm_generic_payload::set_read)
      .def("set_write", &tlm::tlm_generic_payload::set_write)
      .def("set_address", &tlm::tlm_generic_payload::set_address)
      .def("set_data_ptr", &tlm::tlm_generic_payload::set_data_ptr)
      .def("set_streaming_width",
           &tlm::tlm_generic_payload::set_streaming_width)
      .def("set_data_length", &tlm::tlm_generic_payload::set_data_length)
      .def("get_response_status",
           &tlm::tlm_generic_payload::get_response_status)
      .def("get_data_length", &tlm::tlm_generic_payload::get_data_length)
      .def("get_data_ptr", &tlm::tlm_generic_payload::get_data_ptr)
      .def("get_address", &tlm::tlm_generic_payload::get_address)
      .def("get_streaming_width",
           &tlm::tlm_generic_payload::get_streaming_width);
  pybind11::class_<simple_external_socket<Top>>(m, "simple_external_socket")
      .def(pybind11::init<const char*>())
      .def("bind", &simple_external_socket<Top>::bind)
      .def("b_transport", &simple_external_socket<Top>::b_transport);

  pybind11::enum_<sc_time_unit>(m, "sc_time_unit")
      .value("SC_NS", sc_time_unit::SC_NS)
      .value("SC_MS", sc_time_unit::SC_MS)
      .export_values();

  pybind11::class_<sc_core::sc_time>(m, "sc_time")
      .def(pybind11::init<double, sc_time_unit>())
      .def(pybind11::init<sc_core::sc_time>());

  m.attr("SC_ZERO_TIME") = SC_ZERO_TIME;
  pybind11::class_<Top>(m, "top", pybind11::dynamic_attr())
      .def_property("init", &Top::get_init_socket, nullptr);
  m.def("init", &init);
  m.def("get_top", &get_top);
  m.def("start", &start);
}

extern "C" {

int sc_main(int argc, char* argv[]) {
  tlm::tlm_generic_payload tran;
  sc_time delay = SC_ZERO_TIME;
  init();
  start();

  sleep(1);
  auto* top = get_top();
  auto& init = top->get_init_socket();
  init.b_transport(tran, delay);
  sleep(1);

  return 0;
}
}
