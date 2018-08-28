/*	$NetBSD: init.h,v 1.2 2018/08/27 04:58:30 riastradh Exp $	*/

#ifndef __NVBIOS_INIT_H__
#define __NVBIOS_INIT_H__

struct nvbios_init {
	struct nvkm_subdev *subdev;
	struct nvkm_bios *bios;
	u16 offset;
	struct dcb_output *outp;
	int crtc;

	/* internal state used during parsing */
	u8 execute;
	u32 nested;
	u16 repeat;
	u16 repend;
	u32 ramcfg;
};

int nvbios_exec(struct nvbios_init *);
int nvbios_init(struct nvkm_subdev *, bool execute);
#endif
