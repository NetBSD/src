/*	$NetBSD: pcir.h,v 1.2 2018/08/27 04:58:30 riastradh Exp $	*/

#ifndef __NVBIOS_PCIR_H__
#define __NVBIOS_PCIR_H__
struct nvbios_pcirT {
	u16 vendor_id;
	u16 device_id;
	u8  class_code[3];
	u32 image_size;
	u16 image_rev;
	u8  image_type;
	bool last;
};

u32 nvbios_pcirTe(struct nvkm_bios *, u32, u8 *ver, u16 *hdr);
u32 nvbios_pcirTp(struct nvkm_bios *, u32, u8 *ver, u16 *hdr,
		  struct nvbios_pcirT *);
#endif
