#ifndef SIMPLE_EXTERNAL_INITIATOR_H__
#define SIMPLE_EXTERNAL_INITIATOR_H__
#include "sysc/kernel/sc_module.h"
#include "sysc/kernel/sc_time.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_gp.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_core/tlm_2/tlm_generic_payload/tlm_generic_payload.h"
#include "thread_event.h"
#include <sysc/communication/sc_mutex.h>
#include <systemc.h>
#include <cstdio>
#include <iostream>
#include <memory>

template<typename MODULE,unsigned int BUSWIDTH = 32,typename TYPES = tlm::tlm_base_protocol_types>
SC_MODULE(simple_external_socket){
    using base_target_socket_type = tlm::tlm_base_target_socket_b<BUSWIDTH, tlm::tlm_fw_transport_if<TYPES>, tlm::tlm_bw_transport_if<TYPES>>;
    public:
        tlm_utils::simple_initiator_socket<MODULE> init;
        thread_safe_event external_event;
        sc_event proxy_send_event;
        sc_mutex proxy_mutex;
        tlm::tlm_generic_payload* arg_trans{nullptr};
        sc_core::sc_time* arg_delay{nullptr};

        SC_CTOR(simple_external_socket):
        external_event("external_event"){
            SC_METHOD(external_event_triger);
            sensitive << external_event;
            dont_initialize();
            SC_THREAD(initiator_proxy);
        }

        void bind(base_target_socket_type& s){
            init.bind(s);
        }

        void b_transport(tlm::tlm_generic_payload & trans, sc_time & delay){
            // set argument
            arg_trans = &trans;
            arg_delay = &delay;
            external_event.notify();
            // wait(proxy_done_event);
        }
    private:
    
        void initiator_proxy(){
            while(true){
                cerr<<"0000" << std::endl;
                wait(proxy_send_event);
                cerr<<"cccc" << std::endl;
                if (arg_trans && arg_delay){
                    proxy_mutex.lock();
                    init->b_transport(*arg_trans,*arg_delay);
                    proxy_mutex.unlock();
                }
                cerr<<"dddd" << std::endl;
                // proxy_done_event.notify();
            }
        }

        void external_event_triger(){
            printf("bbbb\n");
            proxy_send_event.notify();
        }
        
};

#endif