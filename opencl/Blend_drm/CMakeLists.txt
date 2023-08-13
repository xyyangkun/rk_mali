add_executable( Blend_drm Blend_drm.cpp )
#target_link_libraries( Blend_drm ${OPENCL_LIBRARIES} easymedia /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libstdc++.so.6.0.28) 
target_link_libraries( Blend_drm ${OPENCL_LIBRARIES} easymedia)

configure_file(Blend_drm.cl ${CMAKE_CURRENT_BINARY_DIR}/Blend_drm.cl COPYONLY)
