FROM ubuntu:22.04
RUN apt-get update && apt-get install -y wget git cloud-image-utils libfdt1 unzip g++ make
ARG DEMO_PATH="/workspaces/cosim_demo"
ARG SYSTEMC_VERSION=systemc-2.3.3
ARG SYSTEMC_PATH="${DEMO_PATH}/${SYSTEMC_VERSION}"
# get clanged lsp
RUN wget https://github.com/clangd/clangd/releases/download/16.0.2/clangd-linux-16.0.2.zip && \
    unzip clangd-linux-16.0.2.zip && \
    mv clangd_16.0.2/bin/* /bin/ &&  \
    mv clangd_16.0.2/lib/* /lib/ && \
    rm -r clangd_16.0.2 && rm clangd-linux-16.0.2.zip
RUN mkdir -p $DEMO_PATH && cd $DEMO_PATH && wget -q https://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.3.tar.gz && \
    tar xzf systemc-2.3.3.tar.gz && cd $SYSTEMC_PATH && ./configure --prefix=$SYSTEMC_PATH && \
    make -j && make install
