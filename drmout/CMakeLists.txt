include_directories(
	.
	../include/
	../include/libdrm
	../include/rga/
	)

link_directories(
	#/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/target/usr/lib/
	#/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/target/lib/
	#/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/
	)

AUX_SOURCE_DIRECTORY(. _DRMDSP_DIR_SRCS1)
list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS1})

add_executable(drm_out drm_out.c ${_DIR_SRCS})
target_link_libraries(drm_out rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)
