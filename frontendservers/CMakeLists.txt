add_executable(dispatcher dispatcher.cpp)
add_executable(frontendserver frontendserver.cpp)

target_link_libraries(dispatcher httpserver controller)
target_link_libraries(frontendserver httpserver controller)

install(TARGETS dispatcher DESTINATION bin)
