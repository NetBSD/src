/*	$NetBSD: cl506e.h,v 1.2 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CL506E_H__
#define __NVIF_CL506E_H__

struct nv50_channel_dma_v0 {
	__u8  version;
	__u8  chid;
	__u8  pad02[6];
	__u64 vmm;
	__u64 pushbuf;
	__u64 offset;
};
#endif
