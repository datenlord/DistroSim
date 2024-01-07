#ifndef GENERIC_INITIATOR_H__
#define GENERIC_INITIATOR_H__
#include <pybind11/functional.h>
#include <systemc.h>
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include "pybind11/pytypes.h"
#include "python_async_event.h"
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_time.h"
#include "tlm_core/tlm_2/tlm_2_interfaces/tlm_dmi.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_generic_payload.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/simple_initiator_socket.h"

using base_target_socket_type =
    tlm::tlm_base_target_socket_b<32, tlm::tlm_fw_transport_if<>,
                                  tlm::tlm_bw_transport_if<>>;
namespace py = pybind11;

struct stimuli {
  tlm::tlm_generic_payload* tran;
  bool blocking_flag;
  py::object future;
};

/// The `generic_initiator` is used for python to send TLM transactions
SC_MODULE(generic_initiator) {
 public:
  tlm_utils::simple_initiator_socket<generic_initiator> init_socket;
  sc_in<bool> resetn;
  sc_in<bool> clk;
  SC_CTOR(generic_initiator) : init_socket("init_socket") {
    init_socket.register_nb_transport_bw(
        this, &generic_initiator::nb_transport_bw_func);
    SC_THREAD(run);
  }

  python_async_event put_stimuli(
      py::object & loop, tlm::tlm_generic_payload & trans, bool blocking_flag) {
    auto detail = std::make_shared<stimuli>();
    detail->blocking_flag = blocking_flag;
    detail->tran = &trans;
    py::object future = loop.attr("create_future")();
    detail->future = future;

    // we copy it to avoid the payload being deleted by the caller
    stimuli_queue_mutex_.lock();
    stimuli_queue_.push_back(detail);
    stimuli_queue_mutex_.unlock();

    return python_async_event(future);
  }

  std::shared_ptr<stimuli> get_stimuli() {
    std::lock_guard<std::mutex> guard(stimuli_queue_mutex_);
    if (stimuli_queue_.empty()) {
      return nullptr;
    }
    auto input = stimuli_queue_.front();
    stimuli_queue_.pop_front();
    return input;
  }

  python_async_event wait_stimuli_response(py::object & loop,
                                           tlm::tlm_generic_payload & trans) {
    if (nonblocking_map_.find(&trans) != nonblocking_map_.end()) {
      // we have already sent the transaction
      auto future = loop.attr("create_future")();
      nonblocking_map_[&trans] = future;
      return python_async_event(future);
    }
    throw std::runtime_error("The transaction has not been sent");
  }

  void bind(base_target_socket_type & s) {
    init_socket.bind(s);
  }

  bool get_direct_mem_ptr(tlm::tlm_dmi & dmi_data) {
    bool has_dmi = false;
    tlm::tlm_generic_payload trans;
    dmi_data.init();
    has_dmi = init_socket->get_direct_mem_ptr(trans, dmi_data);
    return has_dmi;
  }

 private:
  void run() {
    sc_time delay = SC_ZERO_TIME;
    wait(resetn.posedge_event());
    while (true) {
      auto input = get_stimuli();
      if (input) {
        if (input->blocking_flag) {
          // try to send the transaction blockingly
          init_socket->b_transport(*input->tran, delay);
        } else {
          // else we send the transaction non-blockingly
          tlm::tlm_phase begin_req_phase = tlm::BEGIN_REQ;
          init_socket->nb_transport_fw(*input->tran, begin_req_phase, delay);
          // The target socket will call the `nb_transport_bw_func` with `tlm::END_REQ`
          nonblocking_map_.emplace(input->tran,
                                   pybind11::cast<pybind11::none>(Py_None));
        }
        input->future.attr("set_result")(input->tran->get_response_status());
      }
      wait(clk->posedge_event());
    }
  }

  tlm::tlm_sync_enum nb_transport_bw_func(tlm::tlm_generic_payload & payload,
                                          tlm::tlm_phase & phase,
                                          sc_core::sc_time & delay) {
    switch (phase) {
      case tlm::END_REQ:
        // Successfully send the transaction
        break;
      case tlm::BEGIN_RESP:
        // The target socket has finished processing the transaction
        if (nonblocking_map_.find(&payload) != nonblocking_map_.end()) {
          auto future = nonblocking_map_[&payload];
          nonblocking_map_.erase(&payload);
          future.attr("set_result")(payload.get_response_status());
        }
        break;
      default:
        assert(false);
    }
    return tlm::TLM_ACCEPTED;
  }

  std::deque<std::shared_ptr<stimuli>> stimuli_queue_;
  std::mutex stimuli_queue_mutex_;
  std::map<tlm::tlm_generic_payload*, py::object> nonblocking_map_;
};

#endif