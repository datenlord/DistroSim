#include "systemc.h"
#include <unistd.h>

#ifndef XDMA_PYTHON_THREAD_EVENT_H__
#define XDMA_PYTHON_THREAD_EVENT_H__
class thread_safe_event_if : public sc_interface {
    virtual void notify(sc_time delay = SC_ZERO_TIME) = 0;
     const sc_event &default_event() const override = 0;
protected:
    virtual void update() = 0;
};

class thread_safe_event : public sc_prim_channel, public thread_safe_event_if {
public:
    explicit thread_safe_event(const char *name = ""): event(name) {}

    void notify(sc_time delay = SC_ZERO_TIME) override {
        this->delay = delay;
        async_request_update();
    }

    const sc_event &default_event() const override {
        return event;
    }
protected:
     void update() override {
        event.notify(delay);
    }
    sc_event event;
    sc_time delay;
};

#endif // XDMA_PYTHON_THREAD_EVENT_H__