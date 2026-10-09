cmake_minimum_required(VERSION 3.20)
project(Mills LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Mills
    main.cpp
    NineMensMorris.h
        Minimax.h
)

target_link_libraries(Mills PRIVATE Qt6::Widgets)
