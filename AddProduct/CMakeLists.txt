cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 11)
project(AddProduct LANGUAGES CXX)

find_package(aws-lambda-runtime REQUIRED)
find_package(AWSSDK COMPONENTS dynamodb)

add_executable(${PROJECT_NAME} "main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE AWS::aws-lambda-runtime ${AWSSDK_LINK_LIBRARIES})
target_compile_options(${PROJECT_NAME} PRIVATE
    "-Wall"
    "-Wextra"
    "-Wconversion"
    "-Wshadow"
    "-Wno-sign-conversion")

target_compile_features(${PROJECT_NAME} PRIVATE "cxx_std_11")
aws_lambda_package_target(${PROJECT_NAME})
