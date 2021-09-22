
#source directory
#AUX_SOURCE_DIRECTORY(drmDsp _DRMDSP_DIR_SRCS)
#list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS})
#list(APPEND RGA_SRCS rgaCopy.cpp  RockchipFileOps.cpp )

#add_executable(rgaCopy ${RGA_SRCS})
#target_link_libraries(rgaCopy rga drm pthread)


#include_directories(
#	${PROJECT_SOURCE_DIR}/../libmali/include/GBM/
#	${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/usr/include
#	${PROJECT_SOURCE_DIR}/../../buildroot/output/rockchip_${PLATFORM_NAME}/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/drm
#)

#add_executable(glsample_gbm main_gbm.cpp gbm_wrapper.c )
add_executable(glsample_gbm main_gbm.cpp)
target_include_directories(glsample_gbm PUBLIC ${PROJECT_SOURCE_DIR}/include/GBM/)
target_link_libraries(glsample_gbm  mali-bifrost-g52-g2p0-gbm drm)

include_directories(
	.
	../libsrc/
	../include/libdrm/
	../include/rockchip
	../include/rga
	)


AUX_SOURCE_DIRECTORY(../libsrc/ _DRMDSP_DIR_SRCS1)
AUX_SOURCE_DIRECTORY(../libsrc/drmDsp _DRMDSP_DIR_SRCS2)
list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS1})
list(APPEND _DIR_SRCS ${_DRMDSP_DIR_SRCS2})

add_executable(glsample_drm glsample_drm.cpp ${_DIR_SRCS})
target_include_directories(glsample_drm PUBLIC ${PROJECT_SOURCE_DIR}/include/GBM/)
target_link_libraries(glsample_drm mali-bifrost-g52-g2p0-gbm rga drm pthread z v4l2 v4l2rds  rockchip_mpp)

add_executable(drm-gbm drm-gbm.c ${_DIR_SRCS})
target_include_directories(drm-gbm PUBLIC ${PROJECT_SOURCE_DIR}/include/GBM/)
target_link_libraries(drm-gbm mali-bifrost-g52-g2p0-gbm rga drm pthread z v4l2 v4l2rds  rockchip_mpp)



