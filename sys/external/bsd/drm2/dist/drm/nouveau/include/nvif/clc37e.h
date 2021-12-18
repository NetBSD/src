/*	$NetBSD: clc37e.h,v 1.2 2021/12/18 23:45:33 riastradh Exp $	*/

/* SPDX-License-Identifier: MIT */
#ifndef __NVIF_CLC37E_H__
#define __NVIF_CLC37E_H__

struct nvc37e_window_channel_dma_v0 {
	__u8  version;
	__u8  index;
	__u8  pad02[6];
	__u64 pushbuf;
};

#define NVC37E_WINDOW_CHANNEL_DMA_V0_NTFY_UEVENT                           0x00
#endif
