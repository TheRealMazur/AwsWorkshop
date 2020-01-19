#!/bin/bash
mkdir -p ~/install
git clone https://github.com/awslabs/aws-lambda-cpp.git
cd aws-lambda-cpp
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
-DBUILD_SHARED_LIBS=OFF \
-DCMAKE_INSTALL_PREFIX=~/install
make -j $(($(nproc)-1))
make install