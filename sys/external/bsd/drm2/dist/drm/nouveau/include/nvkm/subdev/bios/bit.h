/*	$NetBSD: bit.h,v 1.2.6.2 2019/06/10 22:08:15 christos Exp $	*/

#ifndef __NVBIOS_BIT_H__
#define __NVBIOS_BIT_H__
struct bit_entry {
	u8  id;
	u8  version;
	u16 length;
	u16 offset;
};

int bit_entry(struct nvkm_bios *, u8 id, struct bit_entry *);
#endif
