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

ifneq "$(VCS_HOME)" ""
SYSTEMC_INCLUDE ?=$(VCS_HOME)/include/systemc232/
SYSTEMC_LIBDIR ?= $(VCS_HOME)/linux/lib
TLM2 ?= $(VCS_HOME)/etc/systemc/tlm/

HAVE_VERILOG=y
HAVE_VERILOG_VERILATOR?=n
HAVE_VERILOG_VCS=y
else
SYSTEMC ?= /usr/local/systemc-2.3.2/
SYSTEMC_INCLUDE ?=$(SYSTEMC)/include/
SYSTEMC_LIBDIR ?= $(SYSTEMC)/lib-linux64
# In case your TLM-2.0 installation is not bundled with
# with the SystemC one.
# TLM2 ?= /opt/systemc/TLM-2009-07-15
endif

SCML ?= /usr/local/scml-2.3/
SCML_INCLUDE ?= $(SCML)/include/
SCML_LIBDIR ?= $(SCML)/lib-linux64/

HAVE_VERILOG?=n
HAVE_VERILOG_VERILATOR?=n
HAVE_VERILOG_VCS?=n

CFLAGS += -Wall -O2 -g
CXXFLAGS += -Wall -O2 -g

ifneq "$(SYSTEMC_INCLUDE)" ""
CPPFLAGS += -I $(SYSTEMC_INCLUDE)
endif
ifneq "$(TLM2)" ""
CPPFLAGS += -I $(TLM2)/include/tlm
endif

# CPPFLAGS += -I .
CPPFLAGS += -I ./lib
LDFLAGS  += -L $(SYSTEMC_LIBDIR)
#LDLIBS += -pthread -Wl,-Bstatic -lsystemc -Wl,-Bdynamic
LDLIBS   += -pthread -lsystemc

PCIE_MODEL_O = pcie-model/tlm-modules/pcie-controller.o
PCIE_MODEL_O += pcie-model/tlm-modules/libpcie-callbacks.o
PCIE_MODEL_CPPFLAGS += -I pcie-model/libpcie/src -I pcie-model/

VERSAL_CPM_QDMA_DEMO_C = pcie/versal/cpm-qdma-demo.cc
VERSAL_CPM4_QDMA_DEMO_O = pcie/versal/cpm4-qdma-demo.o
VERSAL_CPM5_QDMA_DEMO_O = pcie/versal/cpm5-qdma-demo.o

PCIE_XDMA_DEMO_C = pcie/versal/xdma-demo.cc
PCIE_XDMA_DEMO_O = $(PCIE_XDMA_DEMO_C:.cc=.o)
PCIE_XDMA_DEMO_OBJS += $(PCIE_XDMA_DEMO_O) $(PCIE_MODEL_O)

VERSAL_CPM4_QDMA_DEMO_OBJS += $(VERSAL_CPM4_QDMA_DEMO_O) $(PCIE_MODEL_O)
VERSAL_CPM5_QDMA_DEMO_OBJS += $(VERSAL_CPM5_QDMA_DEMO_O) $(PCIE_MODEL_O)

# Uncomment to enable use of scml2
# CPPFLAGS += -I $(SCML_INCLUDE)
# LDFLAGS += -L $(SCML_LIBDIR)
# LDLIBS += -lscml2 -lscml2_logging

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
VOBJ_DIR=obj_dir
VFILES_DIR=bsv
VFILES=mkBsvTop.v

# ifeq "$(HAVE_VERILOG_VERILATOR)" "y"
VERILATOR_ROOT?=/usr/share/verilator
VERILATOR=verilator

VM_SC?=1
VM_TRACE?=0
VM_COVERAGE?=0
V_LDLIBS += $(VOBJ_DIR)/VmkBsvTop__ALL.a
LDLIBS += $(V_LDLIBS)
VERILATED_O=verilated.o

VFLAGS += -Wno-fatal
VFLAGS += --sc --Mdir $(VOBJ_DIR)
VFLAGS += -CFLAGS "-DHAVE_VERILOG" -CFLAGS "-DHAVE_VERILOG_VERILATOR"
VFLAGS += -y $(VFILES_DIR)
VFLAGS += --pins-bv 31
VFLAGS += --top-module mkBsvTop

CPPFLAGS += -DHAVE_VERILOG
CPPFLAGS += -DHAVE_VERILOG_VERILATOR
CPPFLAGS += -I $(VOBJ_DIR)
CPPFLAGS += -I $(VERILATOR_ROOT)/include

# ifeq "$(VM_TRACE)" "1"
# VFLAGS += --trace
# SC_OBJS += verilated_vcd_c.o
# SC_OBJS += verilated_vcd_sc.o
# CPPFLAGS += -DVM_TRACE=1
# endif
# endif

# ifeq "$(HAVE_VERILOG_VCS)" "y"
# VCS=vcs
# SYSCAN=syscan
# VLOGAN=vlogan
# VHDLAN=vhdlan

# CSRC_DIR = csrc

# VLOGAN_FLAGS += -sysc
# VLOGAN_FLAGS += +v2k -sc_model apb_slave_timer

# VHDLAN_FLAGS += -sysc
# VHDLAN_FLAGS += -sc_model apb_slave_dummy

# SYSCAN_ZYNQ_DEMO = zynq_demo.cc
# SYSCAN_ZYNQMP_DEMO = zynqmp_demo.cc
# SYSCAN_ZYNQMP_LMAC2_DEMO = zynqmp_lmac2_demo.cc
# SYSCAN_SCFILES += demo-dma.cc debugdev.cc remote-port-tlm.cc
# VCS_CFILES += remote-port-proto.c remote-port-sk.c safeio.c

# SYSCAN_FLAGS += -tlm2 -sysc=opt_if
# SYSCAN_FLAGS += -cflags -DHAVE_VERILOG -cflags -DHAVE_VERILOG_VCS
# VCS_FLAGS += -sysc sc_main -sysc=adjust_timeres
# VFLAGS += -CFLAGS "-DHAVE_VERILOG" -CFLAGS "-DHAVE_VERILOG_VERILATOR"
# endif



OBJS = $(C_OBJS) $(SC_OBJS)

VERSAL_CPM4_QDMA_DEMO_OBJS += $(OBJS)
VERSAL_CPM5_QDMA_DEMO_OBJS += $(OBJS)
PCIE_XDMA_DEMO_OBJS += $(OBJS)

TARGET_VERSAL_CPM4_QDMA_DEMO = pcie/versal/cpm4-qdma-demo
TARGET_VERSAL_CPM5_QDMA_DEMO = pcie/versal/cpm5-qdma-demo
TARGET_PCIE_XDMA_DEMO = pcie/versal/xdma-demo

PCIE_MODEL_DIR=pcie-model/tlm-modules
# ifneq ($(wildcard $(PCIE_MODEL_DIR)/.),)
# TARGETS += $(TARGET_VERSAL_CPM4_QDMA_DEMO)
# TARGETS += $(TARGET_VERSAL_CPM5_QDMA_DEMO)
# endif
TARGETS += $(TARGET_PCIE_XDMA_DEMO)

all: $(TARGETS)

-include $(VERSAL_CPM4_QDMA_DEMO_OBJS:.o=.d)
-include $(VERSAL_CPM5_QDMA_DEMO_OBJS:.o=.d)
-include $(PCIE_XDMA_DEMO_OBJS:.o=.d)
CFLAGS += -MMD
CXXFLAGS += -MMD

## libpcie ##
-include pcie-model/libpcie/libpcie.mk

$(VERILATED_O) : $(VFILES_DIR)
	$(VENV) $(VERILATOR) $(VFLAGS) $(VFILES)
	$(MAKE) -C $(VOBJ_DIR) -f VmkBsvTop.mk
	$(MAKE) -C $(VOBJ_DIR) -f VmkBsvTop.mk $(VERILATED_O)
	
# $(VERSAL_CPM5_QDMA_DEMO_O): $(VERSAL_CPM_QDMA_DEMO_C)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# $(TARGET_VERSAL_CPM5_QDMA_DEMO): CPPFLAGS += $(PCIE_MODEL_CPPFLAGS)
# $(TARGET_VERSAL_CPM5_QDMA_DEMO): LDLIBS += libpcie.a
# $(TARGET_VERSAL_CPM5_QDMA_DEMO): $(VERSAL_CPM5_QDMA_DEMO_OBJS) libpcie.a
# 	$(CXX) $(LDFLAGS) -o $@ $(VERSAL_CPM5_QDMA_DEMO_OBJS) $(LDLIBS)


# $(VERSAL_CPM4_QDMA_DEMO_O): $(VERSAL_CPM_QDMA_DEMO_C)
# 	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -DQDMA_CPM4_VERSION -c -o $@ $<

# $(TARGET_VERSAL_CPM4_QDMA_DEMO): CPPFLAGS += $(PCIE_MODEL_CPPFLAGS)
# $(TARGET_VERSAL_CPM4_QDMA_DEMO): LDLIBS += libpcie.a
# $(TARGET_VERSAL_CPM4_QDMA_DEMO): $(VERSAL_CPM4_QDMA_DEMO_OBJS) libpcie.a
# 	$(CXX) $(LDFLAGS) -o $@ $(VERSAL_CPM4_QDMA_DEMO_OBJS) $(LDLIBS)

$(TARGET_PCIE_XDMA_DEMO): CPPFLAGS += $(PCIE_MODEL_CPPFLAGS)
$(TARGET_PCIE_XDMA_DEMO): LDLIBS += libpcie.a
$(TARGET_PCIE_XDMA_DEMO): $(VERILATED_O) $(PCIE_XDMA_DEMO_OBJS) libpcie.a
	$(CXX) $(LDFLAGS) -o $@ $(PCIE_XDMA_DEMO_OBJS) $(LDLIBS) $(VOBJ_DIR)/$(VERILATED_O)
	
verilated_%.o: $(VERILATOR_ROOT)/include/verilated_%.cpp

clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGETS)
	$(RM) $(TARGET_VERSAL_CPM5_QDMA_DEMO) $(VERSAL_CPM5_QDMA_DEMO_OBJS)
	$(RM) $(VERSAL_CPM5_QDMA_DEMO_OBJS:.o=.d)
	$(RM) $(TARGET_VERSAL_CPM4_QDMA_DEMO) $(VERSAL_CPM4_QDMA_DEMO_OBJS)
	$(RM) $(VERSAL_CPM4_QDMA_DEMO_OBJS:.o=.d)
	$(RM) -r libpcie libpcie.a
	$(RM) $(TARGET_PCIE_XDMA_DEMO) $(PCIE_XDMA_DEMO_OBJS)
	$(RM) -r $(VOBJ_DIR)
