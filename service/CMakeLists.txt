add_library(
    service
    STATIC
    src/service/mailservice.hpp
    src/service/mailservice.cpp
    src/service/storageservice.hpp
    src/service/storageservice.cpp
    src/service/userservice.hpp
    src/service/userservice.cpp
    src/service/adminservice.hpp
    src/service/adminservice.cpp
    src/service/dispatchservice.hpp
    src/service/dispatchservice.cpp
    src/service/cookieservice.cpp
    src/service/cookieservice.hpp
    src/service/staticservice.cpp
    src/service/staticservice.hpp
)

target_link_libraries(
    service
    utils
    storeclient
    SMTP
    network
)

target_include_directories(
    service
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_executable(servicetest servicetest.cpp)

target_link_libraries(servicetest PRIVATE service)