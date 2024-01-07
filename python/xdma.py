import asyncio
from libdistrosim import init,start,stop,xdma_top,tlm_generic_payload,memory

# write 0x80,0x84 with the base addr
# and then write the head counter 0x88 with 2
async def write_base_addr(user_bar, fake_memory, temp_reg):
    low = fake_memory.get_raw_addr() & 0xffffffff
    high = fake_memory.get_raw_addr() >> 32

    # write low addr
    temp_reg.write32(0, low)
    tran1 = tlm_generic_payload()
    tran1.set_write()
    tran1.set_data_length(4)
    tran1.set_streaming_width(4)
    tran1.set_address(0x80)
    tran1.set_data_ptr_with_memory(temp_reg, 0)
    _is_blocking = False
    status = await user_bar.put_stimuli(tran1, _is_blocking)
    print(status)

    # write high addr
    temp_reg.write32(4, high)
    tran1.set_write()
    tran1.set_data_length(4)
    tran1.set_streaming_width(4)
    tran1.set_address(0x84)
    tran1.set_data_ptr_with_memory(temp_reg, 4)
    _is_blocking = False
    status = await user_bar.put_stimuli(tran1, _is_blocking)
    print(status)

    # write head counter
    temp_reg.write32(8, 2)
    tran1.set_write()
    tran1.set_data_length(4)
    tran1.set_streaming_width(4)
    tran1.set_address(0x88)
    tran1.set_data_ptr_with_memory(temp_reg, 8)
    _is_blocking = False
    status = user_bar.put_stimuli(tran1, _is_blocking)
    print(status)

# dsc_bypass_h2c.register_b_transport(dsc_bypass_h2c_b_transport)
# dsc_bypass_c2h.register_b_transport(dsc_bypass_c2h_b_transport)
# c2h_data.register_b_transport(c2h_data_b_transport)


async def main():
    loop = asyncio.get_event_loop()
    init(loop)
    top = xdma_top.get_top()
    user_bar = top.py_user_bar
    fake_memory = memory(4096, True)  # align to 4K
    temp_reg = memory(16, False)
    start()
    await write_base_addr(user_bar, fake_memory, temp_reg)

asyncio.run(main())
