#ifndef XDMA_TOP__H
#define XDMA_TOP__H
#include "VmkBsvTop.h"
#include "distrosim.h"
#include "generic_initiator.h"
#include "generic_target.h"
#include "soc/pci/xilinx/xdma_bridge.h"
#include "soc/pci/xilinx/xdma_signal.h"
#include "tlm-bridges/tlm2axilite-bridge.h"
#define XDMA_CHANNEL_NUM 1

/// A XDMA descriptor bypass channel
class xdma_wrapper_descriptor_bypass : public sc_module {
 public:
  tlm2xdma_desc_bypass_bridge dsc_bypass_bridge_c2h;  // send
  tlm2xdma_desc_bypass_bridge dsc_bypass_bridge_h2c;  // send
  tlm2axis_bridge<DMA_DATA_WIDTH> h2c_bridge;         // send
  axis2tlm_bridge<DMA_DATA_WIDTH> c2h_bridge;         // receive
  explicit xdma_wrapper_descriptor_bypass(const sc_module_name& name)
      : sc_module(name),
        dsc_bypass_bridge_c2h("dsc_bypass_bridge_c2h", false),
        dsc_bypass_bridge_h2c("dsc_bypass_bridge_h2c", true),
        h2c_bridge("h2c_bridge"),
        c2h_bridge("c2h_bridge") {}
};

/// A XDMA wrapper to hold the channel and user bar bridge
class xdma_wrapper : public sc_module {
 public:
  tlm2axilite_bridge<32, 32> user_bar;
  sc_vector<xdma_wrapper_descriptor_bypass> descriptor_bypass_channels;
  explicit xdma_wrapper(const sc_module_name& name)
      : sc_module(name),
        user_bar("user_bar"),
        descriptor_bypass_channels("channel", XDMA_CHANNEL_NUM) {}
};

/// A top module to connect the xdma_wrapper and user logic
class xdma_top : public sc_module, public distrosim_top {
 public:
  SC_HAS_PROCESS(xdma_top);

  // python interface
  generic_initiator py_user_bar;
  generic_initiator py_h2c_data;
  generic_target py_c2h_data;
  generic_target py_dsc_bypass_h2c;
  generic_target py_dsc_bypass_c2h;

  explicit xdma_top(const sc_module_name& name)
      : sc_module(name),
        py_user_bar("py_user_bar"),
        py_h2c_data("py_h2c_data"),
        py_c2h_data("py_c2h_data"),
        py_dsc_bypass_h2c("py_dsc_bypass_h2c"),
        py_dsc_bypass_c2h("py_dsc_bypass_c2h"),
        clock_signal_("clock", 10, SC_NS),
        slow_clock_signal_("slow_clock", 20, SC_NS),
        xdma_(new xdma_wrapper("xdma")),
        xdma_signals_("signal"),
        user_logic_(new VmkBsvTop("user_logic")) {
    // connect signal
    xdma_signals_.connect_user_logic(user_logic_);
    xdma_signals_.connect_xdma(xdma_);

    // setup clk
    for (int i = 0; i < XDMA_CHANNEL_NUM; i++) {
      xdma_->descriptor_bypass_channels[i].dsc_bypass_bridge_h2c.clk(
          slow_clock_signal_);
      xdma_->descriptor_bypass_channels[i].dsc_bypass_bridge_c2h.clk(
          slow_clock_signal_);
      xdma_->descriptor_bypass_channels[i].h2c_bridge.clk(slow_clock_signal_);
      xdma_->descriptor_bypass_channels[i].c2h_bridge.clk(slow_clock_signal_);
    }
    xdma_->user_bar.clk(slow_clock_signal_);
    user_logic_->CLK(clock_signal_);
    user_logic_->CLK_slowClock(slow_clock_signal_);

    // python initiator needs clock to drive
    py_user_bar.clk(slow_clock_signal_);
    py_h2c_data.clk(slow_clock_signal_);

    // set TLM bridge
    py_user_bar.init_socket.bind(xdma_->user_bar.tgt_socket);
    py_h2c_data.init_socket.bind(
        xdma_->descriptor_bypass_channels[0].h2c_bridge.tgt_socket);
    xdma_->descriptor_bypass_channels[0].c2h_bridge.socket.bind(
        py_c2h_data.tgt_socket);
    xdma_->descriptor_bypass_channels[0].dsc_bypass_bridge_h2c.init_socket.bind(
        py_dsc_bypass_h2c.tgt_socket);
    xdma_->descriptor_bypass_channels[0].dsc_bypass_bridge_c2h.init_socket.bind(
        py_dsc_bypass_c2h.tgt_socket);

    // set reset signal
    user_logic_->RST_N(rst_n_);
    user_logic_->RST_N_slowReset(rst_n_);
    for (int i = 0; i < XDMA_CHANNEL_NUM; i++) {
      xdma_->descriptor_bypass_channels[i].dsc_bypass_bridge_c2h.resetn(rst_n_);
      xdma_->descriptor_bypass_channels[i].dsc_bypass_bridge_h2c.resetn(rst_n_);
      xdma_->descriptor_bypass_channels[i].h2c_bridge.resetn(rst_n_);
      xdma_->descriptor_bypass_channels[i].c2h_bridge.resetn(rst_n_);
    }
    xdma_->user_bar.resetn(rst_n_);
    py_user_bar.resetn(rst_n_);
    py_h2c_data.resetn(rst_n_);
    SC_THREAD(pull_reset);
  }

  /// python interface for user_bar
  generic_initiator& get_py_user_bar() { return py_user_bar; }

  /// python interface for h2c_data
  generic_initiator& get_py_h2c_data() { return py_h2c_data; }

  /// python interface for c2h_data
  generic_target& get_py_c2h_data() { return py_c2h_data; }

  /// python interface for dsc_bypass_h2c
  generic_target& get_py_dsc_bypass_h2c() { return py_dsc_bypass_h2c; }

  /// python interface for dsc_bypass_c2h
  generic_target& get_py_dsc_bypass_c2h() { return py_dsc_bypass_c2h; }

  static void register_to_pybind11(py::module& module) {
    py::class_<xdma_top>(module, "xdma_top")
        .def("get_top", &xdma_top::get_top)
        .def_property_readonly("py_user_bar", &xdma_top::get_py_user_bar)
        .def_property_readonly("py_h2c_data", &xdma_top::get_py_h2c_data)
        .def_property_readonly("py_c2h_data", &xdma_top::get_py_c2h_data)
        .def_property_readonly("py_dsc_bypass_h2c",
                               &xdma_top::get_py_dsc_bypass_h2c)
        .def_property_readonly("py_dsc_bypass_c2h",
                               &xdma_top::get_py_dsc_bypass_c2h);
  }

  static xdma_top* get_top() {
    static xdma_top instance("xdma");
    return &instance;
  }

 private:
  void pull_reset() {
    /* Pull the reset signal.  */
    rst_n_.write(false);
    wait(50, sc_core::SC_NS);
    rst_n_.write(true);
  }

  sc_clock clock_signal_;
  sc_clock slow_clock_signal_;
  xdma_wrapper* xdma_;
  xdma_signal xdma_signals_;
  VmkBsvTop* user_logic_;
  sc_signal<bool> rst_n_;
};
#endif