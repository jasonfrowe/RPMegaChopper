cmake_minimum_required(VERSION 3.18)

add_subdirectory(tools)

set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)

project(RPMegaChopper C CXX ASM)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(RPMegaChopper)
rp6502_asset(RPMegaChopper 0x10000 images/Chopper.bin)
rp6502_asset(RPMegaChopper 0x15800 images/Tiles_Ground.bin)
rp6502_asset(RPMegaChopper 0x15D00 images/Cloud_A.bin)
rp6502_asset(RPMegaChopper 0x16500 images/Cloud_B.bin)
rp6502_asset(RPMegaChopper 0x16D00 images/Cloud_C.bin)
rp6502_asset(RPMegaChopper 0x16F00 images/LandingPad.bin)
rp6502_asset(RPMegaChopper 0x18100 images/HomeBase.bin)
rp6502_asset(RPMegaChopper 0x18D00 images/EnemyBase.bin)
rp6502_asset(RPMegaChopper 0x1A500 images/Flags.bin)
rp6502_asset(RPMegaChopper 0x1A900 images/Hostages.bin)
rp6502_asset(RPMegaChopper 0x1BD00 images/bullet.bin)
rp6502_asset(RPMegaChopper 0x1BD08 images/Explosion.bin)
rp6502_asset(RPMegaChopper 0x1D108 images/SmallExplode.bin)
rp6502_asset(RPMegaChopper 0x1D488 images/Tank.bin)
rp6502_asset(RPMegaChopper 0x1DF88 images/Boom.bin)
rp6502_asset(RPMegaChopper 0x1E388 images/Balloon.bin)
rp6502_asset(RPMegaChopper 0x1EF88 images/Jet.bin)
rp6502_asset(RPMegaChopper 0x1F188 images/Bomb.bin)
rp6502_asset(RPMegaChopper 0x1F208 images/Minichopper.bin)
rp6502_asset(RPMegaChopper help src/main.hlp)
rp6502_executable(RPMegaChopper
    DATA file
    RESET file
)
target_sources(RPMegaChopper PRIVATE
    src/main.c
    src/input.c
    src/player.c
    src/clouds.c
    src/landing.c
    src/homebase.c
    src/flags.c
    src/enemybase.c
    src/bullets.c
    src/hostages.c
    src/hud.c
    src/explosion.c
    src/smallexplosion.c
    src/tanks.c
    src/ebullets.c
    src/bomb.c
    src/balloon.c
    src/jet.c
    src/sound.c
    src/music.c
    src/highscore.c
    src/boom.c
)
