#
# Cosim Makefiles
#
# Copyright (c) 2016 Xilinx Inc.
# Written by Edgar E. Iglesias
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

-include .config.mk

INSTALL ?= install

SYSTEMC ?= /usr/local/systemc-2.3.2/
SYSTEMC_INCLUDE ?=$(SYSTEMC)/include/
SYSTEMC_LIBDIR ?= $(SYSTEMC)/lib-linux64

VERILATOR_ROOT?=/usr/share/verilator
VERILATOR=verilator

CFLAGS += -Wall -O2 -g
CXXFLAGS += -Wall -O2 -g

CPPFLAGS += -I $(SYSTEMC_INCLUDE)
CPPFLAGS += -I ./lib
LDFLAGS  += -L $(SYSTEMC_LIBDIR)
LDLIBS   += -pthread -lsystemc

PCIE_MODEL_O = pcie-model/tlm-modules/pcie-controller.o
PCIE_MODEL_O += pcie-model/tlm-modules/libpcie-callbacks.o
PCIE_MODEL_CPPFLAGS += -I pcie-model/libpcie/src -I pcie-model/

PCIE_XDMA_DEMO_C = pcie/versal/xdma-demo.cc
PCIE_XDMA_DEMO_O = $(PCIE_XDMA_DEMO_C:.cc=.o)
PCIE_XDMA_DEMO_OBJS += $(PCIE_XDMA_DEMO_O) $(PCIE_MODEL_O)

VOBJ_DIR=obj_dir
VFILES_DIR=bsv
VTOP_FILE=mkBsvTop.v
VTOP_BASENAME=$(basename $(VTOP_FILE))

SC_OBJS += ./lib/trace.o
SC_OBJS += ./lib/debugdev.o
SC_OBJS += ./lib/demo-dma.o
SC_OBJS += ./lib/xilinx-axidma.o

LIBSOC_PATH=libsystemctlm-soc
CPPFLAGS += -I $(LIBSOC_PATH)

CPPFLAGS += -I $(LIBSOC_PATH)/soc/xilinx/versal/
SC_OBJS += $(LIBSOC_PATH)/soc/xilinx/versal/xilinx-versal.o

CPPFLAGS += -I $(LIBSOC_PATH)/soc/xilinx/versal-net/
SC_OBJS += $(LIBSOC_PATH)/soc/xilinx/versal-net/xilinx-versal-net.o

CPPFLAGS += -I $(LIBSOC_PATH)/tests/test-modules/
SC_OBJS += $(LIBSOC_PATH)/tests/test-modules/memory.o

LIBRP_PATH=$(LIBSOC_PATH)/libremote-port
C_OBJS += $(LIBRP_PATH)/safeio.o
C_OBJS += $(LIBRP_PATH)/remote-port-proto.o
C_OBJS += $(LIBRP_PATH)/remote-port-sk.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-memory-master.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-memory-slave.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-wires.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-ats.o
SC_OBJS += $(LIBRP_PATH)/remote-port-tlm-pci-ep.o
SC_OBJS += $(LIBSOC_PATH)/soc/pci/core/pci-device-base.o
SC_OBJS += $(LIBSOC_PATH)/soc/dma/xilinx/mcdma/mcdma.o
SC_OBJS += $(LIBSOC_PATH)/soc/net/ethernet/xilinx/mrmac/mrmac.o
CPPFLAGS += -I $(LIBRP_PATH)

VENV=SYSTEMC_INCLUDE=$(SYSTEMC_INCLUDE) SYSTEMC_LIBDIR=$(SYSTEMC_LIBDIR)

# Generating pattern: V$(name)__ALL.a
V_LDLIBS += $(VOBJ_DIR)/V$(VTOP_BASENAME)__ALL.a
LDLIBS += $(V_LDLIBS)
VERILATED_O=verilated.o

VFLAGS += -Wno-fatal
VFLAGS += --sc --Mdir $(VOBJ_DIR)
VFLAGS += -CFLAGS "-DHAVE_VERILOG" -CFLAGS "-DHAVE_VERILOG_VERILATOR"
VFLAGS += -y $(VFILES_DIR)
VFLAGS += --pins-bv 31
VFLAGS += --top-module $(VTOP_BASENAME)

CPPFLAGS += -DHAVE_VERILOG
CPPFLAGS += -DHAVE_VERILOG_VERILATOR
CPPFLAGS += -I $(VOBJ_DIR)
CPPFLAGS += -I $(VERILATOR_ROOT)/include

OBJS = $(C_OBJS) $(SC_OBJS)

PCIE_XDMA_DEMO_OBJS += $(OBJS)

TARGET_PCIE_XDMA_DEMO = pcie/versal/xdma-demo

PCIE_MODEL_DIR=pcie-model/tlm-modules

TARGETS += $(TARGET_PCIE_XDMA_DEMO)

all: $(TARGETS)

-include $(PCIE_XDMA_DEMO_OBJS:.o=.d)
CFLAGS += -MMD
CXXFLAGS += -MMD

## libpcie ##
-include pcie-model/libpcie/libpcie.mk

$(VERILATED_O) : $(VFILES_DIR)
	$(VENV) $(VERILATOR) $(VFLAGS) $(VTOP_FILE)
	$(MAKE) -C $(VOBJ_DIR) -f V$(VTOP_BASENAME).mk
	$(MAKE) -C $(VOBJ_DIR) -f V$(VTOP_BASENAME).mk $(VERILATED_O)

# Generating header file and the verilated.o
$(VOBJ_DIR)/V$(VTOP_BASENAME).h: $(VERILATED_O)

$(TARGET_PCIE_XDMA_DEMO): CPPFLAGS += $(PCIE_MODEL_CPPFLAGS)
$(TARGET_PCIE_XDMA_DEMO): LDLIBS += libpcie.a
$(TARGET_PCIE_XDMA_DEMO): $(VERILATED_O) $(PCIE_XDMA_DEMO_OBJS) libpcie.a 
	$(CXX) $(LDFLAGS) -o $@ $(PCIE_XDMA_DEMO_OBJS) $(LDLIBS) $(VOBJ_DIR)/$(VERILATED_O)
	
clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGETS)
	$(RM) -r libpcie libpcie.a
	$(RM) $(TARGET_PCIE_XDMA_DEMO) $(PCIE_XDMA_DEMO_OBJS)
	$(RM) -r $(VOBJ_DIR)
