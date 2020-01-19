#!/bin/bash
mkdir -p ~/install
git clone https://github.com/aws/aws-sdk-cpp.git
cd aws-sdk-cpp
mkdir build
cd build
cmake .. -DBUILD_ONLY="dynamodb;s3" \
-DCMAKE_BUILD_TYPE=Release \
-DBUILD_SHARED_LIBS=OFF \
-DENABLE_UNITY_BUILD=ON \
-DCUSTOM_MEMORY_MANAGEMENT=OFF \
-DCMAKE_INSTALL_PREFIX=~/install \
-DENABLE_UNITY_BUILD=ON
make -j $(($(nproc)-1))
make install