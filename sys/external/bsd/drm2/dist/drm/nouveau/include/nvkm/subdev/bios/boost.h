/*	$NetBSD: boost.h,v 1.1.1.1 2018/08/27 01:34:55 riastradh Exp $	*/

#ifndef __NVBIOS_BOOST_H__
#define __NVBIOS_BOOST_H__
u16 nvbios_boostTe(struct nvkm_bios *, u8 *, u8 *, u8 *, u8 *, u8 *, u8 *);

struct nvbios_boostE {
	u8  pstate;
	u32 min;
	u32 max;
};

u16 nvbios_boostEe(struct nvkm_bios *, int idx, u8 *, u8 *, u8 *, u8 *);
u16 nvbios_boostEp(struct nvkm_bios *, int idx, u8 *, u8 *, u8 *, u8 *,
		   struct nvbios_boostE *);
u16 nvbios_boostEm(struct nvkm_bios *, u8, u8 *, u8 *, u8 *, u8 *,
		   struct nvbios_boostE *);

struct nvbios_boostS {
	u8  domain;
	u8  percent;
	u32 min;
	u32 max;
};

u16 nvbios_boostSe(struct nvkm_bios *, int, u16, u8 *, u8 *, u8, u8);
u16 nvbios_boostSp(struct nvkm_bios *, int, u16, u8 *, u8 *, u8, u8,
		   struct nvbios_boostS *);
#endif
