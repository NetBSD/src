/*	$NetBSD: clb069.h,v 1.2 2021/12/18 23:45:33 riastradh Exp $	*/

#ifndef __NVIF_CLB069_H__
#define __NVIF_CLB069_H__
struct nvif_clb069_v0 {
	__u8  version;
	__u8  pad01[3];
	__u32 entries;
	__u32 get;
	__u32 put;
};

#define NVB069_V0_NTFY_FAULT                                                0x00
#endif
