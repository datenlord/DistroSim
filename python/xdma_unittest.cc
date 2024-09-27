#include <sys/mman.h>
#include <sysc/kernel/sc_object.h>
#include <sysc/kernel/sc_wait.h>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include "distrosim.h"
#include "generic_initiator.h"
#include "generic_target.h"
#include "sysc/communication/sc_clock.h"
#include "sysc/kernel/sc_dynamic_processes.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "xdma_top.h"

#define DESCRIPTOR_LENGTH 32
#define BATCH_DESCRIPTOR_NUM 8
#define H2C_FETCH_LENGTH DESCRIPTOR_LENGTH* BATCH_DESCRIPTOR_NUM
#define C2H_FETCH_LENGTH DESCRIPTOR_LENGTH
#define GLOBAL_QUANTUM 10000
#define TIME_RESOLUTION 1
#define TIME_RESOLUTION_UNIT
#define MEMORY_SIZE 4096
#define MEMORY_ADDR_LOW_REGISTER 0x80
#define MEMORY_ADDR_HIGH_REGISTER 0x84
#define MEMORY_HEAD_POINTER_REGISTER 0x88
#define MEMORY_TAIL_POINTER_REGISTER 0x8c
#define XMDA_CHANNEL_1 0

// Test associated constants and structs
#define MEMORY_HEAD_POINTER_VAL 4
#define COUNTER_END_CONDITION MEMORY_HEAD_POINTER_VAL

struct xdma_unittest_context {
  // a page of memory for testing
  // Note that the memory **should** alligned to 4K
  char* memory;
  
  // a counter for counting the number of c2h data transfer transactions
  int counter{0};

  // an event for notifying the end of transaction
  sc_event end_of_transaction;
};

class user_bar_module {
 public:
  explicit user_bar_module(sc_clock& clk, sc_signal<bool>& resetn)
      : user_bar_initiator("user_bar_initiator", clk, resetn) {}

  generic_initiator user_bar_initiator;
};

class host_to_card_descriptor_bypass_module {
 public:
  explicit host_to_card_descriptor_bypass_module(sc_clock& clk,
                                                 sc_signal<bool>& resetn)
      : descriptor_bypass_handler("h2c_descriptor_bypass_handler"),
        h2c_data_transfer("h2c_data_transfer", clk, resetn) {}

  generic_target descriptor_bypass_handler;
  generic_initiator h2c_data_transfer;
};

class card_to_host_descriptor_bypass_module {
 public:
  explicit card_to_host_descriptor_bypass_module()
      : descriptor_bypass_handler("c2h_descriptor_bypass_handler"),
        c2h_data_transfer("c2h_data_transfer") {}

  generic_target descriptor_bypass_handler;
  generic_target c2h_data_transfer;
};

// generate stimuli for user bar initiator 
void user_bar_init_thread(generic_initiator* self,
                          std::shared_ptr<xdma_unittest_context>& context) {
  tlm::tlm_generic_payload payload;
  int val;
  uint32_t low_addr;
  uint32_t high_addr;
  sc_time delay = SC_ZERO_TIME;

  // wait reset
  wait(self->resetn.posedge_event());

  // get the address of the page
  auto* memory = context->memory;
  low_addr = reinterpret_cast<uint64_t>(memory) & 0xffffffff;
  high_addr = reinterpret_cast<uint64_t>(memory) >> 32;

  // prepare payload
  payload.set_data_ptr(reinterpret_cast<unsigned char*>(&val));
  payload.set_data_length(sizeof(uint32_t));
  payload.set_streaming_width(sizeof(uint32_t));
  payload.set_write();

  // write low_addr
  val = low_addr;
  payload.set_address(MEMORY_ADDR_LOW_REGISTER);
  payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
  self->put_stimuli_blockingly(payload, delay);
  assert(payload.get_response_status() == tlm::TLM_OK_RESPONSE);

  // write high_addr
  val = high_addr;
  payload.set_address(MEMORY_ADDR_HIGH_REGISTER);
  self->put_stimuli_blockingly(payload, delay);
  assert(payload.get_response_status() == tlm::TLM_OK_RESPONSE);

  // write head pointer
  val = MEMORY_HEAD_POINTER_VAL;
  payload.set_address(MEMORY_HEAD_POINTER_REGISTER);
  payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
  self->put_stimuli_blockingly(payload, delay);
  assert(payload.get_response_status() == tlm::TLM_OK_RESPONSE);

  // wait for the end of transaction
  wait(context->end_of_transaction);

  // read head pointer
  val = 0;
  payload.set_read();
  payload.set_address(MEMORY_HEAD_POINTER_REGISTER);
  payload.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
  self->put_stimuli_blockingly(payload, delay);

  assert(payload.get_response_status() == tlm::TLM_OK_RESPONSE);
  assert(val == MEMORY_HEAD_POINTER_VAL);

  // finish test
  sc_stop();
}

// handle host-to-card descriptor bypass TLM transaction
void descriptor_bypass_h2c_handler(generic_target* self,
                                   tlm::tlm_generic_payload* payload,
                                   sc_core::sc_time* /*delay*/) {
  auto* extension = payload->get_extension<xdma_bypass_descriptor_extension>();
  if (extension == nullptr) {
    throw std::runtime_error("No extension");
  }

  // prepare to send data from host to card
  auto len = extension->get_len();

  assert(len == H2C_FETCH_LENGTH);

  auto addr = payload->get_address();
  auto* data_ptr = payload->get_data_ptr();
  tlm::tlm_generic_payload h2c_transcation;
  sc_time delay = SC_ZERO_TIME;

  h2c_transcation.set_write();
  h2c_transcation.set_address(addr);
  h2c_transcation.set_data_ptr(data_ptr);

  h2c_transcation.set_data_length(len);
  h2c_transcation.set_streaming_width(len);
  h2c_transcation.set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

  self->put_stimuli_via_mux_blockingly(h2c_transcation, delay);

  assert(h2c_transcation.get_response_status() == tlm::TLM_OK_RESPONSE);

  // finish last transaction
  payload->set_response_status(tlm::TLM_OK_RESPONSE);
}

// handle card-to-host descriptor bypass TLM transaction
void descriptor_bypass_c2h_handler(generic_target* /*self*/,
                                   tlm::tlm_generic_payload* payload,
                                   sc_core::sc_time* /*delay*/) {
  auto* extension = payload->get_extension<xdma_bypass_descriptor_extension>();
  if (extension == nullptr) {
    throw std::runtime_error("No extension");
  }

  // prepare to send data from host to card
  auto len = extension->get_len();

  assert(len == C2H_FETCH_LENGTH);

  payload->set_response_status(tlm::TLM_OK_RESPONSE);
}

// handle card-to-host data transfer.
void c2h_data_transfer_handler(
    generic_target* /*target*/, tlm::tlm_generic_payload* payload,
    sc_core::sc_time* /*delay*/,
    std::shared_ptr<xdma_unittest_context>& context) {
  context->counter++;
  payload->set_response_status(tlm::TLM_OK_RESPONSE);

  if (context->counter == COUNTER_END_CONDITION) {
    context->end_of_transaction.notify();
  }
}

int sc_main(int /*argc*/, char* /*argv*/[]) {
  // initialize environment
  sc_set_time_resolution(TIME_RESOLUTION, SC_PS);
  tlm_utils::tlm_quantumkeeper::set_global_quantum(
      sc_time(static_cast<double>(GLOBAL_QUANTUM), SC_NS));

  // prepare a page of memory
  auto context = std::make_shared<xdma_unittest_context>();
  context->memory =
      static_cast<char*>(mmap(nullptr, MEMORY_SIZE, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

  // create top
  auto top = std::make_unique<xdma_top>("xdma_top");
  auto user_bar =
      std::make_unique<user_bar_module>(top->clock_signal, top->rst_n);
  auto h2c_handler = std::make_unique<host_to_card_descriptor_bypass_module>(
      top->clock_signal, top->rst_n);
  auto c2h_handler = std::make_unique<card_to_host_descriptor_bypass_module>();

  // bind user bar
  user_bar->user_bar_initiator.init_socket.bind(top->xdma->user_bar.tgt_socket);

  // bind XDMA descriptor bypass channel
  top->xdma->descriptor_bypass_channels[0].bind(
      h2c_handler->descriptor_bypass_handler.tgt_socket,
      c2h_handler->descriptor_bypass_handler.tgt_socket,
      h2c_handler->h2c_data_transfer.init_socket,
      c2h_handler->c2h_data_transfer.tgt_socket);

  // register handler functions
  std::vector<sc_event*> sc_events{&context->end_of_transaction};

  user_bar->user_bar_initiator.create_sc_thread(
      "user_bar_thread",
      [&context](generic_initiator* self) {
        user_bar_init_thread(self, context);
      },
      sc_events);

  h2c_handler->descriptor_bypass_handler.register_b_transport(
      descriptor_bypass_h2c_handler);
  h2c_handler->descriptor_bypass_handler.register_stimuli_mux_handler(
      [&](tlm::tlm_generic_payload* payload, sc_core::sc_time* delay) {
        h2c_handler->h2c_data_transfer.put_stimuli_blockingly(*payload, *delay);
      });

  c2h_handler->descriptor_bypass_handler.register_b_transport(
      descriptor_bypass_c2h_handler);
  c2h_handler->c2h_data_transfer.register_b_transport(
      [&](generic_target* target, tlm::tlm_generic_payload* payload,
          sc_core::sc_time* delay) {
        c2h_data_transfer_handler(target, payload, delay, context);
      });

  sc_start();

  return 0;
}