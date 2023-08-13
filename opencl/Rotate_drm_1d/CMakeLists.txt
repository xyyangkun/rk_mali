add_executable( Rotate_drm_1d Rotate_drm_1d.cpp )
target_link_libraries( Rotate_drm_1d ${OPENCL_LIBRARIES} easymedia)

configure_file(Rotate_drm_1d.cl ${CMAKE_CURRENT_BINARY_DIR}/Rotate_drm_1d.cl COPYONLY)
