cmake_minimum_required(VERSION 3.10)
project(ToyChain)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)

add_executable(toychain_server
    src/main.cpp
    src/block.cpp
    src/transaction.cpp
    src/blockchain.cpp
    src/server.cpp
)

target_include_directories(toychain_server PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(toychain_server ${OPENSSL_LIBRARIES})
