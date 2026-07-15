/*	$NetBSD: veritefbio.h,v 1.2 2026/07/15 20:53:22 rkujawa Exp $	*/

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
 * veritefb RISC debug ioctl interface.
 *
 * Only available in kernels built with the VERITEFB_DEBUG.
 */

#ifndef VERITEFBIO_H
#define VERITEFBIO_H

#include <sys/ioccom.h>

#define VERITEFB_DIAG_RING	16

/* who owns the RISC (vd_owner; kernel uses the same values) */
#define VERITEFB_OWNER_CONSOLE	0	/* 2D console microcode */
#define VERITEFB_OWNER_PARKED	1	/* parked in the csucode monitor */
#define VERITEFB_OWNER_FOREIGN	2	/* foreign (3D/GL) microcode */

struct veritefb_dbg_diag {
	uint32_t	vd_accel;	/* 0 off, 1 sw-degraded, 2 on */
	uint32_t	vd_owner;	/* VERITEFB_OWNER_* */
	uint32_t	vd_pc;		/* sampled RISC program counter */
	uint32_t	vd_fifoinfree;
	uint32_t	vd_fifooutvalid;
	uint32_t	vd_debugreg;
	uint32_t	vd_heartbeat;	/* 0 ok, 1 skipped, 2 failed */
	uint32_t	vd_ringcount;	/* total FIFO words ever written */
	uint32_t	vd_ring[VERITEFB_DIAG_RING]; /* newest last */
};

struct veritefb_dbg_rw {
	uint32_t	vr_addr;	/* register index or address */
	uint32_t	vr_val;
};

#define VERITEFB_DBG_DIAG	_IOR('V', 100, struct veritefb_dbg_diag)
#define VERITEFB_DBG_HOLD	_IO('V', 101)
#define VERITEFB_DBG_CONT	_IO('V', 102)
#define VERITEFB_DBG_RDREG	_IOWR('V', 103, struct veritefb_dbg_rw)
#define VERITEFB_DBG_RDMEM	_IOWR('V', 104, struct veritefb_dbg_rw)
#define VERITEFB_DBG_FAULT	_IO('V', 105)	/* wedge on purpose */
#define VERITEFB_DBG_RESET	_IOR('V', 106, int) /* reload + restart */

/*
 * Accumulated per-op statistics.
 */
struct veritefb_dbg_stats {
	uint64_t	vs_count[16];
	uint64_t	vs_us[16];
};
#define VFB_STAT_FILL		0	/* eraserows/erasecols */
#define VFB_STAT_BLT		1	/* copyrows/copycols */
#define VFB_STAT_CHAR_SPACE	2	/* putchar: space rectfill */
#define VFB_STAT_CHAR_HIT	3	/* putchar: glyphcache hit */
#define VFB_STAT_CHAR_ADD	4	/* putchar: SW render + cache add */
#define VFB_STAT_CHAR_SW	5	/* putchar: SW fallback */
#define VFB_STAT_SYNC		6	/* engine sync round-trips */
/*
 * 3D path
 */
#define VFB_STAT_V3D_SUBMIT	7	/* dma_submit body */
#define VFB_STAT_V3D_SPIN	8	/* drain DMABusy wait only */
#define VFB_STAT_V3D_SYNC	9	/* V3D_SYNC FIFO round-trip */
#define VFB_STAT_V3D_FLIP	10	/* V3D_FLIP vsync wait only */
#define VFB_STAT_V3D_BYTES	11	/* count += submitted bytes */
#define VFB_STAT_V3D_DRAIN_BUSY	12	/* drain found master busy */
#define VFB_STAT_V3D_DRAIN_IDLE	13	/* drain found master done */

#define VERITEFB_DBG_STATS	_IOR('V', 108, struct veritefb_dbg_stats)
#define VERITEFB_DBG_STATCLR	_IO('V', 109)

/* raw byte access to the chip register window (offset < 0x100) */
#define VERITEFB_DBG_IO_IOSPACE	0x1000	/* vr_addr flag: force the I/O BAR */
#define VERITEFB_DBG_RDIO	_IOWR('V', 110, struct veritefb_dbg_rw)
#define VERITEFB_DBG_WRIO	_IOW('V', 111, struct veritefb_dbg_rw)

#define VERITEFB_DBG_SUSPEND2D	_IO('V', 112)
#define VERITEFB_DBG_RESUME2D	_IO('V', 113)

struct veritefb_dbg_ucload {
	uint32_t	vu_size;	/* image size in bytes */
	const void	*vu_data;	/* userland image pointer */
	uint32_t	vu_entry;	/* out: entry point */
};
#define VERITEFB_DBG_UCLOAD	_IOWR('V', 114, struct veritefb_dbg_ucload)

#define VFB_DBG_FIFO_MAX	16
/* vf_setowner values: keep, or switch the recorded RISC owner */
#define VERITEFB_SETOWNER_KEEP	0xffffffffU
struct veritefb_dbg_fifowr {
	uint32_t	vf_count;	/* 1..VFB_DBG_FIFO_MAX */
	uint32_t	vf_setowner;	/* VERITEFB_OWNER_* or KEEP */
	uint32_t	vf_words[VFB_DBG_FIFO_MAX];
};
#define VERITEFB_DBG_FIFOWR	_IOW('V', 115, struct veritefb_dbg_fifowr)

struct veritefb_dbg_fiford {
	uint32_t	vf_timo_us;	/* bounded wait; clamped by kernel */
	uint32_t	vf_word;	/* out */
};
#define VERITEFB_DBG_FIFORD	_IOWR('V', 116, struct veritefb_dbg_fiford)

/* 32-bit VRAM peek in [0, VFB_MC_SIZE), MEMENDIAN-stable */
#define VERITEFB_DBG_RDVRAM	_IOWR('V', 117, struct veritefb_dbg_rw)

/*
 * Scanout depth switch for 3D experiments: 8 (console) or 16 (565).
 * Only while the console microcode is parked; resume2d restores 8.
 */
#define VERITEFB_DBG_SETDEPTH	_IOW('V', 118, uint32_t)

/*
 * Bus-master into the input FIFO.
 */
struct veritefb_dbg_dma {
	uint64_t	vd_buf;		/* user buffer address */
	uint32_t	vd_len;		/* bytes */
	uint32_t	vd_swap;	/* 0..3 */
};
#define VERITEFB_DBG_DMASUBMIT	_IOW('V', 119, struct veritefb_dbg_dma)

/*
 * RISC program-counter sampling profiler.
 */
#define VFB_PCHIST_BUCKETS	512
struct veritefb_dbg_pcsample {
	uint32_t	vp_hz;		/* 0 stop; else clamped to [1, kern hz] */
	uint32_t	vp_base;	/* PC of bucket 0 */
	uint32_t	vp_shift;	/* PC units per bucket = 1 << vp_shift */
};
#define VERITEFB_DBG_PCSAMPLE	_IOW('V', 120, struct veritefb_dbg_pcsample)

struct veritefb_dbg_pchist {
	uint64_t	vp_samples;	/* total PC samples taken */
	uint64_t	vp_missed;	/* samples outside the window */
	uint32_t	vp_base;	/* echoed window base */
	uint32_t	vp_shift;	/* echoed window shift */
	uint32_t	vp_min;		/* lowest raw PC seen (~0u if none) */
	uint32_t	vp_max;		/* highest raw PC seen */
	uint32_t	vp_hist[VFB_PCHIST_BUCKETS];
};
#define VERITEFB_DBG_PCHIST	_IOR('V', 121, struct veritefb_dbg_pchist)

#endif /* VERITEFBIO_H */
