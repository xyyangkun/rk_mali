add_executable( Rotate_drm_1d Rotate_drm_1d.cpp )
target_link_libraries( Rotate_drm_1d ${OPENCL_LIBRARIES} easymedia /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libstdc++.so.6.0.28) 

configure_file(Rotate_drm_1d.cl ${CMAKE_CURRENT_BINARY_DIR}/Rotate_drm_1d.cl COPYONLY)
