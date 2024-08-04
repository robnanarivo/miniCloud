cmake_minimum_required(VERSION 3.16)
project(PennCloud VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(utils)
add_subdirectory(network)
add_subdirectory(controller)
add_subdirectory(storeclient)
add_subdirectory(SMTP)
add_subdirectory(httpserver)
add_subdirectory(service)
add_subdirectory(storeserver)
add_subdirectory(frontendservers)