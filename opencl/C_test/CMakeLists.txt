project(c_test)

#add_executable(t t.c )
#target_link_libraries(t ${OPENCL_LIBRARIES} )
#configure_file(HelloWorld.cl ${CMAKE_CURRENT_BINARY_DIR}/HelloWorld.cl COPYONLY)

add_executable(c_rotate c_rotate.c )
add_executable(c_blend c_blend.c )



