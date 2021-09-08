
cmake_minimum_required( VERSION 2.6.3 )

SET(CMAKE_SYSTEM_NAME Linux)
#SET(CMAKE_C_COMPILER "arm-linux-gnueabihf-gcc")
#SET(CMAKE_CXX_COMPILER "arm-linux-gnueabihf-g++")
#SET(CMAKE_C_COMPILER "/data/works/rk3568/rk356x-linux-20210809/prebuilts/gcc/linux-x86/aarch64/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu/bin/aarch64-linux-gcc")
#SET(CMAKE_CXX_COMPILER  "/data/works/rk3568/rk356x-linux-20210809/prebuilts/gcc/linux-x86/aarch64/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu/bin/aarch64-linux-g++")
SET(CMAKE_C_COMPILER    "/data/works/rk3568/rk356x-linux-20210809/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER  "/data/works/rk3568/rk356x-linux-20210809/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++")
#SET(CMarch64E_SYSTEM_PROCESSOR "armv7-a")
#SET(CMAKE_SYSTEM_PROCESSOR "armv7-a_hardfp")
SET(CMAKE_SYSTEM_PROCESSOR "armv8-a")

set(SYSROOT_PATH  /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot)

set(CMAKE_SYSROOT "${SYSROOT_PATH}")
set(CMAKE_SYSROOT "${SYSROOT_PATH}")

add_definitions(-fPIC)
add_definitions(-DARMLINUX)
add_definitions(-D__gnu_linux__)
