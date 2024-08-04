add_library(
    network
    STATIC
    src/network/address.hpp
    src/network/address.cpp
    src/network/datachunk.hpp
    src/network/datachunk.cpp
    src/network/tcpsocket.hpp
    src/network/udpsocket.hpp
    src/network/tcpsocket.cpp
    src/network/udpsocket.cpp
    src/network/httprequest.hpp
    src/network/httprequest.cpp
    src/network/httpresponse.hpp
    src/network/httpresponse.cpp
    src/network/networkinfo.cpp
    src/network/networkinfo.hpp
    src/network/staticfile.cpp
    src/network/staticfile.hpp
)

target_link_libraries(network utils)

target_include_directories(network PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
