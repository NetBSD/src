/*	$NetBSD: veritefbio.h,v 1.1 2026/07/11 15:18:21 rkujawa Exp $	*/

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

struct veritefb_dbg_diag {
	uint32_t	vd_accel;	/* 0 off, 1 sw-degraded, 2 on */
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

/* accumulated per-op statistics */
struct veritefb_dbg_stats {
	uint64_t	vs_count[8];
	uint64_t	vs_us[8];
};
#define VFB_STAT_FILL		0	/* eraserows/erasecols */
#define VFB_STAT_BLT		1	/* copyrows/copycols */
#define VFB_STAT_CHAR_SPACE	2	/* putchar: space rectfill */
#define VFB_STAT_CHAR_HIT	3	/* putchar: glyphcache hit */
#define VFB_STAT_CHAR_ADD	4	/* putchar: SW render + cache add */
#define VFB_STAT_CHAR_SW	5	/* putchar: SW fallback */

#define VERITEFB_DBG_STATS	_IOR('V', 108, struct veritefb_dbg_stats)
#define VERITEFB_DBG_STATCLR	_IO('V', 109)

/* raw byte access to the chip register window (offset < 0x100) */
#define VERITEFB_DBG_IO_IOSPACE	0x1000	/* vr_addr flag: force the I/O BAR */
#define VERITEFB_DBG_RDIO	_IOWR('V', 110, struct veritefb_dbg_rw)
#define VERITEFB_DBG_WRIO	_IOW('V', 111, struct veritefb_dbg_rw)

#endif /* VERITEFBIO_H */
