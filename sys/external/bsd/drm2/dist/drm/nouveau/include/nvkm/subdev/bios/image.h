/*	$NetBSD: image.h,v 1.2.6.2 2019/06/10 22:08:15 christos Exp $	*/

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
