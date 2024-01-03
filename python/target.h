#ifndef TARGET_H__
#define TARGET_H__
#include <utility>
#include "systemc.h"
#include "tlm_utils/simple_target_socket.h"
#include "pybind11/functional.h"

SC_MODULE(Target) {
 public:
  tlm_utils::simple_target_socket<Target> tgt_socket;
  SC_CTOR(Target) : tgt_socket("tgt_socket") {
    tgt_socket.register_b_transport(this, &Target::b_transport);
  }
  void register_b_transport(std::function<void(tlm::tlm_generic_payload*)> func) {
    has_func_ = true;
    func_ = std::move(func);
  }
 private:
  void b_transport(tlm::tlm_generic_payload & trans, sc_core::sc_time & delay) {
    if (has_func_) {
      func_(&trans);
    }
    trans.set_response_status(tlm::TLM_OK_RESPONSE);
  }

  bool has_func_{false};
  std::function<void(tlm::tlm_generic_payload*)> func_;
};
#endif