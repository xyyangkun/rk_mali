include_directories(
	.
	../libsrc/
	../include/libdrm/
	../include/rockchip
	../include/rga
	)

link_directories(
	/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/target/usr/lib/
	/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/target/lib/
	#/data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/
	)

AUX_SOURCE_DIRECTORY(../libsrc/ _DRMDSP_DIR_SRCS1)
AUX_SOURCE_DIRECTORY(../libsrc/drmDsp _DRMDSP_DIR_SRCS2)
list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS1})
list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS2})

add_executable(test_usb_mipi test_usb_mipi.c my_usb_camera.c  ringqueue.c ${_DIR_SRCS})
target_link_libraries(test_usb_mipi rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(test_usb_hdmi test_usb_hdmi.c my_usb_camera.c  ringqueue.c ${_DIR_SRCS})
target_link_libraries(test_usb_hdmi rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(test_usb_mipi_rgb8888_out test_usb_mipi_rgb8888_out.c my_usb_camera.c  ringqueue.c ${_DIR_SRCS})
target_link_libraries(test_usb_mipi_rgb8888_out rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(test_usb_hdmi_rgb8888_out test_usb_hdmi_rgb8888_out.c my_usb_camera.c  ringqueue.c ${_DIR_SRCS})
target_link_libraries(test_usb_hdmi_rgb8888_out rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(modeset-plane-rotate modeset-plane-rotate.c )
target_link_libraries(modeset-plane-rotate rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)

add_executable(modeset-plane-set modeset-plane-set.c )
target_link_libraries(modeset-plane-set rga drm pthread z v4l2 v4l2rds  rockchip_mpp -lm)
