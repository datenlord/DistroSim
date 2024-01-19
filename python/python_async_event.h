#ifndef PYTHON_ASYNC_EVENT_H__
#define PYTHON_ASYNC_EVENT_H__
#include <pybind11/pybind11.h>
#include <iostream>
using namespace std;
namespace py = pybind11;

/// The `python_async_event` is a struct that wraps the python asyncio event
struct python_async_event {
 public:
  explicit python_async_event(py::object &future) : future(future){
  }
  py::object future;

  py::object await(){
    return future.attr("__await__")();
  }
};

#endif