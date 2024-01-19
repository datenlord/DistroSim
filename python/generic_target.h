#ifndef GENERIC_TARGET_H__
#define GENERIC_TARGET_H__
#include <tlm_core/tlm_2/tlm_generic_payload/tlm_phase.h>
#include "systemc.h"
#include "tlm_utils/simple_target_socket.h"

// `generic_target` wraps and abstract some operations of TLM `target_socket`.
//
// User can register a b_transport function for the target, and use `put_stimuli_via_mux_blockingly`
// to put a stimuli to the target through the stimuli mux. In this case, user should also register a
// stimuli mux handler. A stimuli mux define how the putting stimuli be sent.
SC_MODULE(generic_target) {
 public:
  tlm_utils::simple_target_socket<generic_target> tgt_socket;

  explicit generic_target(const ::sc_module_name& name) : tgt_socket(name) {
    tgt_socket.register_b_transport(this, &generic_target::b_transport);
  }

  // register a b_transport function for the target
  // User can use `self` to access the target instance
  void register_b_transport(
      const std::function<void(generic_target * self, tlm::tlm_generic_payload*,
                               sc_core::sc_time*)>& func) {
    has_b_transport_func_ = true;
    b_transport_func_ = func;
  }

  // register a stimuli mux handler. A stimuli mux define how the putting stimuli be sent
  void register_stimuli_mux_handler(
      const std::function<void(tlm::tlm_generic_payload*, sc_core::sc_time*)>&
          func) {
    has_stimuli_mux_handler_ = true;
    stimuli_mux_handler_ = std::move(func);
  }

  // put a stimuli to the target through the stimuli mux
  void put_stimuli_via_mux_blockingly(tlm::tlm_generic_payload & tran,
                                      sc_time & delay) {
    if (!has_stimuli_mux_handler_) {
      throw std::runtime_error("No set_transport_pipe function registered");
    }
    stimuli_mux_handler_(&tran, &delay);
  }

 private:
  void b_transport(tlm::tlm_generic_payload & trans, sc_core::sc_time & delay) {
    if (has_b_transport_func_) {
      b_transport_func_(this, &trans, &delay);
    } else {
      throw std::runtime_error("No b_transport function registered");
    }
  }

  bool has_b_transport_func_{false};
  bool has_stimuli_mux_handler_{false};

  std::function<void(generic_target*, tlm::tlm_generic_payload*,
                     sc_core::sc_time * sc_time)>
      b_transport_func_;
  std::function<void(tlm::tlm_generic_payload*, sc_core::sc_time*)>
      stimuli_mux_handler_;
};
#endif