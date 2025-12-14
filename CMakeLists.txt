cmake_minimum_required(VERSION 3.18)

add_subdirectory(tools)

set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)

project(RPMegaChopper C CXX ASM)

add_executable(RPMegaChopper)
rp6502_asset(RPMegaChopper 0x10000 images/Chopper.bin)
rp6502_asset(RPMegaChopper 0x15800 images/Tiles_Ground.bin)
rp6502_asset(RPMegaChopper 0x15D00 images/Cloud_A.bin)
rp6502_asset(RPMegaChopper 0x16500 images/Cloud_B.bin)
rp6502_asset(RPMegaChopper 0x16D00 images/Cloud_C.bin)
rp6502_executable(RPMegaChopper
    Chopper.bin.rp6502
    Tiles_Ground.bin.rp6502
    Cloud_A.bin.rp6502
    Cloud_B.bin.rp6502
    Cloud_C.bin.rp6502
    DATA file
    RESET file
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.hlp
)
target_sources(RPMegaChopper PRIVATE
    src/main.c
    src/input.c
    src/player.c
)
