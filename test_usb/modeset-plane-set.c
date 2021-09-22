/*
 * Author: Leon.He
 * e-mail: 343005384@qq.com
 */

#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>


struct buffer_object {
	uint32_t width;
	uint32_t height;
	uint32_t pitch;
	uint32_t handle;
	uint32_t size;
	uint8_t *vaddr;
	uint32_t fb_id;
};

struct property_arg {
	uint32_t obj_id;
	uint32_t obj_type;
	char name[DRM_PROP_NAME_LEN+1];
	uint32_t prop_id;
	uint64_t value;
};


struct buffer_object buf;
struct buffer_object plane_buf[7];


static void write_color(struct buffer_object *bo,unsigned int color)
{
	unsigned int *pt;
	int i;
	
	pt = bo->vaddr;
	for (i = 0; i < (bo->size / 4); i++){
		*pt = color;
		pt++;
	}	
	
}
static void write_color_half(struct buffer_object *bo,unsigned int color1,unsigned int color2)
{
	unsigned int *pt;
	int i,size;
	
	pt = bo->vaddr;
	size = bo->size/4/5;
	for (i = 0; i < (size); i++){
		*pt = (139<<16)|255;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = 255;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = (127<<8)| 255;
		pt++;
	}
	for (i = 0; i < (size); i++){
		*pt = 255<<8;
		pt++;
	}
	for (i = 0; i < (size); i++){
		*pt = (255<<16) | (255<<8);
		pt++;
	}
	
}

static void write_color3(struct buffer_object *bo,unsigned int color1,unsigned int color2, unsigned int color3)
{
	unsigned int *pt;
	int i,size;
	
	pt = bo->vaddr;
	size = bo->size/4/5;
	for (i = 0; i < (size); i++){
		*pt = 0x00000000;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = color1;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = color2;
		pt++;
	}	
	for (i = 0; i < (size); i++){
		*pt = color3;
		pt++;
	}
	for (i = 0; i < (size); i++){
		*pt = 0x00ffffff;
		pt++;
	}
}

static int modeset_create_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	int ret;

	

	create.width = bo->width;
	create.height = bo->height;
	create.bpp = 32;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;


	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

	bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);




#if 0
	drmModeAddFB(fd, bo->width, bo->height, 24, 32, bo->pitch,
			   bo->handle, &bo->fb_id);
#else
	offsets[0] = 0;
	handles[0] = bo->handle;
	pitches[0] = bo->pitch;

	ret = drmModeAddFB2(fd, bo->height, bo->height,
		    DRM_FORMAT_XRGB8888, handles, pitches, offsets,&bo->fb_id, 0);
		    // DRM_FORMAT_ARGB8888, handles, pitches, offsets,&bo->fb_id, 0);
	if(ret ){
		printf("drmModeAddFB2 return err %d\n",ret);
		return 0;
	}

	printf("bo->pitch %d\n",bo->pitch);

			   
#endif



	memset(bo->vaddr, 0xff, bo->size);

	return 0;
}




static int modeset_create_yuvfb(int fd, struct buffer_object *bo)
{
	struct drm_mode_create_dumb create = {};
 	struct drm_mode_map_dumb map = {};
	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	int ret;

	

	create.width = bo->width;
	create.height = bo->height;
	create.bpp = 8;
	drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

	bo->pitch = create.pitch;
	bo->size = create.size;
	bo->handle = create.handle;


	map.handle = create.handle;
	drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map);

	bo->vaddr = mmap(0, create.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map.offset);


	offsets[0] = 0;
	handles[0] = bo->handle;
	pitches[0] = bo->pitch;

	ret = drmModeAddFB2(fd, bo->width, bo->height,
		    DRM_FORMAT_YUV422, handles, pitches, offsets,&bo->fb_id, 0);
	if(ret ){
		printf("drmModeAddFB2 return err %d\n",ret);
		return 0;
	}


	memset(bo->vaddr, 0xff, bo->size);

	return 0;
}


static int modeset_create_planefb(int fd, struct buffer_object *bo)
{

	return 0;
}

static void modeset_destroy_fb(int fd, struct buffer_object *bo)
{
	struct drm_mode_destroy_dumb destroy = {};

	drmModeRmFB(fd, bo->fb_id);

	munmap(bo->vaddr, bo->size);

	destroy.handle = bo->handle;
	drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
}


void get_planes_property(int fd,drmModePlaneRes *pr)
{
	drmModeObjectPropertiesPtr props;
	int i,j;
	drmModePropertyPtr p;
	
	for(i = 0; i < pr->count_planes; i++) {
		
		printf("planes id %d\n",pr->planes[i]);
		props = drmModeObjectGetProperties(fd, pr->planes[i],
					DRM_MODE_OBJECT_PLANE);
		if(props){		
            for (j = 0;j < props->count_props; j++) {
                p = drmModeGetProperty(fd, props->props[j]);
                printf("get property ,name %s, id %d\n",p->name,p->prop_id);
                drmModeFreeProperty(p);
            }	
            
            
            drmModeFreeObjectProperties(props);
		}
		printf("\n\n");
	}
	
}

void set_plane_property(int fd, int plane_id,struct property_arg *p)
{

	drmModeObjectPropertiesPtr props;
	drmModePropertyPtr pt;
	const char *obj_type;
	int ret;
	int i;

	props = drmModeObjectGetProperties(fd, plane_id,
				DRM_MODE_OBJECT_PLANE);
	
    if(props){
        for (i = 0; i < (int)props->count_props; ++i) {

            pt = drmModeGetProperty(fd, props->props[i]);
        
            if (!pt)
                continue;
            if (strcmp(pt->name, p->name) == 0){
                drmModeFreeProperty(pt);
                break;
            }
            
            drmModeFreeProperty(pt);
        }

        if (i == (int)props->count_props) {
            printf("%i has no %s property\n",p->obj_id, p->name);
            return;
        }

        p->prop_id = props->props[i];

        ret = drmModeObjectSetProperty(fd, p->obj_id, p->obj_type,
                           p->prop_id, p->value);
        if (ret < 0)
            printf("faild to set property %s,id %d,value %d\n",p->obj_id, p->name, p->value);

        drmModeFreeObjectProperties(props);
    }
}

//the alpha is globle setting,e.g. HEO alpha is 255, but still cant cover over1,2
void set_alpha(int fd, int plane_id,int value)
{
	struct property_arg prop;

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"alpha",sizeof("alpha"));
	prop.value = value; //0~255
	set_plane_property(fd,plane_id,&prop);
}

void set_rotate(int fd, int plane_id,int value)
{
	struct property_arg prop;
	
	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"rotation",sizeof("rotation"));
	prop.value = value; //otate-0=0x1 rotate-90=0x2 rotate-270=0x8 reflect-x=0x10 reflect-y=0x20
	set_plane_property(fd,plane_id,&prop);

}

void set_colorkey(int fd, int plane_id,int value)
{
	struct property_arg prop;
	

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"colorkey",sizeof("colorkey"));
	prop.value = value; //0~255
	set_plane_property(fd,plane_id,&prop);

}

void set_zpos(int fd, int plane_id,int value)
{
	struct property_arg prop;
	

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"zpos",sizeof("zpos"));
	prop.value = value; //0~255
	set_plane_property(fd,plane_id,&prop);

}

void set_type(int fd, int plane_id,int value)
{
	struct property_arg prop;
	

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"type",sizeof("type"));
	prop.value = value; //0~255
	set_plane_property(fd,plane_id,&prop);

}

void set_blend(int fd, int plane_id,int value)
{
	struct property_arg prop;
	

	memset(&prop, 0, sizeof(prop));
	prop.obj_type = 0;
	prop.name[DRM_PROP_NAME_LEN] = '\0';
	prop.obj_id = plane_id;
	memcpy(prop.name,"pixel blend mode",sizeof("pixel blend mode"));
	prop.value = value; //None=2 Pre-multiplied=0* Coverage=1
	set_plane_property(fd,plane_id,&prop);

}

void usage(char *name) {
	printf("zpos value [0-7]\n");
	printf("alpha value [0-65535]\n");
	printf("plane_index [0-7]\n");
	printf("rotate value:rotate-0=0x1 rotate-90=0x2 "
			"rotate-270=0x8 reflect-x=0x10 reflect-y=0x20");
	printf("%s setzpos  plane_index value   eg: %s setzpos  [0-7] [0 7] \n"
		   "%s setalpha  plane_index value  eg: %s setalpna 0-7]  [0 0xaffff]\n",
		   "%s setrotate plane_index value  eg: %s setrotate [0-7]  [0 0x20]\n",
		   name, name,
		   name, name,
		   name, name);
}

int main(int argc, char **argv)
{
	int fd;
	drmModeConnector *conn;
	drmModeRes *res;
	drmModePlaneRes *plane_res;
	uint32_t conn_id;
	uint32_t crtc_id,x,y;
	uint32_t plane_id;
	int ret;
	int rotation = 1,alpha = 10;;


	fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);

	res = drmModeGetResources(fd);
	crtc_id = res->crtcs[1];
	conn_id = res->connectors[1];

#if 0
	ret = drmSetClientCap(fd, DRM_CLIENT_CAP_ATOMIC, 1);
	if(ret) {
		printf("failed to set cap\n");
		return -1;
	}
#endif

	// overlay
	// 屏蔽这句话默认为overlay模式
	// 打开为drmModeGetPlaneResources获取所有图测试
#if 1
	//drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	ret = drmSetClientCap(fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (ret) {
		printf("failed to set client cap\n");
		return -1;
	}
#endif

	plane_res = drmModeGetPlaneResources(fd);
	plane_id = plane_res->planes[0];
	
	printf("get plane conn_id:%d  crtc_id:%d count %d,plane_id %d\n", conn_id, crtc_id, plane_res->count_planes,plane_id);

	for(int i = 0; i< plane_res->count_planes; i++) {
		printf("index:%d plane_id:%d\n", i, plane_res->planes[i]);
	}
	


	conn = drmModeGetConnector(fd, conn_id);
	buf.width = conn->modes[0].hdisplay;
	buf.height = conn->modes[0].vdisplay;

	printf("count_encoders:%d, connector_id:%d, encoder_id:%d\n",
			conn->count_encoders, conn->connector_id, conn->encoder_id);

	printf("get connector nanme %s,hdisplay %d, vdisplay %d,vrefresh %d\n",conn->modes[0].name,conn->modes[0].vdisplay,\
		conn->modes[0].hdisplay,conn->modes[0].vrefresh);

	if(argc == 0 || argc != 4) {
		usage(argv[0]);
		goto out;
	}

	int plane_index = atoi(argv[2]);
	// int value = atoi(argv[3]);
	int value = (int)strtol(argv[3], NULL, 0);

	int *cmd[] = {"setzpos", "setalpha", "setrotate"};

	printf("cmd choice:%s\n", argv[1]);
	printf("plane_index:%d\n", plane_index);
	printf("set value:%d ==> %#X\n", value, value);

	if(0 == memcmp(argv[1], cmd[0], strlen(cmd[0]))) {
		printf("set zpos\n");
		set_zpos(fd,plane_res->planes[plane_index],value);
		printf("set zpos over\n");

	} else if (0 == memcmp(argv[1], cmd[1], strlen(cmd[1]))) {
		printf("set alpha\n");
		set_alpha(fd,plane_res->planes[plane_index],value);
	} else if (0 == memcmp(argv[1], cmd[2], strlen(cmd[2]))) {
		printf("set alpha\n");
		set_rotate(fd,plane_res->planes[plane_index],value);
	}
	else {
		usage(argv[0]);
		goto out;
	}




#if 0

//int index = 4;
int index = 0;
// int index = 2;

	   

    //set_blend(fd,plane_res->planes[index],0);
    //set_type(fd,plane_res->planes[index],0);
	
    //set_alpha(fd,plane_res->planes[index],0xfff0);
    //set_alpha(fd,plane_res->planes[index],0xffff);
    set_alpha(fd,plane_res->planes[index],0xafff);
    //set_alpha(fd,plane_res->planes[index],0x0);

    set_colorkey(fd,plane_res->planes[index],0x7fffffff);
    //set_colorkey(fd,plane_res->planes[index],0x00ff0000);
    //set_colorkey(fd,plane_res->planes[index],0x0000ff00);
    //set_colorkey(fd,plane_res->planes[index],0x000000ff);
    //set_colorkey(fd,plane_res->planes[index],0x00);
	// index = 4 , plane_id = 155 , zpos =7   apha 0xfff0 时, colorkey没作用  plane_id=155在上面，plane_id = 84在下面  
	//             plane_id = 84   zpos = 6
	// modetest -M rockchip  -P 84@68:1920x1080+40+40 
	// zpos 7 是最上面
	//  更改pane_id = 84的alpha值为afff时为半透明
	//  也可以设置plane的rgb显示的格式为argb, 可以通过自己设置图片的alpha值控制透明度
	// QT程序 hdmi 54 68 zpos默认为 4
	// QT程序 mipi 69 83 zpos默认为 5
    //set_zpos(fd,plane_res->planes[index],0x7);
    set_zpos(fd,plane_res->planes[index],0x7);

	
#endif

	printf("will out\n");
	
out:


	drmModeFreeConnector(conn);
	drmModeFreePlaneResources(plane_res);
	drmModeFreeResources(res);

	close(fd);

	return 0;
}
