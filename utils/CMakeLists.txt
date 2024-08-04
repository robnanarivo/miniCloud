set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

add_library(
    utils
    STATIC
    src/utils/utils.hpp
    src/utils/utils.cpp
)

target_link_libraries(
    utils
    -lpthread
    OpenSSL::Crypto
)

target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
