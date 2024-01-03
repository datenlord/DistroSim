#! /bin/env python3
from libxdma import *
init()
top = get_top()
user_bar = top.py_user_bar
h2c_data = top.py_h2c_data
c2h_data = top.py_c2h_data
dsc_bypass_c2h = top.py_dsc_bypass_c2h
dsc_bypass_h2c = top.py_dsc_bypass_h2c
fake_memory = memory(4096,True) # align to 4K
temp_reg = memory(16,False)

# do nothing, just print the memory
def peek(array):
    print(array)
    
def dsc_bypass_h2c_b_transport(tran):
    print("on dsc_bypass_h2c_b_transport")
    ext = tran.get_xdma_bypass_descriptor_extension()
    # triger h2c channel to send data
    new_tran = tlm_generic_payload()
    new_tran.set_write()
    new_tran.set_data_length(ext.get_len())
    new_tran.set_streaming_width(ext.get_len())
    new_tran.set_address(tran.get_address())
    new_tran.set_data_ptr(tran.get_data_ptr())
    new_tran.set_eop()
    h2c_data.put_input(new_tran,False,peek)
    
def dsc_bypass_c2h_b_transport(tran):
     print("on dsc_bypass_c2h_b_transport")
    # print(f"c2h: {tran.get_address() } to {hex(tran.get_data_ptr())} with length {tran.get_data_length()}")
    
def c2h_data_b_transport(tran):
    print("on c2h_data_b_transport")
    print(f"c2h: {tran.get_address() } to {hex(tran.get_data_ptr())} with length {tran.get_data_length()}")

# write 0x80,0x84 with the base addr
# and then write the head counter 0x88 with 2
def write_base_addr():
    low = fake_memory.get_raw_addr() & 0xffffffff
    high = fake_memory.get_raw_addr() >> 32
    
    # write low addr
    temp_reg.write32(0,low)
    tran1 = tlm_generic_payload()
    tran1.set_write()
    tran1.set_data_length(4)
    tran1.set_streaming_width(4)
    tran1.set_address(0x80)
    tran1.set_data_ptr_with_memory(temp_reg,0)
    user_bar.put_input(tran1,False,peek)
    
    # write high addr
    temp_reg.write32(4,high)
    tran1.set_write()
    tran1.set_data_length(4)
    tran1.set_streaming_width(4)
    tran1.set_address(0x84)
    tran1.set_data_ptr_with_memory(temp_reg,4)
    user_bar.put_input(tran1,False,peek)
    
    # write head counter
    temp_reg.write32(8,2)
    tran1.set_write()
    tran1.set_data_length(4)
    tran1.set_streaming_width(4)
    tran1.set_address(0x88)
    tran1.set_data_ptr_with_memory(temp_reg,8)
    user_bar.put_input(tran1,False,peek)
    
dsc_bypass_h2c.register_b_transport(dsc_bypass_h2c_b_transport)
dsc_bypass_c2h.register_b_transport(dsc_bypass_c2h_b_transport)
c2h_data.register_b_transport(c2h_data_b_transport)
write_base_addr()

start()


