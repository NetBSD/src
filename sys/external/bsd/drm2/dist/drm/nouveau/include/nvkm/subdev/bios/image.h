/*	$NetBSD: image.h,v 1.3 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
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
