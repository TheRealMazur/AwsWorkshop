# AwsWorkshop
## Prerequisites
Make sure you have the following packages installed first:
1. CMake (version 3.9 or later)
1. git
1. Make or Ninja
1. zip
1. libcurl-devel (on Debian-basded distros it's libcurl4-openssl-dev)
## Building dependencies:
**Warning: pay attention to CMAKE_INSTALL_PREFIX**<br/>
By default all scripts use **~/install** directory to store compiled dependencies.
### Build the AWS C++ SDK
Start by building the SDK from source.
```bash
$ mkdir ~/install
$ git clone https://github.com/aws/aws-sdk-cpp.git
$ cd aws-sdk-cpp
$ mkdir build
$ cd build
$ cmake .. -DBUILD_ONLY="dynamodb; s3" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DENABLE_UNITY_BUILD=ON \
  -DCUSTOM_MEMORY_MANAGEMENT=OFF \
  -DCMAKE_INSTALL_PREFIX=~/install \
  -DENABLE_UNITY_BUILD=ON

$ make -j $(($numproc-1))
$ make install
```

### Build the Runtime
Now let's build the C++ Lambda runtime, so in a separate directory clone this repository and follow these steps:

```bash
$ git clone https://github.com/awslabs/aws-lambda-cpp-runtime.git
$ cd aws-lambda-cpp-runtime
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_PREFIX=~/install \
$ make -j $(($numproc-1))
$ make install
```
## To deploy ready to use lambdas:
1. Build lambda packages using build scripts
1. Upload ready .zip files to AWS Lambda