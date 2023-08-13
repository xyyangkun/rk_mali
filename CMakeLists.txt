# CMake 最低版本号要求
cmake_minimum_required (VERSION 2.8)
set(CMAKE_CXX_STANDARD 11)
# 项目信息
project (osee_mali)
# 指定生成目标




SET(CMAKE_SYSTEM_PROCESSOR "arm")
                                                                                
set(TOOLCHAIN_DIR  "/data/works/rk3568/rk3568_v1_2_4/buildroot/output/rockchip_rk3568/host")
#set(TOOLCHAIN_DIR  "/data/works/rk3568/rk356x_v1_2_3//buildroot/output/rockchip_rk3568/host")
#set(TOOLCHAIN_DIR  "/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host")
                                                                                
set(SYSROOT_PATH  ${TOOLCHAIN_DIR}/aarch64-buildroot-linux-gnu/sysroot)         
set(CMAKE_SYSROOT "${SYSROOT_PATH}")                                            
                                                                                
SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-gcc)                    
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/aarch64-linux-g++)                  
SET(CMAKE_FIND_ROOT_PATH  ${SYSROOT_PATH})

# 指定安装路径
#install (TARGETS osee_mali DESTINATION bin)


# 指定生成目标
add_executable(osee_mali mali_test.c)

set(BUILDROOT_BUILD_PROJECT true)

#/data/works/rk3568/rk356x-linux-20210809/external/osee_mail/build$ /data/works/rk3568/rk356x-linux-20210809/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-g++ ../test/HelloWorld.cpp  -I ../../libmali/include/ -L ../../libmali/lib/aarch64-linux-gnu/ -lmali-bifrost-g52-g2p0-wayland

set(PLATFORM_NAME rk3568)

# 依赖库
# 指定生成目标
include_directories(
	${PROJECT_SOURCE_DIR}/include/
	#${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/usr/include
	#${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/easymedia/
	#${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/build/rkmedia/include/rkmedia/
	${SYSROOT_PATH}/usr/include/easymedia
)


get_filename_component(ABSOLUTE_PATH1 ${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/lib ABSOLUTE) 
get_filename_component(ABSOLUTE_PATH2 ${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib ABSOLUTE) 

link_directories(
	#${PROJECT_SOURCE_DIR}/../libmali/lib/aarch64-linux-gnu
	#${PROJECT_SOURCE_DIR}/../../buildroot/target/lib
	#${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_rk3568/host/usr/lib
	#${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_rk3568/host/lib
	${PROJECT_SOURCE_DIR}/libs/
	#${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib

	#${ABSOLUTE_PATH1}
	#${ABSOLUTE_PATH2}
)


#set(OPENCL_LIBRARIES mali-bifrost-g52-g2p0 )
#set(OPENCL_LIBRARIES mali-bifrost-g52-g2p0-gbm)
#set(OPENCL_LIBRARIES mali-bifrost-g52-r25p0-dummy)
#set(OPENCL_LIBRARIES mali-bifrost-g52-g2p0-wayland)
#set(OPENCL_LIBRARIES mali-bifrost-g52-g2p0-wayland drm wayland-client wayland-server m c)
#set(OPENCL_LIBRARIES mali drm wayland-client wayland-server m c)
set(OPENCL_LIBRARIES mali-bifrost-g52-g2p0-dummy)



add_subdirectory(opencl)
add_subdirectory(opengl)
add_subdirectory(test_usb)
add_subdirectory(drmout)
