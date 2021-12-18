/*	$NetBSD: cl507c.h,v 1.2 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CL507C_H__
#define __NVIF_CL507C_H__

struct nv50_disp_base_channel_dma_v0 {
	__u8  version;
	__u8  head;
	__u8  pad02[6];
	__u64 pushbuf;
};

#define NV50_DISP_BASE_CHANNEL_DMA_V0_NTFY_UEVENT                          0x00
#endif
