add_library(
    storeclient
    STATIC
    src/storeclient/storeclient.cpp
    src/storeclient/storeclient.hpp
)

target_link_libraries(
    storeclient
    utils
    network
)

target_include_directories(
    storeclient
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
