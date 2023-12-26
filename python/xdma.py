import libxdma
libxdma.init()
libxdma.start()
tran = libxdma.tlm_generic_payload()
delay = libxdma.sc_time(libxdma.SC_ZERO_TIME)
libxdma.get_top().init.b_transport(tran,delay)