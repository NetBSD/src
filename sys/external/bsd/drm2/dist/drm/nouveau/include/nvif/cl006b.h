/*	$NetBSD: cl006b.h,v 1.1.1.1 2021/12/18 20:15:37 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CL006B_H__
#define __NVIF_CL006B_H__

struct nv03_channel_dma_v0 {
	__u8  version;
	__u8  chid;
	__u8  pad02[2];
	__u32 offset;
	__u64 pushbuf;
};
#endif
