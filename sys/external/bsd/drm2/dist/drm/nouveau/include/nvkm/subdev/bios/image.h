/*	$NetBSD: image.h,v 1.1.1.1 2018/08/27 01:34:55 riastradh Exp $	*/

#ifndef __NVBIOS_IMAGE_H__
#define __NVBIOS_IMAGE_H__
struct nvbios_image {
	u32  base;
	u32  size;
	u8   type;
	bool last;
};

bool nvbios_image(struct nvkm_bios *, int, struct nvbios_image *);
#endif
