add_executable( TestBlend TestBlend.cpp LibCL.cpp )
target_link_libraries( TestBlend ${OPENCL_LIBRARIES} easymedia )

add_executable( TestBlend_1d TestBlend_1d.cpp LibCL.cpp )
target_link_libraries( TestBlend_1d ${OPENCL_LIBRARIES} easymedia)

add_executable( TestRotate TestRotate.cpp LibCL.cpp )
target_link_libraries( TestRotate ${OPENCL_LIBRARIES} easymedia)

add_executable( TestPerformance TestPerformance.cpp LibCL.cpp )
target_link_libraries( TestPerformance ${OPENCL_LIBRARIES} easymedia pthread)


configure_file(LibCL.cl ${CMAKE_CURRENT_BINARY_DIR}/LibCL.cl COPYONLY)
