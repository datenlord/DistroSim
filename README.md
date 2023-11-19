# Software and Hardware Co-Simulation

### Prerequisites
* SystemC
* verilator

### Build XDMA demo
```
make -j 
```
The Makefile will try to convert verilog to systemC firstly and compile it with xdma-demo and TLM libraries.

### Configure Channels
```bash
# ./xdma_signal_generator.py  -h
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
