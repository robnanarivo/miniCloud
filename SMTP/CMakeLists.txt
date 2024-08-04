add_library(
    SMTP
    STATIC
    src/SMTP/smtpclient.hpp
    src/SMTP/smtpclient.cpp
    src/SMTP/smtpserver.hpp
    src/SMTP/smtpserver.cpp
    src/SMTP/mailbox.hpp
    src/SMTP/mailbox.cpp
)

target_link_libraries(
    SMTP
    utils
    network
    storeclient
    service
    -lresolv
)

target_include_directories(
    SMTP
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_executable(smtptest smtptest.cpp)

target_link_libraries(smtptest PRIVATE SMTP)

add_executable(smtpserver src/SMTP/smtpserver.cpp)

target_link_libraries(smtpserver PRIVATE SMTP)