set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(
    storeserver
    STATIC
    src/address.h
    src/master_node.h
    src/parser.cpp
    src/parser.h
    src/request_queue.cpp
    src/request_queue.h
    src/request.h
    src/socket_reader_writer.cpp
    src/socket_reader_writer.h
    src/tablet_node.cpp
    src/tablet_node.h
    src/tablet_worker.cpp
    src/tablet_worker.h
    src/task_queue.cpp
    src/task_queue.h
    src/db_logger.cpp
    src/db_logger.h
    src/tablet_worker_primary.cpp
    src/tablet_worker_primary.h
    src/tablet_worker_secondary.cpp
    src/tablet_worker_secondary.h
    src/tester_rcv.cpp
    src/tester_snd.cpp
)

add_executable(tablet_server src/tablet_server.cpp)
add_executable(master_server src/master_server.cpp)
add_executable(tester_rcv src/tester_rcv.cpp)
add_executable(tester_snd src/tester_snd.cpp)
add_executable(tester_crz src/tester_crz.cpp)


target_link_libraries(tablet_server Threads::Threads OpenSSL::SSL storeserver)
target_link_libraries(master_server Threads::Threads OpenSSL::SSL storeserver)
target_link_libraries(tester_rcv Threads::Threads storeserver)
target_link_libraries(tester_snd Threads::Threads storeserver)
target_link_libraries(tester_crz Threads::Threads storeserver)