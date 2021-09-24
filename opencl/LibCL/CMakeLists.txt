add_executable( TestBlend TestBlend.cpp LibCL.cpp )
target_link_libraries( TestBlend ${OPENCL_LIBRARIES} easymedia /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libstdc++.so.6.0.28) 

add_executable( TestBlend_1d TestBlend_1d.cpp LibCL.cpp )
target_link_libraries( TestBlend_1d ${OPENCL_LIBRARIES} easymedia /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libstdc++.so.6.0.28) 

add_executable( TestRotate TestRotate.cpp LibCL.cpp )
target_link_libraries( TestRotate ${OPENCL_LIBRARIES} easymedia /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libstdc++.so.6.0.28) 

add_executable( TestPerformance TestPerformance.cpp LibCL.cpp )
target_link_libraries( TestPerformance ${OPENCL_LIBRARIES} easymedia pthread /data/works/rk3568/rk356x-linux-20210809/buildroot/output/rockchip_rk3568/host/aarch64-buildroot-linux-gnu/sysroot/usr/lib/libstdc++.so.6.0.28) 


configure_file(LibCL.cl ${CMAKE_CURRENT_BINARY_DIR}/LibCL.cl COPYONLY)
