#! /usr/bin/env python3
import argparse
import re
N_CHANNELS = 1
CHANNEL_TYPE = "stream"  # "mm" or "stream
PREFIX = "xdmaChannel"
REL_PATH_TO_XDMA_SIGNAL_H = "libsystemctlm-soc/soc/pci/xilinx/xdma_signal.h"
REL_PATH_TO_XDMA_DEMO = "pcie/versal/xdma-demo.cc"
DMA_DATA_WIDTH = 256
DMA_ADDR_WIDTH = 64
BRIDGE_ADDR_WIDTH = 32
BRIDGE_DATA_WIDTH = 32
DMA_DATA_WIDTH_IN_BYTES = DMA_DATA_WIDTH // 8

bypass_field = [
    ("bool", "load"),
    (f"sc_bv<DMA_ADDR_WIDTH>", "src_addr"),
    ("uint32_t", "len"),
    (f"sc_bv<DMA_ADDR_WIDTH>", "dst_addr"),
    ("uint32_t", "ctl"),
    ("bool", "ready"),
    ("bool", "desc_done"),  # unsed
]
axis_field = [
    {"name": "tstrb", "generic_type": "signal",
        "type": f"sc_bv<DMA_DATA_WIDTH_IN_BYTES>"},
    {"name": "tlast", "generic_type": "signal", "type": "bool"},
    {"name": "tuser", "generic_type": "signal", "type": "bool"},
    {"name": "tkeep", "generic_type": "signal",
        "type": f"sc_bv<DMA_DATA_WIDTH_IN_BYTES>"},
    {"name": "tready", "generic_type": "signal", "type": "bool"},
    {"name": "tvalid", "generic_type": "signal", "type": "bool"},
    {"name": "tdata", "generic_type": "signal",
        "type": f"sc_bv<DMA_DATA_WIDTH>"},
]

axi_field = [
    {"name": "awvalid", "generic_type": "signal", "type": "bool"},
    {"name": "awready", "generic_type": "signal", "type": "bool"},
    {"name": "awaddr", "generic_type": "signal",
        "type": f"sc_bv<DMA_ADDR_WIDTH>"},
    {"name": "awprot",
     "generic_type": "adpater",
        "from_ty": "sc_bv<AXI_AWPROT_WIDTH>",
        "to_ty": "uint32_t",
        "h2c_func": "bvn_to_uint32t<AXI_AWPROT_WIDTH>",
        "c2h_func": "uint32t_to_bvn<AXI_AWPROT_WIDTH>",
        "h2c_signal_direction": "out"},
    {"name": "awuser", "generic_type": "signal", "type": "sc_bv<AXI_USER_WIDTH>"},
    {"name": "awregion", "generic_type": "signal",
        "type": "sc_bv<AXI_REGION_WIDTH>"},
    {"name": "awqos", "generic_type": "signal", "type": "sc_bv<AXI_QOS_WIDTH>"},
    {"name": "awcache", "generic_type": "signal", "type": "sc_bv<AXI_CACHE_WIDTH>"},
    {"name": "awburst", "generic_type": "signal", "type": "sc_bv<AXI_BURST_WIDTH>"},
    {"name": "awsize", "generic_type": "signal", "type": "sc_bv<AXI_SIZE_WIDTH>"},
    {"name": "awlen", "generic_type": "signal", "type": "sc_bv<AXI_LEN_WIDTH>"},
    {"name": "awid", "generic_type": "signal", "type": "sc_bv<AXI_ID_WIDTH>"},
    {"name": "awlock", "generic_type": "signal", "type": "sc_bv<AXI_LOCK_WIDTH>"},
    {"name": "wid", "generic_type": "signal", "type": "sc_bv<AXI_ID_WIDTH>"},
    {"name": "wvalid", "generic_type": "signal", "type": "bool"},
    {"name": "wready", "generic_type": "signal", "type": "bool"},
    {"name": "wdata", "generic_type": "signal",
        "type": f"sc_bv<DMA_DATA_WIDTH>"},
    {"name": "wstrb", "generic_type": "signal",
        "type": f"sc_bv<DMA_DATA_WIDTH_IN_BYTES>"},
    {"name": "wuser", "generic_type": "signal", "type": "sc_bv<AXI_USER_WIDTH>"},
    {"name": "wlast", "generic_type": "signal", "type": "bool"},
    {"name": "bvalid", "generic_type": "signal", "type": "bool"},
    {"name": "bready", "generic_type": "signal", "type": "bool"},
    {"name": "bresp", "generic_type": "signal", "type": "sc_bv<AXI_RESP_WIDTH>"},
    {"name": "buser", "generic_type": "signal", "type": "sc_bv<AXI_USER_WIDTH>"},
    {"name": "bid", "generic_type": "signal", "type": "sc_bv<AXI_ID_WIDTH>"},
    {"name": "arvalid", "generic_type": "signal", "type": "bool"},
    {"name": "arready", "generic_type": "signal", "type": "bool"},
    {"name": "araddr", "generic_type": "signal",
        "type": f"sc_bv<DMA_ADDR_WIDTH>"},
    {"name": "arprot",
     "generic_type": "adpater",
        "from_ty": "sc_bv<AXI_ARPROT_WIDTH>",
        "to_ty": "uint32_t",
        "h2c_func": "bvn_to_uint32t<AXI_ARPROT_WIDTH>",
        "c2h_func": "uint32t_to_bvn<AXI_ARPROT_WIDTH>",
        "h2c_signal_direction": "out"},
    {"name": "aruser", "generic_type": "signal", "type": "sc_bv<AXI_USER_WIDTH>"},
    {"name": "arregion", "generic_type": "signal",
        "type": "sc_bv<AXI_REGION_WIDTH>"},
    {"name": "arqos", "generic_type": "signal", "type": "sc_bv<AXI_QOS_WIDTH>"},
    {"name": "arcache", "generic_type": "signal", "type": "sc_bv<AXI_CACHE_WIDTH>"},
    {"name": "arburst", "generic_type": "signal", "type": "sc_bv<AXI_BURST_WIDTH>"},
    {"name": "arsize", "generic_type": "signal", "type": "sc_bv<AXI_SIZE_WIDTH>"},
    {"name": "arlen", "generic_type": "signal", "type": "sc_bv<AXI_LEN_WIDTH>"},
    {"name": "arid", "generic_type": "signal", "type": "sc_bv<AXI_ID_WIDTH>"},
    {"name": "arlock", "generic_type": "signal", "type": "sc_bv<AXI_LOCK_WIDTH>"},
    {"name": "rvalid", "generic_type": "signal", "type": "bool"},
    {"name": "rready", "generic_type": "signal", "type": "bool"},
    {"name": "rdata", "generic_type": "signal",
        "type": f"sc_bv<DMA_DATA_WIDTH>"},
    {"name": "rresp", "generic_type": "signal", "type": "sc_bv<AXI_RESP_WIDTH>"},
    {"name": "ruser", "generic_type": "signal", "type": "sc_bv<AXI_USER_WIDTH>"},
    {"name": "rid", "generic_type": "signal", "type": "sc_bv<AXI_ID_WIDTH>"},
    {"name": "rlast", "generic_type": "signal", "type": "bool"},
    {"name": "awsnoop", "generic_type": "signal",
        "type": "sc_bv<AXI_AWSNOOP_WIDTH>"},
    {"name": "awdomain", "generic_type": "signal",
        "type": "sc_bv<AXI_DOMAIN_WIDTH>"},
    {"name": "awbar", "generic_type": "signal", "type": "sc_bv<AXI_BAR_WIDTH>"},
    {"name": "wack", "generic_type": "signal", "type": "bool"},
    {"name": "arsnoop", "generic_type": "signal",
        "type": "sc_bv<AXI_ARSNOOP_WIDTH>"},
    {"name": "ardomain", "generic_type": "signal",
        "type": "sc_bv<AXI_DOMAIN_WIDTH>"},
    {"name": "arbar", "generic_type": "signal", "type": "sc_bv<AXI_BAR_WIDTH>"},
    {"name": "rack", "generic_type": "signal", "type": "bool"},
]


parser = argparse.ArgumentParser(
    description="Command line arguments for configuring XDMA demo settings")

# Defining the command line arguments
parser.add_argument("--n_channels", type=int, default=1,
                    help="Number of channels")
parser.add_argument("--channel_type", type=str, default="stream", choices=[
                    "mm", "stream"], help="Channel type: 'mm' for AXI or 'stream' for AXIs")
parser.add_argument("--dma_data_width", type=int,
                    default=256, help="DMA data width")
parser.add_argument("--dma_addr_width", type=int,
                    default=64, help="DMA address width")
parser.add_argument("--bridge_addr_width", type=int,
                    default=32, help="Bridge address width")
parser.add_argument("--bridge_data_width", type=int,
                    default=32, help="Bridge data width")

args = parser.parse_args()
N_CHANNELS = args.n_channels
CHANNEL_TYPE = args.channel_type
DMA_DATA_WIDTH = args.dma_data_width
DMA_ADDR_WIDTH = args.dma_addr_width
BRIDGE_ADDR_WIDTH = args.bridge_addr_width
BRIDGE_DATA_WIDTH = args.bridge_data_width

with open(REL_PATH_TO_XDMA_SIGNAL_H, "r") as file:
    source_string = file.read()


signals = []
if CHANNEL_TYPE == "mm":
    channel_fileds = axi_field
elif CHANNEL_TYPE == "stream":
    channel_fileds = axis_field
else:
    raise Exception("CHANNEL_TYPE must be mm or stream")
new_content = "\npublic:\n"
# print member variables
for i in range(N_CHANNELS):
    if N_CHANNELS == 1:
        idx = ""
    else:
        idx = i
        new_content += f"// channel {idx}\n"

    for channel_ty in ("h2c", "c2h"):
        for field in bypass_field:
            ty, name = field
            new_content += f"\tsc_signal<{ty}> {PREFIX}{idx}_{channel_ty}DescByp_{name};\n"
            signals.append(
                {"is_adapter": False, "name": f"{PREFIX}{idx}_{channel_ty}DescByp_{name}", "short_name": name})
        new_content += "\n"

    for channel_ty in ("H2c", "C2h"):
        for field in channel_fileds:
            if field["generic_type"] == "signal":
                ty, name = field["type"], field["name"]
                new_content += f"\tsc_signal<{ty}> {PREFIX}{idx}_raw{channel_ty}AxiStream_{name};\n"
                signals.append(
                    {"is_adapter": False,
                        "name": f"{PREFIX}{idx}_raw{channel_ty}AxiStream_{name}", "short_name": name}
                )
            else:
                # field["generic_type"] == "adpater"
                name, from_ty, to_ty, h2c_func, c2h_func, h2c_pin_direction = field["name"], field[
                    "from_ty"], field["to_ty"], field["h2c_func"], field["c2h_func"], field["h2c_signal_direction"]
                signal_name = f"{PREFIX}{idx}_raw{channel_ty}AxiStream_{name}"
                if channel_ty == "H2c":
                    new_type = f"type_adapter<{from_ty},{to_ty},{h2c_func}> {signal_name};"
                else:
                    new_type = f"type_adapter<{to_ty},{from_ty},{c2h_func}> {signal_name};"
                new_content += f"\t{new_type}\n"
                signals.append({"is_adapter": True, "name": signal_name,
                               "h2c_pin_direction": h2c_pin_direction, "short_name": name})
        new_content += "\n"

# print connect_user_logic
new_content += "\ttemplate <typename T> void connect_user_logic(T *dev) {\n"
for i in range(N_CHANNELS):
    idx = i
    if N_CHANNELS == 1:
        idx = ""
    for signal in signals:
        is_adapter, name = signal["is_adapter"], signal["name"]
        if "tuser" in name or "tstrb" in name:
            continue  # 没接线
        if not is_adapter:
            new_content += f"\t\tdev->{name}({name});\n"
        else:
            h2c = "H2c" in name or "h2c" in name
            h2c_pin_direction = signal["h2c_pin_direction"]
            c2h_pin_direction = "in" if h2c_pin_direction == "out" else "out"
            if h2c:
                new_content += f"\t\tdev->{name}({name}.{h2c_pin_direction}_pin_signal);\n"
            else:
                new_content += f"\t\tdev->{name}({name}.{c2h_pin_direction}_pin_signal);\n"
new_content += "\t}\n"

# print connect_xdma
new_content += "\ttemplate <typename T> void connect_xdma(T *dev) {\n"
for i in range(N_CHANNELS):
    # desc bypass
    for signal in signals:
        signal_name = signal["name"]

        is_desc_bypass = "DescByp" in signal_name
        if "H2c" in signal_name or "h2c" in signal_name:
            channel_ty = "h2c"
        else:
            channel_ty = "c2h"

        short_name = signal["short_name"]
        if is_desc_bypass:
            if "desc_done" in short_name:
                continue
            new_content += f"\t\tdev->descriptor_bypass_channels[{i}].dsc_bypass_bridge_{channel_ty}.{short_name}({signal_name});\n"
        else:
            if "tstrb" in signal_name:
                continue  # 没接线
            if "tkeep" in signal_name:
                short_name = "tstrb"  # tkeep -> tstrb，特殊处理
            if not signal["is_adapter"]:
                new_content += f"\t\tdev->descriptor_bypass_channels[{i}].{channel_ty}_bridge.{short_name}({signal_name});\n"
            else:
                h2c = "H2c" in signal_name or "h2c" in signal_name
                c2h_pin_direction_rev = signal["h2c_pin_direction"]
                h2c_pin_direction_rev = "in" if h2c_pin_direction == "out" else "out"
                if h2c:
                    new_content += f"\t\tdev->descriptor_bypass_channels[{i}].{channel_ty}_bridge.{short_name}({signal['name']}.{h2c_pin_direction_rev}_pin_signal);\n"
                else:
                    new_content += f"\t\tdev->descriptor_bypass_channels[{i}].{channel_ty}_bridge.{short_name}({signal['name']}.{c2h_pin_direction_rev}_pin_signal);\n"

new_content += "\t}\n"

new_content += "\ttemplate <typename T> void connect_user_logic(T &dev) {connect_user_logic(&dev);}\n"
new_content += "\ttemplate <typename T> void connect_xdma(T &dev) { connect_xdma(&dev); }\n"
new_content += "\txdma_bypass_signal(sc_core::sc_module_name name):sc_module(name)"
for signal in signals:
    new_content += f",\n\t\t{signal['name']}(\"{signal['name']}\")"
new_content += "{}\n"

# replace old content
pattern = r'class xdma_bypass_signal : public sc_core::sc_module \s*\{(.*?)\};'
result = re.sub(
    pattern, f'class xdma_bypass_signal : public sc_core::sc_module {{{new_content}}};', source_string, flags=re.DOTALL)

# update #defination
pattern = r'#define DMA_DATA_WIDTH (\d+)'
result = re.sub(pattern, f'#define DMA_DATA_WIDTH {DMA_DATA_WIDTH}', result)
pattern = r'#define DMA_ADDR_WIDTH (\d+)'
result = re.sub(pattern, f'#define DMA_ADDR_WIDTH {DMA_ADDR_WIDTH}', result)
pattern = r'#define BRIDGE_DATA_WIDTH (\d+)'
result = re.sub(
    pattern, f'#define BRIDGE_DATA_WIDTH {BRIDGE_DATA_WIDTH}', result)
pattern = r'#define BRIDGE_ADDR_WIDTH (\d+)'
result = re.sub(
    pattern, f'#define BRIDGE_ADDR_WIDTH {BRIDGE_ADDR_WIDTH}', result)
pattern = r'#define DMA_DATA_WIDTH_IN_BYTES (\d+)'
result = re.sub(
    pattern, f'#define DMA_DATA_WIDTH_IN_BYTES {DMA_DATA_WIDTH_IN_BYTES}', result)

with open(REL_PATH_TO_XDMA_SIGNAL_H, "w") as file:
    file.write(result)

# Now, we need to update xdma-demo.cc
# Update xdma-demo.cc
with open(REL_PATH_TO_XDMA_DEMO, "r") as file:
    source_string = file.read()
pattern = r'#define XDMA_CHANNEL_NUM (\d)'
result = re.sub(
    pattern, f'#define XDMA_CHANNEL_NUM {N_CHANNELS}', source_string)
if CHANNEL_TYPE == "mm":
    pattern = r'#define XDMA_BYPASS_C2H_BRIDGE\s+(.*)'
    result = re.sub(
        pattern, f'#define XDMA_BYPASS_C2H_BRIDGE axi2tlm_bridge<DMA_DATA_WIDTH,DMA_ADDR_WIDTH>', result)
    pattern = r'#define XDMA_BYPASS_H2C_BRIDGE\s+(.*)'
    result = re.sub(
        pattern, f'#define XDMA_BYPASS_H2C_BRIDGE tlm2axi_bridge<DMA_DATA_WIDTH,DMA_ADDR_WIDTH>', result)
elif CHANNEL_TYPE == "stream":
    pattern = r'#define XDMA_BYPASS_C2H_BRIDGE\s+(.*)'
    result = re.sub(
        pattern, f'#define XDMA_BYPASS_C2H_BRIDGE axis2tlm_bridge<DMA_DATA_WIDTH>', result)
    pattern = r'#define XDMA_BYPASS_H2C_BRIDGE\s+(.*)'
    result = re.sub(
        pattern, f'#define XDMA_BYPASS_H2C_BRIDGE tlm2axis_bridge<DMA_DATA_WIDTH>', result)
with open(REL_PATH_TO_XDMA_DEMO, "w") as file:
    file.write(result)
