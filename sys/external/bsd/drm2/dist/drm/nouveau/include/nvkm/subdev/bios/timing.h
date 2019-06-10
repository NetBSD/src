/*	$NetBSD: timing.h,v 1.2.6.2 2019/06/10 22:08:15 christos Exp $	*/

#ifndef __NVBIOS_TIMING_H__
#define __NVBIOS_TIMING_H__
#include <subdev/bios/ramcfg.h>

u16 nvbios_timingTe(struct nvkm_bios *,
		    u8 *ver, u8 *hdr, u8 *cnt, u8 *len, u8 *snr, u8 *ssz);
u16 nvbios_timingEe(struct nvkm_bios *, int idx,
		    u8 *ver, u8 *hdr, u8 *cnt, u8 *len);
u16 nvbios_timingEp(struct nvkm_bios *, int idx,
		    u8 *ver, u8 *hdr, u8 *cnt, u8 *len, struct nvbios_ramcfg *);
#endif
