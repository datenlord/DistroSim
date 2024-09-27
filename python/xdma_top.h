#ifndef XDMA_TOP__H
#define XDMA_TOP__H
#include "VmkBsvTop.h"
#include "distrosim.h"
#include "soc/pci/xilinx/xdma_bridge.h"
#include "soc/pci/xilinx/xdma_signal.h"
#include "tlm-bridges/tlm2axilite-bridge.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#define XDMA_CHANNEL_NUM 1

using base_target_socket_type =
    tlm::tlm_base_target_socket_b<32, tlm::tlm_fw_transport_if<>,
                                  tlm::tlm_bw_transport_if<>>;
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

  template <typename MODULE1, typename MODULE2, typename MODULE3,
            typename MODULE4>
  void bind(
      tlm_utils::simple_target_socket<MODULE1>& h2c_descriptor_bypass_target,
      tlm_utils::simple_target_socket<MODULE2>& c2h_descriptor_bypass_target,
      tlm_utils::simple_initiator_socket<MODULE3>& h2c_data_transfer_initiator,
      tlm_utils::simple_target_socket<MODULE4>& c2h_data_transfer_target) {
    dsc_bypass_bridge_c2h.init_socket.bind(c2h_descriptor_bypass_target);
    dsc_bypass_bridge_h2c.init_socket.bind(h2c_descriptor_bypass_target);
    h2c_data_transfer_initiator.bind(h2c_bridge.tgt_socket);
    c2h_bridge.socket.bind(c2h_data_transfer_target);
  }
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
class xdma_top : public sc_module {
 public:
  SC_HAS_PROCESS(xdma_top);

  sc_clock clock_signal;
  sc_clock slow_clock_signal;
  sc_signal<bool> rst_n;
  xdma_wrapper* xdma;
  xdma_signal xdma_signals;
  VmkBsvTop* user_logic;

  explicit xdma_top(const sc_module_name& name)
      : sc_module(name),
        clock_signal("clock", 10, SC_NS),
        slow_clock_signal("slow_clock", 20, SC_NS),
        xdma(new xdma_wrapper("xdma")),
        xdma_signals("signal"),
        user_logic(new VmkBsvTop("user_logic")) {
    // connect signal
    xdma_signals.connect_user_logic(user_logic);
    xdma_signals.connect_xdma(xdma);

    // setup clk
    for (int i = 0; i < XDMA_CHANNEL_NUM; i++) {
      xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_h2c.clk(
          slow_clock_signal);
      xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_c2h.clk(
          slow_clock_signal);
      xdma->descriptor_bypass_channels[i].h2c_bridge.clk(slow_clock_signal);
      xdma->descriptor_bypass_channels[i].c2h_bridge.clk(slow_clock_signal);
    }
    xdma->user_bar.clk(slow_clock_signal);
    user_logic->CLK(clock_signal);
    user_logic->CLK_slowClock(slow_clock_signal);

    // set reset signal
    user_logic->RST_N(rst_n);
    user_logic->RST_N_slowReset(rst_n);
    for (int i = 0; i < XDMA_CHANNEL_NUM; i++) {
      xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_c2h.resetn(rst_n);
      xdma->descriptor_bypass_channels[i].dsc_bypass_bridge_h2c.resetn(rst_n);
      xdma->descriptor_bypass_channels[i].h2c_bridge.resetn(rst_n);
      xdma->descriptor_bypass_channels[i].c2h_bridge.resetn(rst_n);
    }
    xdma->user_bar.resetn(rst_n);

    SC_THREAD(pull_reset);
  }

  static xdma_top* get_top() {
    static xdma_top instance("xdma");
    return &instance;
  }

 private:
  void pull_reset() {
    /* Pull the reset signal.  */
    rst_n.write(false);
    wait(50, sc_core::SC_NS);
    rst_n.write(true);
  }
};
#endif