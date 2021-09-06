
# drm 库， 这个libdrm.so从这个位置获取, 如果其他地址链接是一直提示某些函数缺失。
buildroot/output/rockchip_rk3568/build/camera_engine_rkaiq-1.0/tests/rkisp_demo/demo/libs/arm64/libdrm.so

# libmail库有wayland,和bgm相关的，我们想要使用drm直接显示，使用带bgm的。
