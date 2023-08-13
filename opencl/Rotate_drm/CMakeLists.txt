add_executable( Rotate_drm Rotate_drm.cpp )
target_link_libraries( Rotate_drm ${OPENCL_LIBRARIES} easymedia)

configure_file(Rotate_drm.cl ${CMAKE_CURRENT_BINARY_DIR}/Rotate_drm.cl COPYONLY)
