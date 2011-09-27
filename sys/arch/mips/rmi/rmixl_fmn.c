/*	$NetBSD: rmixl_fmn.c,v 1.4 2011/09/27 01:02:34 jym Exp $	*/
/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ddb.h"

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <mips/cpuregs.h>
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_fmnvar.h>

#ifdef FMN_DEBUG
# define DPRINTF(x)	do { printf x ; } while(0)
#else
# define DPRINTF(x)
#endif

#ifdef DIAGNOSTIC
# define DIAG_PRF(x)	do { printf x ; } while(0)
#else
# define DIAG_PRF(x)
#endif



/*
 * index CPU-dependent table by (global) bucket ID to obtain logical Station ID
 * see Table 12.1 in the XLS PRM
 */
/* use this table for XLS6xx, XLS4xx */
static const int station_xls_4xx[] = {
	[0 ... 7]	= RMIXLS_FMN_STID_CORE0,
	[8 ... 15]	= RMIXLS_FMN_STID_CORE1,
	[16 ... 23]	= RMIXLS_FMN_STID_CORE2,
	[24 ... 31]	= RMIXLS_FMN_STID_CORE3,
	[32 ... 63]	= RMIXLS_FMN_STID_RESERVED,
	[64 ... 71]	= RMIXLS_FMN_STID_PCIE,
	[72 ... 79]	= RMIXLS_FMN_STID_RESERVED,
	[80 ... 87]	= RMIXLS_FMN_STID_GMAC_Q1,
	[88 ... 95]	= RMIXLS_FMN_STID_RESERVED,
	[96 ... 103]	= RMIXLS_FMN_STID_GMAC_Q0,
	[104 ... 107]	= RMIXLS_FMN_STID_DMA,
	[108 ... 109]	= RMIXLS_FMN_STID_CDE,
	[110 ... 119]	= RMIXLS_FMN_STID_RESERVED,
	[120 ... 127]	= RMIXLS_FMN_STID_SAE,
};

/* use this table for XLS408Lite, XLS404Lite */
static const int station_xls_4xx_lite[] = {
	[0 ... 7]	= RMIXLS_FMN_STID_CORE0,
	[8 ... 15]	= RMIXLS_FMN_STID_CORE1,
	[16 ... 23]	= RMIXLS_FMN_STID_CORE2,
	[24 ... 31]	= RMIXLS_FMN_STID_CORE3,
	[32 ... 79]	= RMIXLS_FMN_STID_RESERVED,
	[80 ... 87]	= RMIXLS_FMN_STID_GMAC_Q1,
	[88 ... 95]	= RMIXLS_FMN_STID_RESERVED,
	[96 ... 103]	= RMIXLS_FMN_STID_GMAC_Q0,
	[104 ... 107]	= RMIXLS_FMN_STID_DMA,
	[108 ... 109]	= RMIXLS_FMN_STID_CDE,
	[110 ... 115]	= RMIXLS_FMN_STID_RESERVED,
	[116 ... 119]	= RMIXLS_FMN_STID_PCIE,
	[120 ... 127]	= RMIXLS_FMN_STID_SAE,
};

/* use this table for XLS2xx */
static const int station_xls_2xx[] = {
	[0 ... 7]	= RMIXLS_FMN_STID_CORE0,
	[8 ... 15]	= RMIXLS_FMN_STID_CORE1,
	[16 ... 23]	= RMIXLS_FMN_STID_CORE2,
	[24 ... 31]	= RMIXLS_FMN_STID_CORE3,
	[32 ... 63]	= RMIXLS_FMN_STID_RESERVED,
	[64 ... 71]	= RMIXLS_FMN_STID_PCIE,
	[72 ... 95]	= RMIXLS_FMN_STID_RESERVED,
	[96 ... 103]	= RMIXLS_FMN_STID_GMAC_Q0,
	[104 ... 107]	= RMIXLS_FMN_STID_DMA,
	[108 ... 119]	= RMIXLS_FMN_STID_RESERVED,
	[120 ... 127]	= RMIXLS_FMN_STID_SAE,
};

/* use this table for XLS1xx */
static const int station_xls_1xx[] = {
	[0 ... 7]	= RMIXLS_FMN_STID_CORE0,
	[8 ... 15]	= RMIXLS_FMN_STID_CORE1,
	[16 ... 23]	= RMIXLS_FMN_STID_CORE2,
	[24 ... 31]	= RMIXLS_FMN_STID_CORE3,
	[32 ... 63]	= RMIXLS_FMN_STID_RESERVED,
	[64 ... 71]	= RMIXLS_FMN_STID_PCIE,
	[72 ... 95]	= RMIXLS_FMN_STID_RESERVED,
	[96 ... 101]	= RMIXLS_FMN_STID_GMAC_Q0,
	[102 ... 103]	= RMIXLS_FMN_STID_RESERVED,
	[104 ... 107]	= RMIXLS_FMN_STID_DMA,
	[108 ... 119]	= RMIXLS_FMN_STID_RESERVED,
	[120 ... 127]	= RMIXLS_FMN_STID_SAE,
};

/* use this table for XLRxxx */
static const int station_xlr_xxx[] = {
	[0 ... 7]	= RMIXLR_FMN_STID_CORE0,
	[8 ... 15]	= RMIXLR_FMN_STID_CORE1,
	[16 ... 23]	= RMIXLR_FMN_STID_CORE2,
	[24 ... 31]	= RMIXLR_FMN_STID_CORE3,
	[32 ... 39]	= RMIXLR_FMN_STID_CORE4,
	[40 ... 47]	= RMIXLR_FMN_STID_CORE5,
	[48 ... 55]	= RMIXLR_FMN_STID_CORE6,
	[56 ... 63]	= RMIXLR_FMN_STID_CORE7,
	[64 ... 79]	= RMIXLR_FMN_STID_TXRX_0,
	[80 ... 95]	= RMIXLR_FMN_STID_TXRX_1,
	[96 ... 103]	= RMIXLR_FMN_STID_RGMII,
	[104 ... 107]	= RMIXLR_FMN_STID_DMA,
	[108 ... 111]	= RMIXLR_FMN_STID_RESERVED,
	[112 ... 113]	= RMIXLR_FMN_STID_FREE_0,
	[114 ... 115]	= RMIXLR_FMN_STID_FREE_0,
	[116 ... 119]	= RMIXLR_FMN_STID_RESERVED,
	[120 ... 127]	= RMIXLR_FMN_STID_SAE,
};

typedef struct fmn_station_info {
	const char     *si_name;
	const u_int	si_buckets_max;
	const u_int	si_stid_first;
	const u_int	si_stid_last;
	const u_int	si_bucket_size_dflt;
	const u_int	si_credits_min;
	const u_int	si_regbase;
} fmn_station_info_t;

/* use this table for XLS6xx, XLS4xx */
static const fmn_station_info_t station_info_xls_4xx[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	8,   0,   7,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	8,   8,  15,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	8,  16,  23,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	8,  24,  31,  32, 4, 0 },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0",	3,  80,  87,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_GMAC_Q1] = { "gmac_q1",	3,  96, 103,  32, 0, RMIXL_IO_DEV_GMAC_4 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	4, 104, 107,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLS_FMN_STID_CDE]     = { "cde",	4, 108, 109, 128, 0, RMIXL_IO_DEV_CDE },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	8,  64,  71,  32, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	2, 120, 121, 128, 0, RMIXL_IO_DEV_SAE },
};

/* use this table for XLS4xxLite */
static const fmn_station_info_t station_info_xls_4xx_lite[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	8,   0,   7,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	8,   8,  15,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	8,  16,  23,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	8,  24,  31,  32, 4, 0 },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0",	3,  80,  87,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_GMAC_Q1] = { "gmac_q1",	3,  96, 103,  32, 0, RMIXL_IO_DEV_GMAC_4 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	4, 104, 107,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLS_FMN_STID_CDE]     = { "cde",	4, 108, 109, 128, 0, RMIXL_IO_DEV_CDE },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	4, 116, 119,  64, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	2, 120, 121, 128, 0, RMIXL_IO_DEV_SAE },
};

/* use this table for XLS2xx */
static const fmn_station_info_t station_info_xls_2xx[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	8,   0,   7,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	8,   8,  15,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	8,  16,  23,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	8,  24,  31,  32, 4, 0 },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0",	3,  96, 103,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	4, 104, 107,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	8,  64,  71,  32, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	2, 120, 121, 128, 0, RMIXL_IO_DEV_SAE },
};

/* use this table for XLS1xx */
static const fmn_station_info_t station_info_xls_1xx[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	8,   0,   7,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	8,   8,  15,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	8,  16,  23,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	8,  24,  31,  32, 4, 0 },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0",	3,  96, 101,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	4, 104, 107,  64, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	4,  64,  67,  32, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	2, 120, 121, 128, 0, RMIXL_IO_DEV_SAE },
};

/*
 * use this table for XLRxxx
 * caution:
 * - the XGMII/SPI4 stations si_regbase are 'special'
 * - the RGMII station si_regbase is 'special'
 */ 
static const fmn_station_info_t station_info_xlr_xxx[RMIXLR_FMN_NSTID] = {
	[RMIXLR_FMN_STID_CORE0]   = { "core0",	8,   0,   7,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE1]   = { "core1",	8,   8,  15,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE2]   = { "core2",	8,  16,  23,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE3]   = { "core3",	8,  24,  31,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE4]   = { "core4",	8,  32,  39,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE5]   = { "core5",	8,  40,  47,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE6]   = { "core6",	8,  48,  55,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE7]   = { "core7",	8,  56,  63,  32, 4, 0 },
	[RMIXLR_FMN_STID_TXRX_0]  = { "txrx0",	1,  64,  79,  16, 0, RMIXL_IO_DEV_XGMAC_A },
	[RMIXLR_FMN_STID_TXRX_1]  = { "txrx1",	1,  80,  95,  16, 0, RMIXL_IO_DEV_XGMAC_B },
	[RMIXLR_FMN_STID_RGMII]   = { "rgmii",	8,  96, 103,  32, 0, RMIXL_IO_DEV_GMAC_A },
	[RMIXLR_FMN_STID_DMA]     = { "dma",	4, 104, 107,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLR_FMN_STID_FREE_0]  = { "free0",	2, 112, 113, 128, 0, RMIXL_IO_DEV_XGMAC_A },
	[RMIXLR_FMN_STID_FREE_1]  = { "free1",	2, 114, 115, 128, 0, RMIXL_IO_DEV_XGMAC_B },
	[RMIXLR_FMN_STID_SAE]     = { "sae",	5, 120, 124,  32, 0, RMIXL_IO_DEV_SAE },
};


typedef struct fmn_intrhand {
	int		(*ih_func)(void *, rmixl_fmn_rxmsg_t *);
	void		*ih_arg;
	struct evcnt	ih_count;
} fmn_intrhand_t;

/*
 * per-core FMN structure
 */
typedef struct fmn {
	kmutex_t		       *fmn_lock; 
	u_int				fmn_core;
	u_int				fmn_thread;
	u_int				fmn_nstid;
	const int		       *fmn_stidtab;
	const fmn_station_info_t       *fmn_stinfo;
	void			       *fmn_ih;
	fmn_intrhand_t			fmn_intrhand[RMIXL_FMN_NSTID];
} fmn_t;

static fmn_t fmn_store[1 << 10];	/* index by cpuid) *//* XXX assumes 1 node */
#define NFMN	(sizeof(fmn_store) / sizeof(fmn_store[0]))

static fmn_t *
fmn_lookup(cpuid_t cpuid)
{
	KASSERT(cpuid < (cpuid_t)NFMN);
	return &fmn_store[cpuid];
}

static void rmixl_fmn_init_core_xlr(fmn_t *);
static void rmixl_fmn_init_core_xls(fmn_t *);
static void	rmixl_fmn_config_noncore(fmn_t *);
static void	rmixl_fmn_config_core(fmn_t *);
#ifdef NOTYET
static int	rmixl_fmn_intr_dispatch(void *);
#endif	/* NOTYET */
static int	rmixl_fmn_msg_recv_subr(u_int, rmixl_fmn_rxmsg_t *);

#ifdef FMN_DEBUG
void	rmixl_fmn_cp2_dump(void);
void	rmixl_fmn_cc_dump(void);
#endif

/*
 * macros used because mtc2, mfc2, dmtc2, dmfc2 instructions
 * must use literal values for rd and sel operands
 * so let the compiler sort it out
 */

/*
 * write v to all 8 SELs for given RD
 */
#define FMN_CP2_4SEL_READ(rd, sel, vp)					\
	do {								\
		uint32_t *rp = vp;					\
		RMIXL_MFC2(rd, sel,   rp[0]);				\
		RMIXL_MFC2(rd, sel+1, rp[1]);				\
		RMIXL_MFC2(rd, sel+2, rp[2]);				\
		RMIXL_MFC2(rd, sel+3, rp[3]);				\
	} while (0)

/*
 * write v to all 8 SELs for given RD
 */
#define FMN_CP2_4SEL_WRITE(rd, sel, v)					\
	do {								\
		RMIXL_MTC2(rd, sel,   v);				\
		RMIXL_MTC2(rd, sel+1, v);				\
		RMIXL_MTC2(rd, sel+2, v);				\
		RMIXL_MTC2(rd, sel+3, v);				\
	} while (0)

#define FMN_CP2_8SEL_WRITE(rd, v)					\
	do {								\
		RMIXL_MTC2(rd, 0, v);					\
		RMIXL_MTC2(rd, 1, v);					\
		RMIXL_MTC2(rd, 2, v);					\
		RMIXL_MTC2(rd, 3, v);					\
		RMIXL_MTC2(rd, 4, v);					\
		RMIXL_MTC2(rd, 5, v);					\
		RMIXL_MTC2(rd, 6, v);					\
		RMIXL_MTC2(rd, 7, v);					\
	} while (0)


#define FMN_CP2_SEL_CASE_READ(rd, sel, v)				\
	case sel:							\
		RMIXL_MFC2(rd, sel, v);					\
		break
#define FMN_CP2_SEL_CASE_WRITE(rd, sel, v)				\
	case sel:							\
		RMIXL_MTC2(rd, sel, v);					\
		break
/*
 * read/write a single arbitrary sel for the given rd
 */
#define FMN_CP2_SEL_SWITCH_RW(rw, rd, sel, val)				\
	do {								\
		switch (sel) {						\
			FMN_CP2_SEL_CASE_ ## rw(rd, 0, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 1, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 2, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 3, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 4, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 5, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 6, val);		\
			FMN_CP2_SEL_CASE_ ## rw(rd, 7, val);		\
			default:					\
				panic("%s:%d: bad sel %d\n",		\
					__func__, __LINE__, sel);	\
		}							\
	} while (0)

#define FMN_CP2_RD_CASE_RW(rw, rd, sel, val)				\
	case rd:							\
		FMN_CP2_SEL_SWITCH_RW(rw, rd, sel, val);		\
		break
/*
 * read/write a single arbitrary Credit Counter at (rd, sel)
 * eg:
 *	FMN_CP2_RD_SWITCH_RW(READ,  16, 2, val)
 *	FMN_CP2_RD_SWITCH_RW(WRITE, 18, 0, val)
 */
#define FMN_CP2_RD_SWITCH_RW(rw, rd, sel, val)				\
	do {								\
		switch(rd) {						\
			FMN_CP2_RD_CASE_RW(rw, 0,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 1,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 2,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 3,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 4,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 5,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 6,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 7,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 8,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 9,  sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 10, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 11, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 12, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 13, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 14, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 15, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 16, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 17, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 18, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 19, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 20, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 21, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 22, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 23, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 24, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 25, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 26, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 27, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 28, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 29, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 30, sel, val);		\
			FMN_CP2_RD_CASE_RW(rw, 31, sel, val);		\
			default:					\
				panic("%s:%d: bad regno %d\n",		\
					__func__, __LINE__, rd);	\
		}							\
	} while (0)


void
rmixl_fmn_init(void)
{
	int cpu;
	fmn_t *fmnp;
	static bool once=false;

	KASSERTMSG((CPU_IS_PRIMARY(curcpu())), "ci=%p, index=%d\n",
		curcpu(), cpu_index(curcpu()));
	fmnp = fmn_lookup(curcpu()->ci_cpuid);

	if (once == true)
		panic("%s: call only once!", __func__);
	once = true;

	for (cpu=0; cpu < NFMN; cpu++) {
		fmnp[cpu].fmn_core = RMIXL_CPU_CORE(cpu);
		fmnp[cpu].fmn_thread = RMIXL_CPU_THREAD(cpu);
	}

	rmixl_fmn_init_core();		/* for initial boot cpu (#0) */
	rmixl_fmn_config_noncore(fmnp);	/* boot cpu initializes noncore */
}

/*
 * link to TX station ID table for RMI XLR type chip
 */
static void
rmixl_fmn_init_core_xlr(fmn_t *fmnp)
{
	fmnp->fmn_nstid = RMIXLR_FMN_NSTID;
	fmnp->fmn_stidtab = station_xlr_xxx;
	fmnp->fmn_stinfo = station_info_xlr_xxx;
}

/*
 * link to TX station ID table for RMI XLS type chip
 */
static void
rmixl_fmn_init_core_xls(fmn_t *fmnp)
{
	const fmn_station_info_t *info = NULL;
	const int *tab = NULL;

	switch (MIPS_PRID_IMPL(mips_options.mips_cpu_id)) {
	case MIPS_XLS104:
	case MIPS_XLS108:
		tab = station_xls_1xx;
		info = station_info_xls_1xx;
		break;
	case MIPS_XLS204:
	case MIPS_XLS208:
		tab = station_xls_2xx;
		info = station_info_xls_2xx;
		break;
	case MIPS_XLS404:
	case MIPS_XLS408:
	case MIPS_XLS416:
	case MIPS_XLS608:
	case MIPS_XLS616:
		tab = station_xls_4xx;
		info = station_info_xls_4xx;
		break;
	case MIPS_XLS404LITE:
	case MIPS_XLS408LITE:
		tab = station_xls_4xx_lite;
		info = station_info_xls_4xx_lite;
		break;
	default:
		panic("%s: unknown PRID IMPL %#x\n", __func__,
			MIPS_PRID_IMPL(mips_options.mips_cpu_id));
	}

	fmnp->fmn_nstid = RMIXLS_FMN_NSTID;
	fmnp->fmn_stidtab = tab;
	fmnp->fmn_stinfo = info;
}

void
rmixl_fmn_init_core(void)
{
	fmn_t *fmnp;
	kmutex_t *lk;

	fmnp = fmn_lookup(curcpu()->ci_cpuid);
	KASSERT(fmnp != NULL);
	KASSERT(fmnp->fmn_core == RMIXL_CPU_CORE(curcpu()->ci_cpuid));
	KASSERT(fmnp->fmn_thread == RMIXL_CPU_THREAD(curcpu()->ci_cpuid));

	lk = mutex_obj_alloc(MUTEX_DEFAULT, RMIXL_FMN_INTR_IPL);
	if (lk == NULL)
		panic("%s: mutex_obj_alloc failed", __func__);
	fmnp->fmn_lock = lk;

	mutex_enter(fmnp->fmn_lock);

	/*
	 * do chip-dependent per-core FMN initialization
	 */
	switch(cpu_rmixl_chip_type(mips_options.mips_cpu)) {
	case CIDFL_RMI_TYPE_XLR:
		rmixl_fmn_init_core_xlr(fmnp);
		break;
	case CIDFL_RMI_TYPE_XLS:
		rmixl_fmn_init_core_xls(fmnp);
		break;
	case CIDFL_RMI_TYPE_XLP:
		panic("%s: RMI XLP not yet supported", __func__);
	default:
		panic("%s: RMI chip type %#x unknown", __func__,
			cpu_rmixl_chip_type(mips_options.mips_cpu));
	}

	/*
	 * thread #0 for each core owns 'global' CP2 regs
	 */
	if (fmnp->fmn_thread == 0)
		rmixl_fmn_config_core(fmnp);

	mutex_exit(fmnp->fmn_lock);
}

/*
 * rmixl_fmn_config_noncore
 *
 *	initialize bucket sizes and (minimum) credits for non-core stations to ZERO
 *	configured through memory write operations instead of CP2
 */
static void
rmixl_fmn_config_noncore(fmn_t *fmnp)
{
	for (u_int sid=0; sid < fmnp->fmn_nstid; sid++) {
		u_int regoff = fmnp->fmn_stinfo[sid].si_regbase;
		if (regoff != 0) {
			u_int buckets_max = fmnp->fmn_stinfo[sid].si_buckets_max;
			regoff += RMIXL_FMN_BS_FIRST;
			for (u_int bucket=0; bucket < buckets_max; bucket++) {
				RMIXL_IOREG_WRITE(regoff, 0);
				regoff += sizeof(uint32_t);
			}
		}
	}
}

/*
 * rmixl_fmn_config_core
 *
 *	- assumes fmn_mutex is owned
 *	- configure FMN 
 *	- initialize bucket sizes and (minimum) credits for a core
 */
static void
rmixl_fmn_config_core(fmn_t *fmnp)
{
	const fmn_station_info_t *info = fmnp->fmn_stinfo;
	uint32_t sts1;
	uint32_t cfg;
	uint32_t cp0_status;

	KASSERT(mutex_owned(fmnp->fmn_lock) != 0);
	KASSERT(fmnp->fmn_thread == 0);
	cp0_status = rmixl_cp2_enable();

	/* check/clear any pre-existing status1 error(s) */
	RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 1, sts1);
	if ((sts1 & RMIXL_MSG_STS1_ERRS) != 0)
		RMIXL_MTC2(RMIXL_COP_2_MSG_STS, 1, sts1);

	/* set up MsgConfig reg */
	cfg = ((1 << RMIXL_MSG_CFG0_WMSHIFT)			/* watermark */
	    |  (RMIXL_INTRVEC_FMN << RMIXL_MSG_CFG0_IV_SHIFT)	/* irq */
	    |  (1 << RMIXL_MSG_CFG0_ITM_SHIFT)			/* thread mask */
	    |   RMIXL_MSG_CFG0_WIE				/* watermark intr enb */
	    |   RMIXL_MSG_CFG0_EIE);				/* rx not empty intr enb */
	RMIXL_DMTC2(RMIXL_COP_2_MSG_CFG, 0, cfg);

	/* disable trace mode, credit overrun intr, messaging errors intr */
	RMIXL_DMTC2(RMIXL_COP_2_MSG_CFG, 0, 0);

	/* XXX using 4 buckets per core */
	KASSERT(4 <= info->si_buckets_max);

	/*
	 * initialize default sizes for core buckets
	 * zero sizes for unused buckets
	 */
	KASSERT(info->si_buckets_max == 8);
	uint32_t sz = info->si_bucket_size_dflt;
	KASSERT((sz & ~RMIXL_MSG_BSZ_SIZE) == 0);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 0, sz);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 1, sz);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 2, sz);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 3, sz);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 4, 0);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 5, 0);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 6, 0);
	RMIXL_MTC2(RMIXL_COP_2_MSG_BSZ, 7, 0);

	/*
	 * configure minimum credits for each core, 4 buckets
	 * zero all unused credit counters for this core
	 */
	uint32_t cr = info->si_credits_min;

	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS,   0, cr);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS,   4,  0);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+1, 0, cr);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+1, 4,  0);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+2, 0, cr);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+2, 4,  0);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+3, 0, cr);
	FMN_CP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+3, 4,  0);

	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+4,     0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+5,     0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+6,     0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+7,     0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+8,     0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+9,     0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+10,    0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+11,    0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+12,    0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+13,    0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+14,    0);
	FMN_CP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+15,    0); 

	RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 1, sts1);
	KASSERT((sts1 & RMIXL_MSG_STS1_ERRS) == 0);

	rmixl_cp2_restore(cp0_status);
}

void
rmixl_fmn_init_cpu_intr(void)
{
	fmn_t *fmnp;

	fmnp = fmn_lookup(curcpu()->ci_cpuid);
	mutex_enter(fmnp->fmn_lock);

	for (int i=0; i < fmnp->fmn_nstid; i++)
		evcnt_attach_dynamic(&fmnp->fmn_intrhand[i].ih_count,
			EVCNT_TYPE_INTR, NULL, "rmixl_fmn", fmnp->fmn_stinfo[i].si_name);

#ifdef NOTYET
	/*
	 * establish dispatcher for FMN interrupt
	 */
	extern kmutex_t rmixl_intr_lock;
	void *ih;

	mutex_enter(&rmixl_intr_lock);
	ih = rmixl_vec_establish(RMIXL_INTRVEC_FMN, -1, RMIXL_FMN_INTR_IPL,
		rmixl_fmn_intr_dispatch, fmnp, "fmn");
	if (ih == NULL)
		panic("%s: rmixl_vec_establish failed", __func__);
	mutex_exit(&rmixl_intr_lock);
	fmnp->fmn_ih = ih;
#endif

	mutex_exit(fmnp->fmn_lock);
}

void *
rmixl_fmn_intr_establish(int txstid, int (*func)(void *, rmixl_fmn_rxmsg_t *), void *arg)
{
	fmn_t *fmnp;
	fmn_intrhand_t *ih;

	fmnp = fmn_lookup(curcpu()->ci_cpuid);

	mutex_enter(fmnp->fmn_lock);

	ih = &fmnp->fmn_intrhand[txstid];

	if (ih->ih_func != NULL) {
#ifdef DEBUG
		panic("%s: intrhand[%d] busy", __func__, txstid);
#endif
		ih = NULL;
	}  else {
		ih->ih_func = func;
		ih->ih_arg = arg;
	}

	mutex_exit(fmnp->fmn_lock);

	return ih;
}

void
rmixl_fmn_intr_disestablish(void *cookie)
{
	fmn_t *fmnp;
	fmn_intrhand_t *ih = cookie;

	fmnp = fmn_lookup(curcpu()->ci_cpuid);
	mutex_enter(fmnp->fmn_lock);

	if (ih->ih_func != NULL) {
		ih->ih_func = NULL;
		ih->ih_arg = NULL;
	}
#ifdef DEBUG
	else {
		panic("%s: intrhand[%ld] not in use",
			__func__, ih - &fmnp->fmn_intrhand[0]);
	}
#endif

	mutex_exit(fmnp->fmn_lock);
}

void
rmixl_fmn_intr_poll(u_int bucket, rmixl_fmn_rxmsg_t *rxmsg)
{
	uint32_t bit = 1 << bucket;
	uint32_t cp0_status;

	KASSERT(bucket < 8);

	cp0_status = rmixl_cp2_enable();

	for(;;) {
		rmixl_fmn_msgwait(bit);
		if (rmixl_fmn_msg_recv(bucket, rxmsg) == 0)
			break;
		DELAY(10);		/* XXX */
	}

	rmixl_cp2_restore(cp0_status);
}

#ifdef NOTYET
static int
rmixl_fmn_intr_dispatch(void *arg)
{
	fmn_t *fmnp = arg;
	uint32_t msg_status;
	uint32_t cp0_status;
	uint32_t rfbne;
	int txstid;
	int rv = 0;

	mutex_enter(fmnp->fmn_lock);
	cp0_status = rmixl_cp2_enable();

	RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 0, msg_status);
	rfbne = (~msg_status) >> RMIXL_MSG_STS0_RFBE_SHIFT;

	if (rfbne != 0) {
		DPRINTF(("%s: rfbne %#x\n", __func__, rfbne));
		for (u_int bucket=0; bucket < 8; bucket++) {
			rmixl_fmn_rxmsg_t rxmsg;
			fmn_intrhand_t *ih;

			if ((rfbne & (1 << bucket)) == 0)
				continue;
			if (rmixl_fmn_msg_recv_subr(bucket, &rxmsg) != 0)
				continue;
			rv = 1;
			txstid = fmnp->fmn_stidtab[rxmsg.rxsid];
			ih = &fmnp->fmn_intrhand[txstid];
			if (ih->ih_func != NULL)
				if ((ih->ih_func)(ih->ih_arg, &rxmsg) != 0)
					ih->ih_count.ev_count++;
		}

	}
	rmixl_cp2_restore(cp0_status);
	mutex_exit(fmnp->fmn_lock);

	return rv;
}
#endif	/* NOTYET */

int
rmixl_fmn_msg_send(u_int size, u_int code, u_int dest_id, rmixl_fmn_msg_t *msg)
{
	fmn_t *fmnp;
        uint32_t cp0_status;
        uint32_t msg_status;
        uint32_t msg_status1;
	uint32_t desc;
	int rv = 0;
    
	KASSERT((size >= 1) && size <= 4);
	KASSERT(code <= 0xff);
	KASSERT(dest_id <= 0xff);

	fmnp = fmn_lookup(curcpu()->ci_cpuid);
	mutex_enter(fmnp->fmn_lock);
        cp0_status = rmixl_cp2_enable();

	switch(size) {
	case 1:
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		break;
	case 2:
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 1, msg->data[1]);
		break;
	case 3:
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 1, msg->data[1]);
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 2, msg->data[2]);
		break;
	case 4:
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 1, msg->data[1]);
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 2, msg->data[2]);
		RMIXL_DMTC2(RMIXL_COP_2_TXBUF, 3, msg->data[3]);
		break;
	default:
		DIAG_PRF(("%s: bad size %d", __func__, size));
		rv = -1;
		goto out;
	}

	for (int try=16; try--; ) {
		RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 0, msg_status);
		if ((msg_status & (RMIXL_MSG_STS0_LPF|RMIXL_MSG_STS0_SPF|RMIXL_MSG_STS0_SMP)) == 0)
			goto send;
		DELAY(10);		/* XXX ??? */
	}
	DIAG_PRF(("%s: cpu%u, msg %p, dst_id=%d, sts=%#x: can't send\n",
		__func__, cpu_number(), msg, dest_id, msg_status));
	rv = -1;
	goto out;
 send:
	desc = RMIXL_MSGSND_DESC(size, code, dest_id);
	DPRINTF(("%s: cpu%u, desc %#x\n", __func__, cpu_number(), desc));
	for (int try=16; try--; ) {
		rmixl_msgsnd(desc);
		RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 0, msg_status);
		RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 1, msg_status1);
		if (((msg_status  & RMIXL_MSG_STS0_SCF) == 0)
		&&  ((msg_status1 & RMIXL_MSG_STS1_ERRS) == 0))
			goto out;
#if 0
#ifdef DEBUG
		if ((msg_status & RMIXL_MSG_STS0_SCF) != 0) {
			uint32_t r;
			u_int regno = RMIXL_COP_2_CREDITS+fmnp->fmn_core;
			u_int sel = fmnp->fmn_thread;
			printf("%s: CC[%d,%d]=", __func__, regno, sel);
			FMN_CP2_RD_SWITCH_RW(READ, regno, sel, r);
			printf("%s: CC[%d,%d]=%d\n", __func__, regno, sel, r);
		}
#endif	/* DEBUG */
#endif	/* 0 */
		/* clear status1 error(s) */
		if ((msg_status1 & RMIXL_MSG_STS1_ERRS) != 0) {
			RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 1, msg_status1);
			RMIXL_MTC2(RMIXL_COP_2_MSG_STS, 1, msg_status1);
		}
		DIAG_PRF(("%s: src=%ld, dst=%d, sts=%#x, %#x: send error, try %d\n",
			__func__, curcpu()->ci_cpuid, dest_id, msg_status, msg_status1, try));
		DELAY(10);
	}
	rv = -1;
 out:
	rmixl_cp2_restore(cp0_status);
	mutex_exit(fmnp->fmn_lock);

	return rv;
}

/*
 * rmixl_fmn_msg_recv
 *
 *	- grab fmn_lock and call rmixl_fmn_msg_recv_subr to do the real work
 *	- assume cp2 access is already enabled
 */
int
rmixl_fmn_msg_recv(u_int bucket, rmixl_fmn_rxmsg_t *rxmsg)
{
	fmn_t *fmnp;
	int rv;

	fmnp = fmn_lookup(curcpu()->ci_cpuid);
	mutex_enter(fmnp->fmn_lock);
	rv = rmixl_fmn_msg_recv_subr(bucket, rxmsg);
	mutex_exit(fmnp->fmn_lock);

	return rv;
}

/*
 * rmixl_fmn_msg_recv_subr
 *
 *	- assume fmn_lock is owned
 *	- assume cp2 access is already enabled 
 */
static int
rmixl_fmn_msg_recv_subr(u_int bucket, rmixl_fmn_rxmsg_t *rxmsg)
{
	fmn_t *fmnp;
	uint32_t msg_status;
	int rv;

	fmnp = fmn_lookup(curcpu()->ci_cpuid);
	KASSERT(mutex_owned(fmnp->fmn_lock) != 0);

	for (int try=16; try--; ) {
		RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 0, msg_status);
		if ((msg_status & (RMIXL_MSG_STS0_LPF)) == 0)
			goto recv;
	}
	DIAG_PRF(("%s: cpu%u, bucket=%d, sts=%#x: Load Pending Fail\n",
		__func__, cpu_number(), bucket, msg_status));
	rv = -1;
	goto out;
 recv:
	rmixl_msgld(bucket);
	RMIXL_MFC2(RMIXL_COP_2_MSG_STS, 0, msg_status);
	DPRINTF(("%s: cpu%u, bucket=%d, sts=%#x\n",
		__func__, cpu_number(), bucket, msg_status));
	rv = msg_status & (RMIXL_MSG_STS0_LEF|RMIXL_MSG_STS0_LPF);
	if (rv == 0) {
		rxmsg->rxsid = (msg_status & RMIXL_MSG_STS0_RMSID)
				>> RMIXL_MSG_STS0_RMSID_SHIFT;
		rxmsg->code = (msg_status & RMIXL_MSG_STS0_RMSC)
				>> RMIXL_MSG_STS0_RMSC_SHIFT;
		rxmsg->size = ((msg_status & RMIXL_MSG_STS0_RMS)
				>> RMIXL_MSG_STS0_RMS_SHIFT) + 1;
		switch(rxmsg->size) {
		case 1:
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 0, rxmsg->msg.data[0]);
			break;
		case 2:
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 0, rxmsg->msg.data[0]);
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 1, rxmsg->msg.data[1]);
			break;
		case 3:
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 0, rxmsg->msg.data[0]);
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 1, rxmsg->msg.data[1]);
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 2, rxmsg->msg.data[2]);
			break;
		case 4:
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 0, rxmsg->msg.data[0]);
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 1, rxmsg->msg.data[1]);
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 2, rxmsg->msg.data[2]);
			RMIXL_DMFC2(RMIXL_COP_2_RXBUF, 3, rxmsg->msg.data[3]);
			break;
		default:
			/* "impossible" due to bitfield width */
			panic("%s: bad size %d", __func__, rxmsg->size);
		}
	}
 out:

	return rv;
}

#ifdef FMN_DEBUG
void
rmixl_fmn_cp2_dump(void)
{
	uint32_t cp0_status;

	cp0_status = rmixl_cp2_enable();

	CPU2_PRINT_8(RMIXL_COP_2_TXBUF, 0);
	CPU2_PRINT_8(RMIXL_COP_2_TXBUF, 1);
	CPU2_PRINT_8(RMIXL_COP_2_TXBUF, 2);
	CPU2_PRINT_8(RMIXL_COP_2_TXBUF, 3);

	CPU2_PRINT_8(RMIXL_COP_2_RXBUF, 0);
	CPU2_PRINT_8(RMIXL_COP_2_RXBUF, 1);
	CPU2_PRINT_8(RMIXL_COP_2_RXBUF, 2);
	CPU2_PRINT_8(RMIXL_COP_2_RXBUF, 3);

	CPU2_PRINT_4(RMIXL_COP_2_MSG_STS, 0);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_STS, 1);

	CPU2_PRINT_4(RMIXL_COP_2_MSG_CFG, 0);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_CFG, 1);

	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 0);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 1);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 2);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 3);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 4);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 5);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 6);
	CPU2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 7);

	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 0);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 1);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 2);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 3);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 4);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 5);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 6);
	CPU2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 7);

	rmixl_cp2_restore(cp0_status);
}


void
rmixl_fmn_cc_dump(void)
{
	uint32_t cc[4][8];

	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS,   0, &cc[0][0]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS,   4, &cc[0][4]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS+1, 0, &cc[1][0]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS+1, 4, &cc[1][4]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS+2, 0, &cc[2][0]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS+2, 4, &cc[2][4]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS+3, 0, &cc[3][0]);
	FMN_CP2_4SEL_READ(RMIXL_COP_2_CREDITS+3, 4, &cc[3][4]);

	printf("%s: cpu%u\n", __func__, cpu_number());
	for (int i=0; i < 4; i++) {
		for (int j=0; j < 8; j++)
			printf(" %#x,", cc[i][j]);
		printf("\n");
	}
}

#endif	/* FMN_DEBUG */
