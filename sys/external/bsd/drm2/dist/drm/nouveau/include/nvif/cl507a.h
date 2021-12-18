/*	$NetBSD: cl507a.h,v 1.1.1.1 2021/12/18 20:15:37 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CL507A_H__
#define __NVIF_CL507A_H__

struct nv50_disp_cursor_v0 {
	__u8  version;
	__u8  head;
	__u8  pad02[6];
};

#define NV50_DISP_CURSOR_V0_NTFY_UEVENT                                    0x00
#endif
