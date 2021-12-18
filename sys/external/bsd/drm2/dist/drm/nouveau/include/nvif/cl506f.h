/*	$NetBSD: cl506f.h,v 1.2 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CL506F_H__
#define __NVIF_CL506F_H__

struct nv50_channel_gpfifo_v0 {
	__u8  version;
	__u8  chid;
	__u8  pad02[2];
	__u32 ilength;
	__u64 ioffset;
	__u64 pushbuf;
	__u64 vmm;
};
#endif
