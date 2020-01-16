mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=~/install
make -j 4
make aws-lambda-package-AddProduct
