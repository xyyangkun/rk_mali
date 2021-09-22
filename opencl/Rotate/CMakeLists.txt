add_executable( Rotate Rotate.cpp )
target_link_libraries( Rotate ${OPENCL_LIBRARIES} )

configure_file(Rotate.cl ${CMAKE_CURRENT_BINARY_DIR}/Rotate.cl COPYONLY)
