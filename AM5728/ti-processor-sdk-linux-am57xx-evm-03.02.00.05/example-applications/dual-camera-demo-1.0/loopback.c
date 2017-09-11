#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <linux/videodev2.h>
#include <omap_drm.h>
#include <omap_drmif.h>
#include <xf86drmMode.h>
#include "jpeg.h"
#include "loopback.h"

#define ERROR(fmt, ...) \
	do { fprintf(stderr, "ERROR:%s:%d: " fmt "\n", __func__, __LINE__,\
##__VA_ARGS__); } while (0)

#define MSG(fmt, ...) \
	do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)

/* Dynamic debug. */
#define DBG(fmt, ...) \
	do { fprintf(stderr, fmt "\n", ##__VA_ARGS__); } while (0)

/* align x to next highest multiple of 2^n */
#define ALIGN2(x,n)   (((x) + ((1 << (n)) - 1)) & ~((1 << (n)) - 1))
#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | \
	((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | \
	((uint32_t)(uint8_t)(d) << 24 ))
#define FOURCC_STR(str)    FOURCC(str[0], str[1], str[2], str[3])

#define PAGE_SHIFT 12
#define NBUF (3)
#define MAX_DRM_PLANES 5
#define CAP_WIDTH 800
#define CAP_HEIGHT 600
#define PIP_POS_X  25
#define PIP_POS_Y  25
#define MAX_ZORDER_VAL 3 //For AM57x device, max zoder value is 3

struct control status;

struct dmabuf_buffer {
	uint32_t fourcc, width, height;
	int nbo;
	struct omap_bo *bo[4];
	uint32_t pitches[4];
	int fd[4];		/* dmabuf */
	unsigned fb_id;
};

struct connector_info {
	unsigned int id;
	char mode_str[64];
	drmModeModeInfo *mode;
	drmModeEncoder *encoder;
	int crtc;
	int pipe;
};

/*
* drm output device structure declaration
*/
struct drm_device_info
{
	int fd;
	int width;
	int height;
	char dev_name[9];
	char name[4];
	unsigned int bo_flags;
	struct dmabuf_buffer **buf[2];
	struct omap_device *dev;
	unsigned int crtc_id;
	unsigned int plane_id[2];
	unsigned int prop_fbid;
	unsigned int prop_crtcid;
	uint64_t zorder_val_primary_plane;
	uint64_t trans_key_mode_val;
	uint32_t zorder_val[MAX_DRM_PLANES-1];
} drm_device;

/*
* V4L2 capture device structure declaration
*/
struct v4l2_device_info {
	int type;
	int fd;
	enum v4l2_memory memory_mode;
	unsigned int num_buffers;
	int width;
	int height;
	char dev_name[12];
	char name[10];

	struct v4l2_buffer *buf;
	struct v4l2_format fmt;
	struct dmabuf_buffer **buffers;
} cap0_device, cap1_device;

static struct omap_bo *alloc_bo(struct drm_device_info *device, unsigned int bpp,
		unsigned int width, unsigned int height,
		unsigned int *bo_handle, unsigned int *pitch)
{
	struct omap_bo *bo;
	unsigned int bo_flags = device->bo_flags;

	bo_flags |= OMAP_BO_WC;
	bo = omap_bo_new(device->dev, width * height * bpp / 8, bo_flags);

	if (bo) {
		*bo_handle = omap_bo_handle(bo);
		*pitch = width * bpp / 8;
		if (bo_flags & OMAP_BO_TILED)
			*pitch = ALIGN2(*pitch, PAGE_SHIFT);
	}

	return bo;
}

//You can use DRM ioctl as well to allocate buffers (DRM_IOCTL_MODE_CREATE_DUMB)
//and drmPrimeHandleToFD() to get the buffer descriptors
static struct dmabuf_buffer *alloc_buffer(struct drm_device_info *device,
		unsigned int fourcc, unsigned int w,
		unsigned int h)
{
	struct dmabuf_buffer *buf;
	unsigned int bo_handles[4] = {0}, offsets[4] = {0};
	int ret;

	buf = (struct dmabuf_buffer *) calloc(1, sizeof(struct dmabuf_buffer));
	if (!buf) {
		ERROR("allocation failed");
		return NULL;
	}

	buf->fourcc = fourcc;
	buf->width = w;
	buf->height = h;
	buf->nbo = 1;
	buf->bo[0] = alloc_bo(device, 16, buf->width, buf->height,
		&bo_handles[0], &buf->pitches[0]);

	ret = drmModeAddFB2(device->fd, buf->width, buf->height, fourcc,
		bo_handles, buf->pitches, offsets, &buf->fb_id, 0);

	if (ret) {
		ERROR("drmModeAddFB2 failed: %s (%d)", strerror(errno), ret);
		return NULL;
	}

	return buf;
}

void free_vid_buffers(struct dmabuf_buffer **buf, unsigned int n)
{
	unsigned int i;

	if (buf == NULL) return;
	for (i = 0; i < n; i++) {
		if (buf[i]) {
			close(buf[i]->fd[0]);
			omap_bo_del(buf[i]->bo[0]);
			free(buf[i]);
		}
	}
	free(buf);
}


static struct dmabuf_buffer **get_vid_buffers(struct drm_device_info *device,
		unsigned int n,
		unsigned int fourcc, unsigned int w, unsigned int h)
{
	struct dmabuf_buffer **bufs;
	unsigned int i = 0;

	bufs = (struct dmabuf_buffer **) calloc(n, sizeof(*bufs));
	if (!bufs) {
		ERROR("allocation failed");
		goto fail;
	}

	for (i = 0; i < n; i++) {
		bufs[i] = alloc_buffer(device, fourcc, w, h);
		if (!bufs[i]) {
			ERROR("allocation failed");
			goto fail;
		}
	}
	return bufs;

fail:
	return NULL;
}

static int v4l2_init_device(struct v4l2_device_info *device)
{
	int ret;
	struct v4l2_capability capability;

	/* Open the capture device */
	device->fd = open((const char *) device->dev_name, O_RDWR);
	if (device->fd <= 0) {
		printf("Cannot open %s device\n", device->dev_name);
		return -1;
	}

	MSG("\n%s: Opened Channel\n", device->name);

	/* Check if the device is capable of streaming */
	if (ioctl(device->fd, VIDIOC_QUERYCAP, &capability) < 0) {
		perror("VIDIOC_QUERYCAP");
		goto ERROR;
	}

	if (capability.capabilities & V4L2_CAP_STREAMING)
		MSG("%s: Capable of streaming\n", device->name);
	else {
		ERROR("%s: Not capable of streaming\n", device->name);
		goto ERROR;
	}

	{
		struct v4l2_streamparm streamparam;
		streamparam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (ioctl(device->fd, VIDIOC_G_PARM, &streamparam) < 0){
			perror("VIDIOC_G_PARM");
			goto ERROR;
		}
	}

	device->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ret = ioctl(device->fd, VIDIOC_G_FMT, &device->fmt);
	if (ret < 0) {
		ERROR("VIDIOC_G_FMT failed: %s (%d)", strerror(errno), ret);
		goto ERROR;
	}

	device->fmt.fmt.pix.pixelformat = FOURCC_STR("YUYV");
	device->fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	device->fmt.fmt.pix.width = device->width;
	device->fmt.fmt.pix.height = device->height;

	ret = ioctl(device->fd, VIDIOC_S_FMT, &device->fmt);
	if (ret < 0) {
		perror("VIDIOC_S_FMT");
		goto ERROR;
	}

	MSG("%s: Init done successfully\n", device->name);
	return 0;

ERROR:
	close(device->fd);

	return -1;
}

static void v4l2_exit_device(struct v4l2_device_info *device)
{

	free(device->buf);
	close(device->fd);

	return;
}


/*
* Enable streaming for V4L2 capture device
*/
static int v4l2_stream_on(struct v4l2_device_info *device)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret = 0;

	ret = ioctl(device->fd, VIDIOC_STREAMON, &type);

	if (ret) {
		ERROR("VIDIOC_STREAMON failed: %s (%d)", strerror(errno), ret);
	}

	return ret;
}

/*
* Disable streaming for V4L2 capture device
*/
static int v4l2_stream_off(struct v4l2_device_info *device)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret = -1;

	if (device->fd <= 0) {
		return ret;
	}

	ret = ioctl(device->fd, VIDIOC_STREAMOFF, &type);

	if (ret) {
		ERROR("VIDIOC_STREAMOFF failed: %s (%d)", strerror(errno), ret);
	}

	return ret;
}

static int v4l2_request_buffer(struct v4l2_device_info *device,
		struct dmabuf_buffer **bufs)
{
	struct v4l2_requestbuffers reqbuf;
	unsigned int i;
	int ret,dmafd;

	if (device->buf) {
		// maybe eventually need to support this?
		ERROR("already reqbuf'd");
		return -1;
	}

	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = device->memory_mode;
	reqbuf.count = device->num_buffers;

	ret = ioctl(device->fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		ERROR("VIDIOC_REQBUFS failed: %s (%d)", strerror(errno), ret);
		return ret;
	}

	if ((reqbuf.count != device->num_buffers) ||
		(reqbuf.type != V4L2_BUF_TYPE_VIDEO_CAPTURE) ||
		(reqbuf.memory != V4L2_MEMORY_DMABUF)) {
			ERROR("unsupported..");
			return -1;
	}

	device->num_buffers = reqbuf.count;
	device->buffers = bufs;
	device->buf = (struct v4l2_buffer *) calloc(device->num_buffers, \
			sizeof(struct v4l2_buffer));
	if (!device->buf) {
		ERROR("allocation failed");
		return -1;
	}

	for (i = 0; i < device->num_buffers; i++) {
		if (bufs[i]->nbo != 1){
			ERROR("Number of buffers not right");
		};

		/* Call omap_bo_dmabuf only once, to export only once
		* Otherwise, each call will return duplicated fds
		* This way, every call to omap_bo_dmabuf will return a new fd
		* Which won't match with any previously exported fds
		* Instead, store dma fd in buf->fd[] */
		dmafd = omap_bo_dmabuf(bufs[i]->bo[0]);
		bufs[i]->fd[0] = dmafd;

		device->buf[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		device->buf[i].memory = V4L2_MEMORY_DMABUF;
		device->buf[i].index = i;

		MSG("Exported buffer fd = %d\n", dmafd);
		ret = ioctl(device->fd, VIDIOC_QUERYBUF, &device->buf[i]);
		device->buf[i].m.fd = dmafd;

		if (ret) {
			ERROR("VIDIOC_QUERYBUF failed: %s (%d)", strerror(errno), ret);
			return ret;
		}
	}

	return 0;
}

/*
* Queue V4L2 buffer
*/
static int v4l2_queue_buffer(struct v4l2_device_info *device,
		struct dmabuf_buffer *buf)
{
	struct v4l2_buffer *v4l2buf = NULL;
	int  ret, fd;
	unsigned char i;

	if(buf->nbo != 1){
		ERROR("number of bufers not right\n");
		return -1;
	}

	fd = buf->fd[0];

	for (i = 0; i < device->num_buffers; i++) {
		if (device->buf[i].m.fd == fd) {
			v4l2buf = &device->buf[i];
		}
	}

	if (!v4l2buf) {
		ERROR("invalid buffer");
		return -1;
	}
	ret = ioctl(device->fd, VIDIOC_QBUF, v4l2buf);
	if (ret) {
		ERROR("VIDIOC_QBUF failed: %s (%d)", strerror(errno), ret);
	}

	return ret;
}

/*
* DeQueue V4L2 buffer
*/
struct dmabuf_buffer *v4l2_dequeue_buffer(struct v4l2_device_info *device)
{
	struct dmabuf_buffer *buf;
	struct v4l2_buffer v4l2buf;
	int ret;

	v4l2buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2buf.memory = V4L2_MEMORY_DMABUF;
	ret = ioctl(device->fd, VIDIOC_DQBUF, &v4l2buf);
	if (ret) {
		ERROR("VIDIOC_DQBUF failed: %s (%d)\n", strerror(errno), ret);
		return NULL;
	}

	buf = device->buffers[v4l2buf.index];

	device->buf[v4l2buf.index].timestamp = v4l2buf.timestamp;
	if(buf->nbo != 1){
		ERROR("num buffers not proper\n");
		return NULL;
	}

	return buf;
}



unsigned int get_drm_prop_val(int fd, drmModeObjectPropertiesPtr props,
							  const char *name)
{
	drmModePropertyPtr p;
	unsigned int i, prop_id = 0; /* Property ID should always be > 0 */

	for (i = 0; !prop_id && i < props->count_props; i++) {
		p = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(p->name, name)){
			prop_id = p->prop_id;
			break;
		}
		drmModeFreeProperty(p);
	}
	if (!prop_id) {
		printf("Could not find %s property\n", name);
		drmModeFreeObjectProperties(props);
		exit(-1);
	}

	drmModeFreeProperty(p);
	return props->prop_values[i];
}

unsigned int find_drm_prop_id(int fd, drmModeObjectPropertiesPtr props,
							  const char *name)
{
	drmModePropertyPtr p;
	unsigned int i, prop_id = 0; /* Property ID should always be > 0 */

	for (i = 0; !prop_id && i < props->count_props; i++) {
		p = drmModeGetProperty(fd, props->props[i]);
		if (!strcmp(p->name, name)){
			prop_id = p->prop_id;
			break;
		}
		drmModeFreeProperty(p);
	}
	if (!prop_id) {
		printf("Could not find %s property\n", name);
		drmModeFreeObjectProperties(props);
		exit(-1);
	}

	return prop_id;
}

void add_property(int fd, drmModeAtomicReqPtr req,
				  drmModeObjectPropertiesPtr props,
				  unsigned int plane_id,
				  const char *name, int value)
{
	unsigned int prop_id = find_drm_prop_id(fd, props, name);
	if(drmModeAtomicAddProperty(req, plane_id, prop_id, value) < 0){
		printf("failed to add property\n");
	}
}

void drm_add_plane_property(struct drm_device_info *dev, drmModeAtomicReqPtr req)
{
	unsigned int i;
	unsigned int crtc_x_val = 0;
	unsigned int crtc_y_val = 0;
	unsigned int crtc_w_val = dev->width;
	unsigned int crtc_h_val = dev->height;
	drmModeObjectProperties *props;
	unsigned int zorder_val = 1;
	unsigned int buf_index;
	struct v4l2_device_info  *v4l2_device;

	for(i = 0; i < status.num_cams; i++){
		unsigned int plane_id = dev->plane_id[i];

		if(i) {
			crtc_x_val = PIP_POS_X;
			crtc_y_val = PIP_POS_Y;
			crtc_w_val /= 3;
			crtc_h_val /= 3;
		}
		props = drmModeObjectGetProperties(dev->fd, plane_id,
			DRM_MODE_OBJECT_PLANE);

		if(props == NULL){
			ERROR("drm obeject properties for plane type is NULL\n");
			exit (-1);
		}

		//fb id value will be set every time new frame is to be displayed
		dev->prop_fbid = find_drm_prop_id(drm_device.fd, props, "FB_ID");

		//Will need to change run time crtc id to disable/enable display of plane
		dev->prop_crtcid = find_drm_prop_id(drm_device.fd, props, "CRTC_ID");

		//storing zorder val to restore it before quitting the demo
		drm_device.zorder_val[i] = get_drm_prop_val(drm_device.fd, props, "zorder");

		//Set the fb id based on which camera is chosen to be main camera 
		if (status.main_cam  == 1){
			buf_index = (i+1)%2;
		}
		else {
			buf_index = i;
		}

		if(buf_index == 0){
			v4l2_device = &cap0_device;
		}
		else{
			v4l2_device = &cap1_device;
		}
		printf("w=%d, h=%d\n", v4l2_device->width, v4l2_device->height);
		add_property(dev->fd, req, props, plane_id, "FB_ID", dev->buf[buf_index][0]->fb_id);

		//set the plane properties once. No need to set these values every time
		//with the display of frame. 
		add_property(dev->fd, req, props, plane_id, "CRTC_ID", dev->crtc_id);
		add_property(dev->fd, req, props, plane_id, "CRTC_X", crtc_x_val);
		add_property(dev->fd, req, props, plane_id, "CRTC_Y", crtc_y_val);
		add_property(dev->fd, req, props, plane_id, "CRTC_W", crtc_w_val);
		add_property(dev->fd, req, props, plane_id, "CRTC_H", crtc_h_val);
		add_property(dev->fd, req, props, plane_id, "SRC_X", 0);
		add_property(dev->fd, req, props, plane_id, "SRC_Y", 0);
		add_property(dev->fd, req, props, plane_id, "SRC_W", v4l2_device->width << 16);
		add_property(dev->fd, req, props, plane_id, "SRC_H", v4l2_device->height << 16);
		add_property(dev->fd, req, props, plane_id, "zorder", zorder_val++);
	}
}

uint32_t drm_reserve_plane(int fd, unsigned int *ptr_plane_id, int num_planes)
{
	unsigned int i;
	int idx = 0;
	drmModeObjectProperties *props;
	drmModePlaneRes *res = drmModeGetPlaneResources(fd);
	if(res == NULL){
		ERROR("plane resources not found\n");
	}

	for (i = 0; i < res->count_planes; i++) {
		uint32_t plane_id = res->planes[i];
		unsigned int type_val;

		drmModePlane *plane = drmModeGetPlane(fd, plane_id);
		if(plane == NULL){
			ERROR("plane  not found\n");
		}

		props = drmModeObjectGetProperties(fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);

		if(props == NULL){
			ERROR("plane (%d) properties not found\n",  plane->plane_id);
		}

		type_val = get_drm_prop_val(fd, props, "type");

		if(type_val == DRM_PLANE_TYPE_OVERLAY){
			ptr_plane_id[idx++] = plane_id;
		}

		drmModeFreeObjectProperties(props);
		drmModeFreePlane(plane);

		if(idx == num_planes){
			drmModeFreePlaneResources(res);
			return 0;
		}
	}

	ERROR("plane couldn't be reserved\n");
	return -1;
}


/* Get crtc id and resolution. */
void drm_crtc_resolution(struct drm_device_info *device)
{
	drmModeCrtc *crtc;
	int i;

	drmModeResPtr res = drmModeGetResources(device->fd);

	if (res == NULL){
		ERROR("drmModeGetResources failed: %s\n", strerror(errno));
		exit(0);
	};

	for (i = 0; i < res->count_crtcs; i++) {
		unsigned int crtc_id = res->crtcs[i];

		crtc = drmModeGetCrtc(device->fd, crtc_id);
		if (!crtc) {
			DBG("could not get crtc %i: %s\n", res->crtcs[i], strerror(errno));
			continue;
		}
		if (!crtc->mode_valid) {
			drmModeFreeCrtc(crtc);
			continue;
		}

		printf("CRTCs size: %dx%d\n", crtc->width, crtc->height);
		device->crtc_id = crtc_id;
		device->width = crtc->width;
		device->height = crtc->height;

		drmModeFreeCrtc(crtc);
	}

	drmModeFreeResources(res);
}

/*
* DRM restore properties
*/
static void drm_restore_props(struct drm_device_info *device)
{
	unsigned int i;
	drmModeObjectProperties *props;
	int ret;

	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	props = drmModeObjectGetProperties(device->fd, device->crtc_id,
		DRM_MODE_OBJECT_CRTC);

	//restore trans-key-mode and z-order of promary plane
	add_property(device->fd, req, props, device->crtc_id, "trans-key-mode", \
			device->trans_key_mode_val);
	add_property(device->fd, req, props, device->crtc_id, "zorder", \
			device->zorder_val_primary_plane);

	//restore z-order of overlay planes
	for(i = 0; i < status.num_cams; i++){
		props = drmModeObjectGetProperties(device->fd, device->plane_id[i],
			DRM_MODE_OBJECT_PLANE);

		if(props == NULL){
			ERROR("drm obeject properties for plane type is NULL\n");
			exit (-1);
		}

		add_property(device->fd, req, props, device->plane_id[i], \
				"zorder", device->zorder_val[i]);
	}

	//Commit all the added properties
	ret = drmModeAtomicCommit(device->fd, req, DRM_MODE_ATOMIC_TEST_ONLY, 0);
	if(!ret){
		drmModeAtomicCommit(device->fd, req, 0, 0);
	}
	else{
		ERROR("ret from drmModeAtomicCommit = %d\n", ret);
	}

	drmModeAtomicFree(req);
}

/*
* drm device init
*/
static int drm_init_device(struct drm_device_info *device)
{
	if (!device->fd) {
		device->fd = drmOpen("omapdrm", NULL);
		if (device->fd < 0) {
			ERROR("could not open drm device: %s (%d)", strerror(errno), errno);
			return -1;
		}
	}

	drmSetClientCap(device->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	drmSetClientCap(device->fd, DRM_CLIENT_CAP_ATOMIC, 1);

	device->dev = omap_device_new(device->fd);

	/* Get CRTC id and resolution. As well set the global display width and height */
	drm_crtc_resolution(device);

	/* Store display resolution so GUI can be configured */
	status.display_xres = device->width;
	status.display_yres = device->height;

	drm_reserve_plane(device->fd, device->plane_id, status.num_cams);

	return 0;
}

/*
*Clean resources while exiting drm device
*/
static void drm_exit_device(struct drm_device_info *device)
{
	drm_restore_props(device);
	drmSetClientCap(device->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 0);
	drmSetClientCap(device->fd, DRM_CLIENT_CAP_ATOMIC, 0);

	omap_device_del(device->dev);
	device->dev = NULL;
	if (device->fd > 0) {
		close(device->fd);
	}

	return;
}

/*
* Set up the DSS for blending of video and graphics planes
*/
static int drm_init_dss(void)
{
	drmModeObjectProperties *props;
	int ret;
    FILE *fp;
    char str[10];
    char trans_key_mode = 2;

	/*
	* Dual camera demo is supported on Am437x and AM57xx evm. Find which
	* specific SoC the demo is running to set the trans key mode -
	* found at the corresponding TRM, for example,
	* For AM437x: trans_key_mode = 1 GFX Dest Trans
	*             TransKey applies to GFX overlay, marking which
	*              pixels come from VID overlays)
	* For AM57xx: trans_key_mode = 2 Source Key.
	*             Match on Layer4 makes Layer4 transparent (zorder 3)
	*
	* The deault mode is 1 for backward compatibility
	*/

	fp = fopen("/proc/sys/kernel/hostname", "r");
	fscanf(fp, "%s", str);

	//terminate the string after the processor name. "-evm" extension is
	//ignored in case the demo gets supported on other boards like idk etc
	str[6] = '\0';
	printf("Running the demo on %s processor\n", str);

	//Set trans-key-mode to 1 if dual camera demo is running on AM437x
	if (strcmp(str,"am437x") == 0){
		trans_key_mode = 1;
	}

	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	/* Set CRTC properties */
	props = drmModeObjectGetProperties(drm_device.fd, drm_device.crtc_id,
		DRM_MODE_OBJECT_CRTC);

	drm_device.zorder_val_primary_plane = get_drm_prop_val(drm_device.fd,
		props, "zorder");
	drm_device.trans_key_mode_val = get_drm_prop_val(drm_device.fd, props,
		"trans-key-mode");

	add_property(drm_device.fd, req, props, drm_device.crtc_id,
		"trans-key-mode", trans_key_mode);
	add_property(drm_device.fd, req, props, drm_device.crtc_id,
		"alpha_blender", 1);
	add_property(drm_device.fd, req, props, drm_device.crtc_id,
		"zorder", MAX_ZORDER_VAL);

	/* Set overlay plane properties like zorder, crtc_id, buf_id, src and */
	/* dst w, h etc                                                       */
	drm_add_plane_property(&drm_device, req);

	//Commit all the added properties
	ret = drmModeAtomicCommit(drm_device.fd, req, DRM_MODE_ATOMIC_TEST_ONLY, 0);
	if(!ret){
		drmModeAtomicCommit(drm_device.fd, req, 0, 0);
	}
	else{
		ERROR("ret from drmModeAtomicCommit = %d\n", ret);
		return -1;
	}

	drmModeAtomicFree(req);
	return 0;
}


/*
* drm disable pip layer
*/
void drm_disable_pip(void)
{
	int ret;
	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	drmModeAtomicAddProperty(req, drm_device.plane_id[1],
		drm_device.prop_fbid, 0);
	drmModeAtomicAddProperty(req, drm_device.plane_id[1],
		drm_device.prop_crtcid, 0);

	ret = drmModeAtomicCommit(drm_device.fd, req,
		DRM_MODE_ATOMIC_TEST_ONLY, 0);
	if(!ret){
		drmModeAtomicCommit(drm_device.fd, req,
			0, 0);
	}
	else{
		ERROR("failed to enable plane %d atomically: %s",
			drm_device.plane_id[!status.main_cam], strerror(errno));
	}

	drmModeAtomicFree(req);
}

void drm_enable_pip(void)
{
	int ret;

	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	drmModeAtomicAddProperty(req, drm_device.plane_id[1],
		drm_device.prop_fbid, drm_device.buf[!status.main_cam][0]->fb_id);
	drmModeAtomicAddProperty(req, drm_device.plane_id[1],
		drm_device.prop_crtcid, drm_device.crtc_id);

	ret = drmModeAtomicCommit(drm_device.fd, req,
		DRM_MODE_ATOMIC_TEST_ONLY, 0);

	if(!ret){
		drmModeAtomicCommit(drm_device.fd, req, 0, 0);
	}
	else{
		ERROR("failed to enable plane %d atomically: %s",
			drm_device.plane_id[!status.main_cam], strerror(errno));
	}

	drmModeAtomicFree(req);
}

/*
* Capture v4l2 frame and save to jpeg
*/
static int capture_frame(struct v4l2_device_info *v4l2_device,
						 struct dmabuf_buffer *buf)
{

	jpegWrite(omap_bo_map(buf->bo[0]),
		status.num_jpeg, v4l2_device->width, v4l2_device->height);

	return 0;
}

/*
* Initialize the app resources with default parameters
*/
void default_parameters(void)
{
	/* Main camera display */
	memset(&drm_device, 0, sizeof(drm_device));
	strcpy(drm_device.dev_name,"/dev/drm");
	strcpy(drm_device.name,"drm");
	drm_device.width=0;
	drm_device.height=0;
	drm_device.bo_flags = OMAP_BO_SCANOUT;
	drm_device.fd = 0;

	/* Main camera */
	cap0_device.memory_mode = V4L2_MEMORY_DMABUF;
	cap0_device.num_buffers = NBUF;
	strcpy(cap0_device.dev_name,"/dev/video1");
	strcpy(cap0_device.name,"Capture 0");
	cap0_device.buffers = NULL;
	cap0_device.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	cap0_device.width = CAP_WIDTH;
	cap0_device.height = CAP_HEIGHT;

	/* PiP camera */
	cap1_device.memory_mode = V4L2_MEMORY_DMABUF;
	cap1_device.num_buffers = NBUF;
	strcpy(cap1_device.dev_name,"/dev/video0");
	strcpy(cap1_device.name,"Capture 1");
	cap1_device.buffers = NULL;
	cap1_device.fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	cap1_device.width = CAP_WIDTH;
	cap1_device.height = CAP_HEIGHT;

	/* Set the default parameters for device options */
	status.main_cam=0;
	status.num_cams=2;
	status.num_jpeg=0;
	if(status.num_cams == 1){
		status.pip=false;
	}
	else{
		status.pip=true;
	}
	status.jpeg=false;
	status.exit=false;

	/* Ensure that jpeg image save directory exists */
	mkdir("/usr/share/camera-images/", 0777);

	return;
}

/*
* Free resource and exit devices
*/
void exit_devices(void)
{
	v4l2_exit_device(&cap0_device);
	if (status.num_cams==2) {
		v4l2_exit_device(&cap1_device);
	}
	free_vid_buffers(drm_device.buf[0], NBUF);
	free_vid_buffers(drm_device.buf[1], NBUF);
	drm_exit_device(&drm_device);
}

/*
* End camera streaming
*/
void end_streaming(void)
{
	v4l2_stream_off(&cap0_device);
	if (status.num_cams==2) {
		v4l2_stream_off(&cap1_device);
	}
}

void set_plane_properties()
{
	int ret;
	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	req = drmModeAtomicAlloc();

	drmModeAtomicAddProperty(req, drm_device.plane_id[0],
		drm_device.prop_fbid, drm_device.buf[status.main_cam][0]->fb_id);

	if(status.pip){
		drmModeAtomicAddProperty(req, drm_device.plane_id[1],
			drm_device.prop_fbid, drm_device.buf[!status.main_cam][0]->fb_id);
	}
	ret = drmModeAtomicCommit(drm_device.fd, req,
		DRM_MODE_ATOMIC_TEST_ONLY, 0);
	if(!ret){
		drmModeAtomicCommit(drm_device.fd, req,
			0, 0);
	}
	else{
		ERROR("failed to enable plane %d atomically: %s",
			drm_device.plane_id[!status.main_cam], strerror(errno));
	}
	drmModeAtomicFree(req);
}
/*
* Initializes all drm and v4l2 devices for loopback
*/
int init_loopback(void)
{
	bool status_cam0 = 0;
	bool status_cam1 = 0;

	/* Declare properties for video and capture devices */
	default_parameters();

	/* Initialize the drm display devic */
	if (drm_init_device(&drm_device)) goto Error;

	/* Check to see if the display resolution is very small.  If so, the
	* camera capture resolution needs to be lowered so that the scaling
	* limits of the DSS are not reached */
	if (drm_device.width < 640) {
		/* Set capture 0 device resolution */
		cap0_device.width = 640;
		cap0_device.height = 480;

		/* Set capture 1 device resolution */
		cap1_device.width = 640;
		cap1_device.height = 480;
	}

	/* Initialize the v4l2 capture devices */
	if (v4l2_init_device(&cap0_device) < 0) {
		printf("first camera detection failed\n");
		/* If there is not a second camera, program can still continue */
		status.num_cams=1;
		status.main_cam=1;
		status.pip=false;
	}
	else{
		unsigned int i;
		struct dmabuf_buffer **buffers = get_vid_buffers(&drm_device, cap0_device.num_buffers, 
			cap0_device.fmt.fmt.pix.pixelformat, cap0_device.width, cap0_device.height);

		if (!buffers) {
			goto Error;
		}

		drm_device.buf[0] = buffers;

		/* Pass these buffers to the capture drivers */
		if (v4l2_request_buffer(&cap0_device, buffers) < 0) {
			goto Error;
		}

		for (i = 0; i < cap0_device.num_buffers; i++) {
			v4l2_queue_buffer(&cap0_device, buffers[i]);
		}

		status_cam0 = 1;
	}

	if(v4l2_init_device(&cap1_device) < 0) {
		/* If there is not a second camera, program can still continue */
		if(status.num_cams ==2){
			status.num_cams=1;
			status.pip=false;
			printf("Only one camera detected\n");
		}
		//first camera wasn't detected
		else if (!status_cam0){
			printf("No camera detected\n");
			goto Error;
		}
	}
	else{
		unsigned int i;
		struct dmabuf_buffer **buffers = get_vid_buffers(&drm_device, cap1_device.num_buffers, 
			cap1_device.fmt.fmt.pix.pixelformat, cap1_device.width, cap1_device.height);
		if (!buffers) {
			goto Error;
		}

		drm_device.buf[1] = buffers;

		/* Pass these buffers to the capture drivers */
		if (v4l2_request_buffer(&cap1_device, buffers) < 0) {
			goto Error;
		}

		for (i = 0; i < cap1_device.num_buffers; i++) {
			v4l2_queue_buffer(&cap1_device, buffers[i]);
		}

		status_cam1 = 1;
	}

	/* Enable streaming for the v4l2 capture devices */
	if(status_cam0){
		if (v4l2_stream_on(&cap0_device) < 0) goto Error;
	}

	if (status_cam1) {
		if (v4l2_stream_on(&cap1_device) < 0) goto Error;
	}

	/* Configure the DSS to blend video and graphics layers */
	if (drm_init_dss() < 0 ) goto Error;

	return 0;

Error:
	status.exit = true;
	return -1;
}

static void page_flip_handler(int fd, unsigned int frame,
							  unsigned int sec, unsigned int usec,
							  void *data)
{
	int *waiting_for_flip = data;
	*waiting_for_flip = 0;
}

/*
* Determines which camera feeds are being displayed and
* whether a jpeg image needs to be captured.
*/
void process_frame(void)
{
	fd_set fds;
	int ret, waiting_for_flip = 1;
	struct dmabuf_buffer *buf[2] = {NULL, NULL};
	drmModeAtomicReqPtr req = drmModeAtomicAlloc();

	struct v4l2_device_info *v4l2_device[2] =
	{&cap0_device, &cap1_device};

	drmEventContext evctx = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.vblank_handler = 0,
		.page_flip_handler = page_flip_handler,
	};

	/* Request a capture buffer from the driver that can be copied to */
	/* framebuffer */
	buf[status.main_cam] =
		v4l2_dequeue_buffer(v4l2_device[status.main_cam]);
	drmModeAtomicAddProperty(req, drm_device.plane_id[0],
		drm_device.prop_fbid, buf[status.main_cam]->fb_id);

	if (status.pip==true) {
		buf[!status.main_cam] =
			v4l2_dequeue_buffer(v4l2_device[!status.main_cam]);
		drmModeAtomicAddProperty(req, drm_device.plane_id[1],
			drm_device.prop_fbid, buf[!status.main_cam]->fb_id);
	}

	ret = drmModeAtomicCommit(drm_device.fd, req,
		DRM_MODE_ATOMIC_TEST_ONLY, 0);

	if(!ret){
		drmModeAtomicCommit(drm_device.fd, req,
			DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK, &waiting_for_flip);
		}
		else {
		ERROR("failed to add plane atomically: %s", strerror(errno));
		}

	drmModeAtomicFree(req);

	/* Save jpeg image if triggered */
	if (status.jpeg==true) {
		capture_frame(v4l2_device[status.main_cam], buf[status.main_cam]);
		status.jpeg=false;
		status.num_jpeg++;
		if (status.num_jpeg==10)
			status.num_jpeg=0;
	}

	FD_ZERO(&fds);
	FD_SET(drm_device.fd, &fds);

	while (waiting_for_flip) {
		ret = select(drm_device.fd + 1, &fds, NULL, NULL, NULL);
		if (ret < 0) {
			printf("select err: %s\n", strerror(errno));
			return;
		}
		else if (ret == 0) {
			printf("select timeout!\n");
			return;
		}
		else if (FD_ISSET(0, &fds)) {
			continue;
		}
		drmHandleEvent(drm_device.fd, &evctx);
	}

	v4l2_queue_buffer(v4l2_device[status.main_cam], buf[status.main_cam]);
	if(status.pip == true){
		v4l2_queue_buffer(v4l2_device[!status.main_cam], buf[!status.main_cam]);
	}
}
