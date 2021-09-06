
#source directory
#AUX_SOURCE_DIRECTORY(drmDsp _DRMDSP_DIR_SRCS)
#list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS})
#list(APPEND RGA_SRCS rgaCopy.cpp  RockchipFileOps.cpp )

#add_executable(rgaCopy ${RGA_SRCS})
#target_link_libraries(rgaCopy rga drm pthread)


add_executable(HelloWorld HelloWorld.cpp )
target_link_libraries(HelloWorld ${OPENCL_LIBRARIES} )
configure_file(HelloWorld.cl ${CMAKE_CURRENT_BINARY_DIR}/HelloWorld.cl COPYONLY)



