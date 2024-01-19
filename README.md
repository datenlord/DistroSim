# Software and Hardware Co-Simulation

### Prerequisites
* SystemC
* verilator

### Build XDMA demo
```
make verilated.o
make pcie/versal/xdma-demo
```
The Makefile will try to convert verilog to systemC firstly and compile it with xdma-demo and TLM libraries.

### Configure Channels
```bash
# scripts/xdma_signal_generator.py  -h
usage: xdma_signal_generator.py [-h] [--n_channels N_CHANNELS] [--channel_type {mm,stream}] [--dma_data_width DMA_DATA_WIDTH] [--dma_addr_width DMA_ADDR_WIDTH] [--bridge_addr_width BRIDGE_ADDR_WIDTH] [--bridge_data_width BRIDGE_DATA_WIDTH]

Command line arguments for configuring XDMA demo settings

options:
  -h, --help            show this help message and exit
  --n_channels N_CHANNELS
                        Number of channels
  --channel_type {mm,stream}
                        Channel type: 'mm' for AXI or 'stream' for AXIs
  --dma_data_width DMA_DATA_WIDTH
                        DMA data width
  --dma_addr_width DMA_ADDR_WIDTH
                        DMA address width
  --bridge_addr_width BRIDGE_ADDR_WIDTH
                        Bridge address width
  --bridge_data_width BRIDGE_DATA_WIDTH
                        Bridge data width
```
### Build Python wrapper
additional requirements: 
* pybind11
update the header path in .config.mk. For example:
```
SYSTEMC = /workspaces/cosim_demo/systemc-2.3.3
PYBIND11_INCLUDE = /usr/local/lib/python3.10/dist-packages/pybind11/include
PYTHON_INCLUDE = /usr/include/python3.10
```

#### (Optional) Rebuild systemC with flag=c++14
Verilator dyanmic linking require us to have same version of systemC library built by same standard of c++ compiler. That means if the verilator use c++14 to compile , it will require our system to compile with c++14 as well.

We can add the following flags when using configure:
```
./configure --prefix=$SYSTEMC_PATH 'CXXFLAGS=-std=c++14 -g'
make -j
make install
```

#### Build python wrapper
Use `make python/libdistrosim.so` to compile libdistrosim.so. 

The you can use python to import `libdistrosim` .

Before you use the dynamic linking, make sure SystemC path is in the `LD_LIBRARY_PATH`
```
export LD_LIBRARY_PATH=/workspaces/cosim_demo/systemc-2.3.3/lib-linux64:$LD_LIBRARY_PATH
```

