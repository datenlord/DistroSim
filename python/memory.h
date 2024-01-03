#ifndef PYTHON_MEMORY_H__
#define PYTHON_MEMORY_H__
#include <pybind11/pybind11.h>
#include <sys/types.h>
#include <cstdint>
#include <memory>
#include "pybind11/pytypes.h"
#include <sys/mman.h>
namespace py = pybind11;

#define PAGE_SIZE 4096
class memory {
 public:
  explicit memory(uint64_t len,bool align_to_page) : len_(len) {
    if (!align_to_page){
        data_ = new char[len];
    }else{
        len_ = ((len+PAGE_SIZE-1) / PAGE_SIZE)  * PAGE_SIZE;
        data_ = static_cast<char*>(mmap(nullptr, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    }
  }
  uint32_t read32(uint64_t addr) {
    check_addr<4>(addr);
    return *reinterpret_cast<uint32_t*>(data_ + addr);
  }

  void write32(uint64_t addr, uint32_t data) {
    check_addr<4>(addr);
    *reinterpret_cast<uint32_t*>(data_ + addr) = data;
  }
  uint64_t read64(uint64_t addr) {
    check_addr<8>(addr);
    return *reinterpret_cast<uint64_t*>(data_ + addr);
  }
  void write64(uint64_t addr, uint64_t data) {
    check_addr<8>(addr);
    *reinterpret_cast<uint64_t*>(data_ + addr) = data;
  }

  uint8_t read8(uint64_t addr) {
    check_addr<1>(addr);
    return *reinterpret_cast<uint8_t*>(data_ + addr);
  }

  void write8(uint64_t addr, uint8_t data) {
    check_addr<1>(addr);
    *reinterpret_cast<uint8_t*>(data_ + addr) = data;
  }

  uint16_t read16(uint64_t addr) {
    check_addr<2>(addr);
    return *reinterpret_cast<uint16_t*>(data_ + addr);
  }

  void write16(uint64_t addr, uint16_t data) {
    check_addr<2>(addr);
    *reinterpret_cast<uint16_t*>(data_ + addr) = data;
  }

  py::bytes read(uint64_t addr, uint64_t len) {
    if (addr < 0 || addr + len > len_) {
      throw py::index_error();
    }
    return py::bytes(data_ + addr, len);
  }

  unsigned char* get_ptr(int offset = 0) {
    return reinterpret_cast<unsigned char*>(data_) + offset;
  }

  uint64_t get_raw_addr() { return reinterpret_cast<uint64_t>(data_); }

  // TODO: double free here
  // ~memory() {
  //     delete[] data_;
  // }

 private:
  template <uint64_t OFFSET>
  inline void check_addr(uint64_t addr) const {
    if (addr + OFFSET > len_) {
      throw py::index_error();
    }
  }
  char* data_;
  uint64_t len_;
};

#endif
