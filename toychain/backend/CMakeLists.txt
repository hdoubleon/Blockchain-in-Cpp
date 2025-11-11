cmake_minimum_required(VERSION 3.16)
project(toychain_backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(toychain_lib
    src/transaction.cpp
    src/block.cpp
    src/blockchain.cpp
)
target_include_directories(toychain_lib PUBLIC include)

add_executable(toychain src/main.cpp)
target_link_libraries(toychain PRIVATE toychain_lib)
