#ifndef INITIATOR_H__
#define INITIATOR_H__
#include <pybind11/functional.h>
#include <sysc/communication/sc_mutex.h>
#include <systemc.h>
#include <algorithm>
#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_time.h"
#include "thread_event.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_generic_payload.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/simple_initiator_socket.h"

using base_target_socket_type =
    tlm::tlm_base_target_socket_b<32, tlm::tlm_fw_transport_if<>,
                                  tlm::tlm_bw_transport_if<>>;

using TLMCallbackFunc = std::function<void(pybind11::bytearray)>;
struct input_detail {
  tlm::tlm_generic_payload tran;
  bool stop_flag;
  TLMCallbackFunc callback;
};

/// The `Initiator` is used for python to send TLM transactions
SC_MODULE(Initiator) {
 public:
  tlm_utils::simple_initiator_socket<Initiator> init_socket;
  sc_in<bool> resetn;
  sc_in<bool> clk;
  SC_CTOR(Initiator) : init_socket("init_socket") {
    SC_THREAD(run);
  }

  void put_input(tlm::tlm_generic_payload & trans, bool stop_flag,
                 TLMCallbackFunc& callback) {
    auto detail = std::make_shared<input_detail>();
    detail->stop_flag = stop_flag;
    detail->tran.deep_copy_from(trans);
    detail->tran.set_data_ptr(trans.get_data_ptr());
    detail->callback = callback;
    // we copy it to avoid the payload being deleted by the caller
    input_queue_.push_back(detail);
  }

  std::shared_ptr<input_detail> get_input() {
    if (input_queue_.empty()) {
      return nullptr;
    }
    auto input = input_queue_.front();
    input_queue_.pop_front();
    return input;
  }

  void bind(base_target_socket_type & s) {
    init_socket.bind(s);
  }

 private:
  void run() {
    sc_time delay = SC_ZERO_TIME;
    wait(resetn.posedge_event());
    while (true) {
      auto input = get_input();
      if (input) {
        cout << "send trans" << endl;

        // send transaction
        init_socket->b_transport(input->tran, delay);

        // if the transaction is not OK, we print error
        // otherwise we call the callback function
        if (input->tran.get_response_status() != tlm::TLM_OK_RESPONSE) {
          cerr << "error" << endl;
        } else {
          input->callback(pybind11::bytearray(
              reinterpret_cast<char*>(input->tran.get_data_ptr()),
              input->tran.get_data_length()));
        }

        // If the stop flag is set, we stop the simulation
        if (input->stop_flag) {
          sc_stop();
        }
      }
      wait(clk->posedge_event());
    }
  }

  // input transaction queue from python
  std::deque<std::shared_ptr<input_detail>> input_queue_;
};

#endif