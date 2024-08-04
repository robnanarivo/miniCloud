set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(
    httpserver
    STATIC
    src/httpserver/httpserver.hpp
    src/httpserver/httpserver.cpp
    src/httpserver/router.hpp
    src/httpserver/router.cpp
    src/httpserver/threadpool.hpp
    src/httpserver/threadpool.cpp
)

target_link_libraries(httpserver Threads::Threads network controller)

target_include_directories(httpserver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
