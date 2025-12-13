cmake_minimum_required(VERSION 3.18)

add_subdirectory(tools)

set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)

project(RPMegaChopper C CXX ASM)

add_executable(RPMegaChopper)
rp6502_executable(RPMegaChopper
    DATA file
    RESET file
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.hlp
)
target_sources(RPMegaChopper PRIVATE
    src/main.c
)
