add_executable( Blend Blend.cpp )
target_link_libraries( Blend ${OPENCL_LIBRARIES} )

configure_file(Blend.cl ${CMAKE_CURRENT_BINARY_DIR}/Blend.cl COPYONLY)
