#ifndef DISTROSIM_H__
#define DISTROSIM_H__
#include <pybind11/pybind11.h>
namespace py = pybind11;
class distrosim_top {
 public:
  static void register_to_pybind11(py::module& module){};
};

#endif