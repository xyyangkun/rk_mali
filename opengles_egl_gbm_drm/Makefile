CC = /data/works/rk3568/rk356x-linux-20210809/prebuilts/gcc/linux-x86/aarch64/gcc-linaro-6.3.1-2017.05-x86_64_aarch64-linux-gnu/bin/aarch64-linux-gnu-gcc
.PHONY: clean


CFLAGS=-I . -I ../include -I../libsrc -I../include/GBM -I../include/libdrm
LDFLAGS= -Wl,-rpath,../libs/ -ldrm -lmali-bifrost-g52-g2p0-gbm

ta_objs=ta/ta.o ta/ta_utils.o ta/ta_talloc.o
drm_objs=drm/drm_common.o drm/drm_atomic.o drm/drm_log.o
main_objs=main.o egl.o

egl_drm: ${ta_objs} ${drm_objs} ${main_objs}
	${CC} -o $@ $^ ${LDFLAGS} 

%.o:%.c
	${CC} -std=gnu99 -g -c ${CFLAGS} -o $@ $^

clean:
	rm -rf ${ta_objs} ${drm_objs} ${main_objs}

