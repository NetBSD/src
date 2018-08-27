/*	$NetBSD: timing.h,v 1.2 2018/08/27 04:58:30 riastradh Exp $	*/

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
