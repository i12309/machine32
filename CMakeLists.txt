cmake_minimum_required(VERSION 3.16)
project(machine32 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(SCREENLIB_ROOT ${CMAKE_CURRENT_LIST_DIR}/../screenLIB)
set(SCREENUI_ROOT ${CMAKE_CURRENT_LIST_DIR}/../ScreenUI)

add_executable(machine32_backend_demo
    src/backend_main.cpp
)

target_include_directories(machine32_backend_demo PRIVATE
    ${SCREENLIB_ROOT}/lib/core/src
    ${SCREENLIB_ROOT}/lib/host/src
    ${SCREENUI_ROOT}/generated/shared
    ${SCREENUI_ROOT}
)
