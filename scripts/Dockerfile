# Copyright (C) 2024 BEIJING DATENLORD TECHNOLOGY CO., LTD 
# Licensed under the Apache License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. 
# You may obtain a copy of the License at 
# http://www.apache.org/licenses/LICENSE-2.0. 
# Unless required by applicable law or agreed to in writing, software 
# distributed under the License is distributed on an "AS IS" BASIS, 
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
# See the License for the specific language governing permissions 
# and limitations under the License.


# Choose ubuntu 20.04 as base image
FROM ubuntu:20.04

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive KERNEL_VERSION=linux-headers-5.4.0-64-generic TEMP_FILE_PATH=/tmp/driver

# Update the source list and install required packages
RUN apt update && \
    apt install -y git make gcc libaio1 libaio-dev kmod $KERNEL_VERSION

# Clone the QDMA driver
RUN git clone https://github.com/Xilinx/dma_ip_drivers.git

# Build the driver
WORKDIR dma_ip_drivers/QDMA/linux-kernel
RUN make TANDEM_BOOT_SUPPORTED=1 KDIR=/usr/src/$KERNEL_VERSION && \
    mkdir -p $TEMP_FILE_PATH

# Move the built driver to the temp path
CMD mv /dma_ip_drivers $TEMP_FILE_PATH
