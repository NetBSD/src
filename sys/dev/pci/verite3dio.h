/*	$NetBSD: verite3dio.h,v 1.1 2026/07/15 20:53:22 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * /dev/verite3d - expierimental 3D interface for veritefb(4)
 *
 * By design, it is completely independent of wsdisplay(4). It supports
 * one exclusive client - opening the device stops the 2D microcode.
 * Closing the device stops the 3D microcode and restores the console,
 * in the state it was left behind.
 *
 * The client supplies its own RISC microcode (V3D_INIT) and owns 
 * the FIFO command stream. The kernel moves bytes (DMA) and tracks VRAM, 
 * but does NOT interpret the command stream. That is responsibility of
 * the microcode (on the hardware side) and the userland process.
 */

#ifndef VERITE3DIO_H
#define VERITE3DIO_H

#include <sys/ioccom.h>

#define V3D_RING_SLOTS	4
#define V3D_SLOT_SIZE	65536

#define V3D_VRAM_MMAP_OFF	((off_t)V3D_RING_SLOTS * V3D_SLOT_SIZE)

/* Load client microcode into the foreign slot and cold-start it. */
struct v3d_init {
	uint64_t	vi_ucode;	/* in: user pointer to image */
	uint32_t	vi_size;	/* in: bytes */
	uint32_t	vi_ctx_base;	/* out: context save area (VRAM) */
	uint32_t	vi_memsize;	/* out: total VRAM bytes */
	uint32_t	vi_pool_base;	/* out: first V3D_ALLOC-managed byte */
};
#define V3D_INIT	_IOWR('V', 0, struct v3d_init)

struct v3d_submit {
	uint32_t	vs_slot;	/* 0 to V3D_RING_SLOTS-1 */
	uint32_t	vs_len;		/* bytes, word multiple */
	uint32_t	vs_swap;	/* descriptor swap code 0..3 */
};
#define V3D_SUBMIT	_IOW('V', 1, struct v3d_submit)

/*
 * Fence: send one word (the client microcode's sync command) and
 * wait for the response word. In/out: word to send / word received.
 */
#define V3D_SYNC	_IOWR('V', 2, uint32_t)

struct v3d_alloc {
	uint32_t	va_size;	/* in: bytes */
	uint32_t	va_align;	/* in: byte alignment (0 = 4) */
	uint32_t	va_addr;	/* out: VRAM byte address */
};
#define V3D_ALLOC	_IOWR('V', 3, struct v3d_alloc)
#define V3D_FREE	_IOW('V', 4, uint32_t)

struct v3d_mode {
	uint32_t	vm_depth;	/* in: 8 or 16; 0 = query only */
	uint32_t	vm_frame_base;	/* in: scanout base; 0 = keep */
	uint32_t	vm_width;	/* out */
	uint32_t	vm_height;	/* out */
	uint32_t	vm_stride;	/* out: scanout line bytes */
	uint32_t	vm_pe_stride;	/* out: PE Stride(3) dst encoding */
};
#define V3D_MODE	_IOWR('V', 5, struct v3d_mode)

/*
 * vsync-waited FRAMEBASE flip.
 */
#define V3D_FLIP_NOWAIT	0x80000000U
#define V3D_FLIP	_IOW('V', 6, uint32_t)

#endif /* VERITE3DIO_H */
