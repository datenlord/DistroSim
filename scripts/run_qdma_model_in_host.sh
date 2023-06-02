#!/bin/bash
set -o errexit
set -o nounset
set -o xtrace

TEMP_FILE_PATH="/tmp/machine-x86-qdma-demo"
DEMO_PATH="/home/${USER}/cosim_demo"
SYSTEMC_VERSION=systemc-2.3.3
SYSTEMC_PATH="${DEMO_PATH}/${SYSTEMC_VERSION}"

# Install the systemc-2.3.3 as dependency
mkdir -p $DEMO_PATH
pushd $DEMO_PATH
wget -q https://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.3.tar.gz &&                                                                                                                                                     
tar xzf systemc-2.3.3.tar.gz
cd $SYSTEMC_PATH
./configure --prefix=$SYSTEMC_PATH
make -j
make install
popd

# Build the cpm5-qdma-demo and cpm4-qdma-demo
git submodule update --init libsystemctlm-soc
git submodule update --init pcie-model
cat <<EOF > .config.mk
SYSTEMC = ${SYSTEMC_PATH}
EOF
cat .config.mk
make pcie/versal/cpm5-qdma-demo pcie/versal/cpm4-qdma-demo

# Launch the cpm5-qdma-demo or the cpm4-qdma-demo (choose one)
mkdir -p $TEMP_FILE_PATH
LD_LIBRARY_PATH=${SYSTEMC_PATH}/lib-linux64/ ./pcie/versal/cpm5-qdma-demo unix:${TEMP_FILE_PATH}/qemu-rport-_machine_peripheral_rp0_rp 10000 &
# or
# LD_LIBRARY_PATH=$SYSTEMC_PATH/lib-linux64/ ./pcie/versal/cpm4-qdma-demo unix:${TEMP_FILE_PATH}/qemu-rport-_machine_peripheral_rp0_rp 10000 &
