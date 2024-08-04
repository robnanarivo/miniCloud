find_package(nlohmann_json 3.2.0 REQUIRED)

add_library(
    controller
    STATIC
    src/controller/controller.hpp
    src/controller/controller.cpp
    src/controller/dummycontroller.hpp
    src/controller/dummycontroller.cpp
    src/controller/fallbackcontroller.hpp
    src/controller/fallbackcontroller.cpp
    src/controller/dispatchcontroller.hpp
    src/controller/dispatchcontroller.cpp
    src/controller/staticcontroller.hpp
    src/controller/staticcontroller.cpp
    src/controller/mailcontroller.hpp
    src/controller/mailcontroller.cpp
    src/controller/storagecontroller.hpp
    src/controller/storagecontroller.cpp
    src/controller/admincontroller.hpp
    src/controller/admincontroller.cpp
    src/controller/usercontroller.hpp
    src/controller/usercontroller.cpp
)

target_link_libraries(
    controller
    utils
    network
    service
    nlohmann_json::nlohmann_json
)

target_include_directories(
    controller
    PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)