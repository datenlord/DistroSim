#ifndef GENERIC_INITIATOR_H__
#define GENERIC_INITIATOR_H__
#include <sysc/kernel/sc_dynamic_processes.h>
#include <sysc/kernel/sc_spawn.h>
#include <systemc.h>
#include "sysc/kernel/sc_event.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_time.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_generic_payload.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/simple_initiator_socket.h"

using base_target_socket_type =
    tlm::tlm_base_target_socket_b<32, tlm::tlm_fw_transport_if<>,
                                  tlm::tlm_bw_transport_if<>>;

// `generic_initiator` wraps and abstract some operations of TLM `initiator_socket`.
//
// User can use `create_sc_thread` to create a sc_thread for the initiator, and use `put_stimuli_blockingly`
// to put a stimuli to the initiator.
SC_MODULE(generic_initiator) {
 public:
  tlm_utils::simple_initiator_socket<generic_initiator> init_socket;
  sc_in<bool> resetn;
  sc_in<bool> clk;

  using SC_CURRENT_USER_MODULE = generic_initiator;

  explicit generic_initiator(const ::sc_core ::sc_module_name& name,
                             sc_clock& external_clk,
                             sc_signal<bool>& external_resetn)
      : init_socket(name) {
    clk(external_clk);
    resetn(external_resetn);
  }

  // register a sc_thread function for the initiator
  // Addtional sensitive events can be added to the `events` vector if needed.
  void create_sc_thread(
      const char* thread_name,
      const std::function<void(generic_initiator*)>& sc_thread_func,
      std::vector<sc_event*>& events) {
    sc_spawn_options options;
    for (auto* event : events) {
      if (event) {
        options.set_sensitivity(event);
      }
    }
    sc_spawn(sc_bind(sc_thread_func, this), thread_name, &options);
  }

  // call the target's b_transport function
  void put_stimuli_blockingly(tlm::tlm_generic_payload & payload,
                              sc_time & delay) {
    init_socket->b_transport(payload, delay);
  }

  bool get_direct_ptr(tlm::tlm_dmi & dmi_data) {
    bool has_dmi = false;
    tlm::tlm_generic_payload trans;
    dmi_data.init();
    has_dmi = init_socket->get_direct_mem_ptr(trans, dmi_data);
    return has_dmi;
  }

 private:
  std::function<void(generic_initiator * initiator)> initiator_thread_func_;
  std::function<void(sc_dt::uint64 start_range, sc_dt::uint64 end_range)>
      invalidate_direct_mem_ptr_func_;
};

#endif