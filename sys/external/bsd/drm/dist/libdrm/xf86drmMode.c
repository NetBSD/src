/*
 * \file xf86drmMode.c
 * Header for DRM modesetting interface.
 *
 * \author Jakob Bornecrantz <wallbraker@gmail.com>
 *
 * \par Acknowledgements:
 * Feb 2007, Dave Airlie <airlied@linux.ie>
 */

/*
 * Copyright (c) 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2007-2008 Dave Airlie <airlied@linux.ie>
 * Copyright (c) 2007-2008 Jakob Bornecrantz <wallbraker@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

/*
 * TODO the types we are after are defined in diffrent headers on diffrent
 * platforms find which headers to include to get uint32_t
 */
#include <stdint.h>
#include <sys/ioctl.h>
#include <stdio.h>

#include "xf86drmMode.h"
#include "xf86drm.h"
#include <drm.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#define U642VOID(x) ((void *)(unsigned long)(x))
#define VOID2U64(x) ((uint64_t)(unsigned long)(x))

/*
 * Util functions
 */

void* drmAllocCpy(void *array, int count, int entry_size)
{
	char *r;
	int i;

	if (!count || !array || !entry_size)
		return 0;

	if (!(r = drmMalloc(count*entry_size)))
		return 0;

	for (i = 0; i < count; i++)
		memcpy(r+(entry_size*i), array+(entry_size*i), entry_size);

	return r;
}

/*
 * A couple of free functions.
 */

void drmModeFreeModeInfo(drmModeModeInfoPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);
}

void drmModeFreeResources(drmModeResPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);

}

void drmModeFreeFB(drmModeFBPtr ptr)
{
	if (!ptr)
		return;

	/* we might add more frees later. */
	drmFree(ptr);
}

void drmModeFreeCrtc(drmModeCrtcPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr);

}

void drmModeFreeConnector(drmModeConnectorPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->modes);
	drmFree(ptr);

}

void drmModeFreeEncoder(drmModeEncoderPtr ptr)
{
	drmFree(ptr);
}

/*
 * ModeSetting functions.
 */

drmModeResPtr drmModeGetResources(int fd)
{
	struct drm_mode_card_res res;
	drmModeResPtr r = 0;

	memset(&res, 0, sizeof(struct drm_mode_card_res));

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res))
		return 0;

	if (res.count_fbs)
		res.fb_id_ptr = VOID2U64(drmMalloc(res.count_fbs*sizeof(uint32_t)));
	if (res.count_crtcs)
		res.crtc_id_ptr = VOID2U64(drmMalloc(res.count_crtcs*sizeof(uint32_t)));
	if (res.count_connectors)
		res.connector_id_ptr = VOID2U64(drmMalloc(res.count_connectors*sizeof(uint32_t)));
	if (res.count_encoders)
		res.encoder_id_ptr = VOID2U64(drmMalloc(res.count_encoders*sizeof(uint32_t)));

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETRESOURCES, &res)) {
		r = NULL;
		goto err_allocs;
	}

	/*
	 * return
	 */


	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->min_width     = res.min_width;
	r->max_width     = res.max_width;
	r->min_height    = res.min_height;
	r->max_height    = res.max_height;
	r->count_fbs     = res.count_fbs;
	r->count_crtcs   = res.count_crtcs;
	r->count_connectors = res.count_connectors;
	r->count_encoders = res.count_encoders;
	/* TODO we realy should test if these allocs fails. */
	r->fbs           = drmAllocCpy(U642VOID(res.fb_id_ptr), res.count_fbs, sizeof(uint32_t));
	r->crtcs         = drmAllocCpy(U642VOID(res.crtc_id_ptr), res.count_crtcs, sizeof(uint32_t));
	r->connectors       = drmAllocCpy(U642VOID(res.connector_id_ptr), res.count_connectors, sizeof(uint32_t));
	r->encoders      = drmAllocCpy(U642VOID(res.encoder_id_ptr), res.count_encoders, sizeof(uint32_t));

err_allocs:
	drmFree(U642VOID(res.fb_id_ptr));
	drmFree(U642VOID(res.crtc_id_ptr));
	drmFree(U642VOID(res.connector_id_ptr));
	drmFree(U642VOID(res.encoder_id_ptr));

	return r;
}

int drmModeAddFB(int fd, uint32_t width, uint32_t height, uint8_t depth,
                 uint8_t bpp, uint32_t pitch, uint32_t bo_handle,
		 uint32_t *buf_id)
{
	struct drm_mode_fb_cmd f;
	int ret;

	f.width  = width;
	f.height = height;
	f.pitch  = pitch;
	f.bpp    = bpp;
	f.depth  = depth;
	f.handle = bo_handle;

	if ((ret = drmIoctl(fd, DRM_IOCTL_MODE_ADDFB, &f)))
		return ret;

	*buf_id = f.fb_id;
	return 0;
}

int drmModeRmFB(int fd, uint32_t bufferId)
{
	return drmIoctl(fd, DRM_IOCTL_MODE_RMFB, &bufferId);


}

drmModeFBPtr drmModeGetFB(int fd, uint32_t buf)
{
	struct drm_mode_fb_cmd info;
	drmModeFBPtr r;

	info.fb_id = buf;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETFB, &info))
		return NULL;

	if (!(r = drmMalloc(sizeof(*r))))
		return NULL;

	r->fb_id = info.fb_id;
	r->width = info.width;
	r->height = info.height;
	r->pitch = info.pitch;
	r->bpp = info.bpp;
	r->handle = info.handle;
	r->depth = info.depth;

	return r;
}


/*
 * Crtc functions
 */

drmModeCrtcPtr drmModeGetCrtc(int fd, uint32_t crtcId)
{
	struct drm_mode_crtc crtc;
	drmModeCrtcPtr r;

	crtc.crtc_id = crtcId;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCRTC, &crtc))
		return 0;

	/*
	 * return
	 */

	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->crtc_id         = crtc.crtc_id;
	r->x               = crtc.x;
	r->y               = crtc.y;
	r->mode_valid      = crtc.mode_valid;
	if (r->mode_valid)
		memcpy(&r->mode, &crtc.mode, sizeof(struct drm_mode_modeinfo));
	r->buffer_id       = crtc.fb_id;
	r->gamma_size      = crtc.gamma_size;
	return r;
}


int drmModeSetCrtc(int fd, uint32_t crtcId, uint32_t bufferId,
                   uint32_t x, uint32_t y, uint32_t *connectors, int count,
		   drmModeModeInfoPtr mode)
{
	struct drm_mode_crtc crtc;

	crtc.x             = x;
	crtc.y             = y;
	crtc.crtc_id       = crtcId;
	crtc.fb_id         = bufferId;
	crtc.set_connectors_ptr = VOID2U64(connectors);
	crtc.count_connectors = count;
	if (mode) {
	  memcpy(&crtc.mode, mode, sizeof(struct drm_mode_modeinfo));
	  crtc.mode_valid = 1;
	} else
	  crtc.mode_valid = 0;

	return drmIoctl(fd, DRM_IOCTL_MODE_SETCRTC, &crtc);
}

/*
 * Cursor manipulation
 */

int drmModeSetCursor(int fd, uint32_t crtcId, uint32_t bo_handle, uint32_t width, uint32_t height)
{
	struct drm_mode_cursor arg;

	arg.flags = DRM_MODE_CURSOR_BO;
	arg.crtc_id = crtcId;
	arg.width = width;
	arg.height = height;
	arg.handle = bo_handle;

	return drmIoctl(fd, DRM_IOCTL_MODE_CURSOR, &arg);
}

int drmModeMoveCursor(int fd, uint32_t crtcId, int x, int y)
{
	struct drm_mode_cursor arg;

	arg.flags = DRM_MODE_CURSOR_MOVE;
	arg.crtc_id = crtcId;
	arg.x = x;
	arg.y = y;

	return drmIoctl(fd, DRM_IOCTL_MODE_CURSOR, &arg);
}

/*
 * Encoder get
 */
drmModeEncoderPtr drmModeGetEncoder(int fd, uint32_t encoder_id)
{
	struct drm_mode_get_encoder enc;
	drmModeEncoderPtr r = NULL;

	enc.encoder_id = encoder_id;
	enc.encoder_type = 0;
	enc.possible_crtcs = 0;
	enc.possible_clones = 0;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETENCODER, &enc))
		return 0;

	if (!(r = drmMalloc(sizeof(*r))))
		return 0;

	r->encoder_id = enc.encoder_id;
	r->crtc_id = enc.crtc_id;
	r->encoder_type = enc.encoder_type;
	r->possible_crtcs = enc.possible_crtcs;
	r->possible_clones = enc.possible_clones;

	return r;
}

/*
 * Connector manipulation
 */

drmModeConnectorPtr drmModeGetConnector(int fd, uint32_t connector_id)
{
	struct drm_mode_get_connector conn;
	drmModeConnectorPtr r = NULL;

	conn.connector_id = connector_id;
	conn.connector_type_id = 0;
	conn.connector_type  = 0;
	conn.count_modes  = 0;
	conn.modes_ptr    = 0;
	conn.count_props  = 0;
	conn.props_ptr    = 0;
	conn.prop_values_ptr = 0;
	conn.count_encoders  = 0;
	conn.encoders_ptr = 0;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
		return 0;

	if (conn.count_props) {
		conn.props_ptr = VOID2U64(drmMalloc(conn.count_props*sizeof(uint32_t)));
		conn.prop_values_ptr = VOID2U64(drmMalloc(conn.count_props*sizeof(uint64_t)));
	}

	if (conn.count_modes)
		conn.modes_ptr = VOID2U64(drmMalloc(conn.count_modes*sizeof(struct drm_mode_modeinfo)));

	if (conn.count_encoders)
		conn.encoders_ptr = VOID2U64(drmMalloc(conn.count_encoders*sizeof(uint32_t)));

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn))
		goto err_allocs;

	if(!(r = drmMalloc(sizeof(*r)))) {
		goto err_allocs;
	}

	r->connector_id = conn.connector_id;
	r->encoder_id = conn.encoder_id;
	r->connection   = conn.connection;
	r->mmWidth      = conn.mm_width;
	r->mmHeight     = conn.mm_height;
	/* convert subpixel from kernel to userspace */
	r->subpixel     = conn.subpixel + 1;
	r->count_modes  = conn.count_modes;
	/* TODO we should test if these alloc & cpy fails. */
	r->count_props  = conn.count_props;
	r->props        = drmAllocCpy(U642VOID(conn.props_ptr), conn.count_props, sizeof(uint32_t));
	r->prop_values  = drmAllocCpy(U642VOID(conn.prop_values_ptr), conn.count_props, sizeof(uint64_t));
	r->modes        = drmAllocCpy(U642VOID(conn.modes_ptr), conn.count_modes, sizeof(struct drm_mode_modeinfo));
	r->count_encoders = conn.count_encoders;
	r->encoders     = drmAllocCpy(U642VOID(conn.encoders_ptr), conn.count_encoders, sizeof(uint32_t));
	r->connector_type  = conn.connector_type;
	r->connector_type_id = conn.connector_type_id;

	if (!r->props || !r->prop_values || !r->modes || !r->encoders)
		goto err_allocs;

err_allocs:
	drmFree(U642VOID(conn.prop_values_ptr));
	drmFree(U642VOID(conn.props_ptr));
	drmFree(U642VOID(conn.modes_ptr));
	drmFree(U642VOID(conn.encoders_ptr));

	return r;
}

int drmModeAttachMode(int fd, uint32_t connector_id, drmModeModeInfoPtr mode_info)
{
	struct drm_mode_mode_cmd res;

	memcpy(&res.mode, mode_info, sizeof(struct drm_mode_modeinfo));
	res.connector_id = connector_id;

	return drmIoctl(fd, DRM_IOCTL_MODE_ATTACHMODE, &res);
}

int drmModeDetachMode(int fd, uint32_t connector_id, drmModeModeInfoPtr mode_info)
{
	struct drm_mode_mode_cmd res;

	memcpy(&res.mode, mode_info, sizeof(struct drm_mode_modeinfo));
	res.connector_id = connector_id;

	return drmIoctl(fd, DRM_IOCTL_MODE_DETACHMODE, &res);
}


drmModePropertyPtr drmModeGetProperty(int fd, uint32_t property_id)
{
	struct drm_mode_get_property prop;
	drmModePropertyPtr r;

	prop.prop_id = property_id;
	prop.count_enum_blobs = 0;
	prop.count_values = 0;
	prop.flags = 0;
	prop.enum_blob_ptr = 0;
	prop.values_ptr = 0;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &prop))
		return 0;

	if (prop.count_values)
		prop.values_ptr = VOID2U64(drmMalloc(prop.count_values * sizeof(uint64_t)));

	if (prop.count_enum_blobs && (prop.flags & DRM_MODE_PROP_ENUM))
		prop.enum_blob_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(struct drm_mode_property_enum)));

	if (prop.count_enum_blobs && (prop.flags & DRM_MODE_PROP_BLOB)) {
		prop.values_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(uint32_t)));
		prop.enum_blob_ptr = VOID2U64(drmMalloc(prop.count_enum_blobs * sizeof(uint32_t)));
	}

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPERTY, &prop)) {
		r = NULL;
		goto err_allocs;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		return NULL;

	r->prop_id = prop.prop_id;
	r->count_values = prop.count_values;

	r->flags = prop.flags;
	if (prop.count_values)
		r->values = drmAllocCpy(U642VOID(prop.values_ptr), prop.count_values, sizeof(uint64_t));
	if (prop.flags & DRM_MODE_PROP_ENUM) {
		r->count_enums = prop.count_enum_blobs;
		r->enums = drmAllocCpy(U642VOID(prop.enum_blob_ptr), prop.count_enum_blobs, sizeof(struct drm_mode_property_enum));
	} else if (prop.flags & DRM_MODE_PROP_BLOB) {
		r->values = drmAllocCpy(U642VOID(prop.values_ptr), prop.count_enum_blobs, sizeof(uint32_t));
		r->blob_ids = drmAllocCpy(U642VOID(prop.enum_blob_ptr), prop.count_enum_blobs, sizeof(uint32_t));
		r->count_blobs = prop.count_enum_blobs;
	}
	strncpy(r->name, prop.name, DRM_PROP_NAME_LEN);
	r->name[DRM_PROP_NAME_LEN-1] = 0;

err_allocs:
	drmFree(U642VOID(prop.values_ptr));
	drmFree(U642VOID(prop.enum_blob_ptr));

	return r;
}

void drmModeFreeProperty(drmModePropertyPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->values);
	drmFree(ptr->enums);
	drmFree(ptr);
}

drmModePropertyBlobPtr drmModeGetPropertyBlob(int fd, uint32_t blob_id)
{
	struct drm_mode_get_blob blob;
	drmModePropertyBlobPtr r;

	blob.length = 0;
	blob.data = 0;
	blob.blob_id = blob_id;

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPBLOB, &blob))
		return NULL;

	if (blob.length)
		blob.data = VOID2U64(drmMalloc(blob.length));

	if (drmIoctl(fd, DRM_IOCTL_MODE_GETPROPBLOB, &blob)) {
		r = NULL;
		goto err_allocs;
	}

	if (!(r = drmMalloc(sizeof(*r))))
		return NULL;

	r->id = blob.blob_id;
	r->length = blob.length;
	r->data = drmAllocCpy(U642VOID(blob.data), 1, blob.length);

err_allocs:
	drmFree(U642VOID(blob.data));
	return r;
}

void drmModeFreePropertyBlob(drmModePropertyBlobPtr ptr)
{
	if (!ptr)
		return;

	drmFree(ptr->data);
	drmFree(ptr);
}

int drmModeConnectorSetProperty(int fd, uint32_t connector_id, uint32_t property_id,
			     uint64_t value)
{
	struct drm_mode_connector_set_property osp;
	int ret;

	osp.connector_id = connector_id;
	osp.prop_id = property_id;
	osp.value = value;

	if ((ret = drmIoctl(fd, DRM_IOCTL_MODE_SETPROPERTY, &osp)))
		return ret;

	return 0;
}

/*
 * checks if a modesetting capable driver has attached to the pci id
 * returns 0 if modesetting supported.
 *  -EINVAL or invalid bus id
 *  -ENOSYS if no modesetting support
*/
int drmCheckModesettingSupported(const char *busid)
{
#ifdef __linux__
	char pci_dev_dir[1024];
	int domain, bus, dev, func;
	DIR *sysdir;
	struct dirent *dent;
	int found = 0, ret;

	ret = sscanf(busid, "pci:%04x:%02x:%02x.%d", &domain, &bus, &dev, &func);
	if (ret != 4)
		return -EINVAL;

	sprintf(pci_dev_dir, "/sys/bus/pci/devices/%04x:%02x:%02x.%d/drm",
		domain, bus, dev, func);

	sysdir = opendir(pci_dev_dir);
	if (sysdir) {
		dent = readdir(sysdir);
		while (dent) {
			if (!strncmp(dent->d_name, "controlD", 8)) {
				found = 1;
				break;
			}

			dent = readdir(sysdir);
		}
		closedir(sysdir);
		if (found)
			return 0;
	}

	sprintf(pci_dev_dir, "/sys/bus/pci/devices/%04x:%02x:%02x.%d/",
		domain, bus, dev, func);

	sysdir = opendir(pci_dev_dir);
	if (!sysdir)
		return -EINVAL;

	dent = readdir(sysdir);
	while (dent) {
		if (!strncmp(dent->d_name, "drm:controlD", 12)) {
			found = 1;
			break;
		}

		dent = readdir(sysdir);
	}

	closedir(sysdir);
	if (found)
		return 0;
#endif
	return -ENOSYS;

}

int drmModeCrtcGetGamma(int fd, uint32_t crtc_id, uint32_t size,
			uint16_t *red, uint16_t *green, uint16_t *blue)
{
	int ret;
	struct drm_mode_crtc_lut l;

	l.crtc_id = crtc_id;
	l.gamma_size = size;
	l.red = VOID2U64(red);
	l.green = VOID2U64(green);
	l.blue = VOID2U64(blue);

	if ((ret = drmIoctl(fd, DRM_IOCTL_MODE_GETGAMMA, &l)))
		return ret;

	return 0;
}

int drmModeCrtcSetGamma(int fd, uint32_t crtc_id, uint32_t size,
			uint16_t *red, uint16_t *green, uint16_t *blue)
{
	int ret;
	struct drm_mode_crtc_lut l;

	l.crtc_id = crtc_id;
	l.gamma_size = size;
	l.red = VOID2U64(red);
	l.green = VOID2U64(green);
	l.blue = VOID2U64(blue);

	if ((ret = drmIoctl(fd, DRM_IOCTL_MODE_SETGAMMA, &l)))
		return ret;

	return 0;
}
