/* $NetBSD: memcreg.h,v 1.2.2.3 2001/04/21 17:53:12 bouyer Exp $ */
/*-
 * Copyright (c) 1997, 1998 Ben Harris
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * memcreg.h - Acorn/ARM MEMC (Anna/VC2304/VL2304/MEMC1A/VL2304A/VL86C110/VY86C110)
 * registers
 */

#ifndef _ARM26_MEMCREG_H
#define _ARM26_MEMCREG_H

/* 
 * Accessing the MEMC is a little odd.  It's not connected to the data
 * bus, so the register and the new value are coded into an address.
 * Thus, to set a register, OR together the register specifier and the
 * new value, and write any word to the resultant address.
 */

#define MEMC_WRITE(value) *(volatile u_int32_t *)value = 0

/*
 * This information is mostly derived from:
 * MEMC Datasheet
 * Published by Acorn Computers Limited.
 * Part no 0460,019
 * Issue No 1.0
 * 30 September 1986
 * ISBN 1 85250 025 6
 *
 * Thanks must go to Jeanette Draper at ARM Ltd
 * <jeanette.draper@arm.com> for finding it for me.
 *
 * Information on master/slave MEMCs came from Tony Duell
 * <ard@p850ug1.demon.co.uk>, who has a copy of the MEMC1A data
 * sheet and might photocopy it for me one day.
 */

/* General memory-map layout provided by MEMC */
#define MEMC_PHYS_BASE		((caddr_t)0x02000000)
#define MEMC_IO_BASE		((caddr_t)0x03000000)
#define MEMC_VIDC_BASE		((caddr_t)0x03400000)
#define MEMC_ROM_LOW_BASE	((caddr_t)0x03400000)
#define MEMC_ROM_HIGH_BASE	((caddr_t)0x03800000)

/*
 * Each MEMC can manage 4Mb in 128 pages.  The memory map only has
 * space for 16Mb of physical RAM.
 */
#define MEMC_MAX_PHYSPAGES 512

/*
 * DMA address generator control registers.  Addresses (>>4) go in
 * bits 2-16, and must thus be in the bottom 512k of physical RAM.
 */
#define MEMC_DMA_MAX		0x00080000
/* Video */
#define MEMC_VINIT		0x03600000
#define MEMC_VSTART		0x03620000
#define MEMC_VEND		0x03640000
/* Cursor */
#define MEMC_CINIT		0x03660000
/* Sound */
#define MEMC_SSTARTN		0x03680000
#define MEMC_SENDN		0x036A0000
#define MEMC_SPTR		0x036C0000
#define MEMC_SET_PTR(reg,addr)	(reg | (addr >> 2))

/* MEMC control register (sec 6.5) */
#define MEMC_CONTROL		0x036E0000

/* Page size */
#define MEMC_CTL_PGSZ_MASK	0x0000000C
#define MEMC_CTL_PGSZ_4K	0x00000000
#define MEMC_CTL_PGSZ_8K	0x00000004
#define MEMC_CTL_PGSZ_16K	0x00000008
#define MEMC_CTL_PGSZ_32K	0x0000000C

/* ROM speeds; low and high banks, relative to RAM speed */
#define MEMC_CTL_LROMSPD_MASK	0x00000030
#define MEMC_CTL_LROMSPD_4N	0x00000000
#define MEMC_CTL_LROMSPD_3N	0x00000010
#define MEMC_CTL_LROMSPD_2N	0x00000020
#define MEMC_CTL_LROMSPD_PAGED	0x00000030

#define MEMC_CTL_HROMSPD_MASK	0x000000C0
#define MEMC_CTL_HROMSPD_4N	0x00000000
#define MEMC_CTL_HROMSPD_3N	0x00000040
#define MEMC_CTL_HROMSPD_2N	0x00000080
#define MEMC_CTL_HROMSPD_PAGED	0x000000C0

/* DRAM refresh control */
#define MEMC_CTL_RFRSH_MASK	0x00000300
#define MEMC_CTL_RFRSH_NONE	0x00000000
#define MEMC_CTL_RFRSH_FLYBACK	0x00000100
#define MEMC_CTL_RFRSH_CONTIN	0x00000300

/* Enable video DMA */
#define MEMC_CTL_VIDEODMA	0x00000400
/* Enable sound DMA */
#define MEMC_CTL_SOUNDDMA	0x00000800
/* Operating System Mode Select */
#define MEMC_CTL_OSMODE		0x00001000

/* Test mode */
/* This should never be set in a running system */
#define MEMC_CTL_TESTMODE	0x00002000

/*
 * Address translation control
 */

/* Absolute address of translation table */
#define MEMC_TRANS_BASE		0x03800000

/*
 * MEMC translation entries are painful, and vary with the page size
 * in use.  Here, ppn is the physical page number, lpn is the logical
 * page number and ppl is the page protection level.
 *
 * The list of transformations at the start of each macro is copied
 * verbatim from the MEMC datasheet (Figure 5) with the exception of
 * the entries for PPN[7] and PPN[8].  In dual-MEMC situations, PPN[7]
 * selects between master and slave MEMCs, and is mapped to A[7] whatever
 * the page size (though Acorn machines always use 32k pages with dual
 * MEMCs).  The Archimedes 540 can have up to 16Mb of RAM, and arranges
 * this by having several address lines go through PALs on their way to the
 * MEMCs.  The upshot of this is that for the purposes of setting the
 * translation tables, PPN[8] maps to A[12].  The A540 always has 32kb pages.
 */

/* Page protection levels (data sheet section 6.6) */
/*-
 *              PPL
 * Mode   00  01  10  11       
 * SVC    R/W R/W R/W R/W
 * OS     R/W R/W R   R
 * User   R/W R   -   -
 */

#define MEMC_PPL_RDWR		0
#define MEMC_PPL_RDONLY		1
#define MEMC_PPL_NOACCESS	2

/*-
 * 4k pages:
 * PPN[7] -> A[7]	(MEMC1a)
 * PPN[6:0] -> A[6:0]
 * PPL[1:0] -> A[9:8]
 * LPN[12:11] -> A[11:10]
 * LPN[10:0] -> A[22:12]
 */
#define MEMC_TRANS_ENTRY_4K(ppn, lpn, ppl)			\
	(MEMC_TRANS_BASE |					\
	 ((ppn) & 0xff) | 					\
	 ((ppl) & 0x3) << 8 | 					\
	 ((lpn) & 0x7ff) << 12 |				\
	 ((lpn) & 0x1800) >> 1)
/*-
 * 8k pages:
 * PPN[7] -> A[7]	(MEMC1a)
 * PPN[6] -> A[0]
 * PPN[5:0] -> A[6:1]
 * PPL[1:0] -> A[9:8]
 * LPN[11:10] -> A[11:10]
 * LPN[9:0] -> A[22:13]
 */
#define MEMC_TRANS_ENTRY_8K(ppn, lpn, ppl)			\
	(MEMC_TRANS_BASE |					\
	 ((ppn) & 0x80) |					\
	 ((ppn) & 0x40) >> 6 |					\
	 ((ppn) & 0x3f) << 1 |					\
	 ((ppl) & 0x3) << 8 |					\
	 ((lpn) & 0xc00)) |					\
	 ((lpn) & 0x3ff) << 13)
/*-
 * 16k pages:
 * PPN[7] -> A[7]	(MEMC1a)
 * PPN[6:5] -> A[1:0]
 * PPN[4:0] -> A[6:2]
 * PPL[1:0] -> A[9:8]
 * LPN[10:9] -> A[11:10]
 * LPN[9:0] -> A[22:14]
 */
#define MEMC_TRANS_ENTRY_16K(ppn, lpn, ppl)			\
	(MEMC_TRANS_BASE |					\
	 ((ppn) & 0x80) |					\
	 ((ppn) & 0x60) >> 5 |					\
	 ((ppn) & 0x1f) << 2 |					\
	 ((ppl) & 0x3) << 8 |					\
	 ((lpn) & 0x600) << 1 |					\
	 ((lpn) & 0x1ff) << 14)
/*-
 * 32k pages (here, the MEMC descends into madness...):
 * PPN[8] -> A[12]	(A540)
 * PPN[7] -> A[7]	(MEMC1a)
 * PPN[6] -> A[1]
 * PPN[5] -> A[2]
 * PPN[4] -> A[0]
 * PPN[3:0] -> A[6:3]
 * PPL[1:0] -> A[9:8]
 * LPN[9:8] -> A[11:10]
 * LPN[7:0] -> A[22:15]
 */
#define MEMC_TRANS_ENTRY_32K(ppn, lpn, ppl)			\
	(MEMC_TRANS_BASE |					\
	 ((ppn) & 0x100) << 4 |					\
	 ((ppn) & 0x80) |					\
	 ((ppn) & 0x40) >> 5 |					\
	 ((ppn) & 0x20) >> 3 |					\
	 ((ppn) & 0x10) >> 4 |					\
	 ((ppn) & 0x0f) << 3 |					\
	 ((ppl) & 0x3) << 8 |					\
	 ((lpn) & 0x300) << 2 |					\
	 ((lpn) & 0x0ff) << 15)

#endif
