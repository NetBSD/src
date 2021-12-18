/*	$NetBSD: cl507d.h,v 1.2 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CL507D_H__
#define __NVIF_CL507D_H__

struct nv50_disp_core_channel_dma_v0 {
	__u8  version;
	__u8  pad01[7];
	__u64 pushbuf;
};

#define NV50_DISP_CORE_CHANNEL_DMA_V0_NTFY_UEVENT                          0x00
#endif
