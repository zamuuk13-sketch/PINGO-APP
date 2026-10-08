cmake_minimum_required(VERSION 3.20)

project(PingoApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PingoApp WIN32
    src/main.cpp
)

target_compile_definitions(PingoApp PRIVATE UNICODE _UNICODE)

target_link_libraries(PingoApp PRIVATE Version Shell32)

if (MSVC)
    target_compile_options(PingoApp PRIVATE /W4)
endif()
