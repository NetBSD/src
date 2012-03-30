/*	rmixl_fmn.c,v 1.1.2.10 2012/01/19 08:05:24 matt Exp	*/
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
#include "opt_cputype.h"

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/atomic.h>
#include <sys/intr.h>

#include <dev/pci/pcidevs.h>

#include <mips/cpuregs.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_cpuvar.h>
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
 * macros used because mtc2, mfc2, dmtc2, dmfc2 instructions
 * must use literal values for rd and sel operands
 * so let the compiler sort it out
 */

/*
 * read first 4 SELs for given RD
 */
#define FMN_COP2_4SEL_READ(rd, sel, vp)					\
	do {								\
		uint32_t *rp = (vp);					\
		rp[0] = mips_mfc2(rd, sel+0);				\
		rp[1] = mips_mfc2(rd, sel+1);				\
		rp[2] = mips_mfc2(rd, sel+2);				\
		rp[3] = mips_mfc2(rd, sel+3);				\
	} while (0)

/*
 * write v to first 4 SELs for given RD
 */
#define FMN_COP2_4SEL_WRITE(rd, sel, v)					\
	do {								\
		mips_mtc2(rd, sel,   v);				\
		mips_mtc2(rd, sel+1, v);				\
		mips_mtc2(rd, sel+2, v);				\
		mips_mtc2(rd, sel+3, v);				\
	} while (0)

#define FMN_COP2_8SEL_WRITE(rd, v)					\
	do {								\
		mips_mtc2(rd, 0, v);					\
		mips_mtc2(rd, 1, v);					\
		mips_mtc2(rd, 2, v);					\
		mips_mtc2(rd, 3, v);					\
		mips_mtc2(rd, 4, v);					\
		mips_mtc2(rd, 5, v);					\
		mips_mtc2(rd, 6, v);					\
		mips_mtc2(rd, 7, v);					\
	} while (0)


#define FMN_COP2_SEL_CASE_READ(rd, sel, v)				\
	case sel:							\
		v = mips_mfc2(rd, sel);					\
		break
#define FMN_COP2_SEL_CASE_WRITE(rd, sel, v)				\
	case sel:							\
		mips_mtc2(rd, sel, v);					\
		break
/*
 * read/write a single arbitrary sel for the given rd
 */
#define FMN_COP2_SEL_SWITCH_RW(rw, rd, sel, val)			\
	do {								\
		switch (sel) {						\
			FMN_COP2_SEL_CASE_ ## rw(rd, 0, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 1, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 2, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 3, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 4, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 5, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 6, val);		\
			FMN_COP2_SEL_CASE_ ## rw(rd, 7, val);		\
			default:					\
				panic("%s:%d: bad sel %d\n",		\
					__func__, __LINE__, sel);	\
		}							\
	} while (0)

#define FMN_COP2_RD_CASE_RW(rw, rd, sel, val)				\
	case rd:							\
		FMN_COP2_SEL_SWITCH_RW(rw, rd, sel, val);		\
		break
/*
 * read/write a single arbitrary Credit Counter at (rd, sel)
 * eg:
 *	FMN_COP2_RD_SWITCH_RW(READ,  16, 2, val)
 *	FMN_COP2_RD_SWITCH_RW(WRITE, 18, 0, val)
 */
#define FMN_COP2_RD_SWITCH_RW(rw, rd, sel, val)				\
	do {								\
		switch(rd) {						\
			FMN_COP2_RD_CASE_RW(rw, 0,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 1,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 2,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 3,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 4,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 5,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 6,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 7,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 8,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 9,  sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 10, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 11, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 12, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 13, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 14, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 15, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 16, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 17, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 18, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 19, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 20, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 21, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 22, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 23, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 24, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 25, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 26, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 27, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 28, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 29, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 30, sel, val);		\
			FMN_COP2_RD_CASE_RW(rw, 31, sel, val);		\
			default:					\
				panic("%s:%d: bad regno %d\n",		\
					__func__, __LINE__, rd);	\
		}							\
	} while (0)


static int
fmn_stray_intr(void *v, rmixl_fmn_rxmsg_t *msg)
{
	/* nothing */
	return 0;
}

typedef uint8_t fmn_queue_id_t;

typedef struct fmn_station_info {
	const char *			si_name;
	uint16_t			si_qid_first;
	uint16_t			si_qid_last;
	u_int				si_buckets_max;
	u_int				si_bucket_size_dflt;
	u_int				si_credits_min;
	u_int				si_regbase;
} fmn_station_info_t;

typedef struct fmn_intrhand {
	rmixl_fmn_intr_handler_t	ih_func;
	void *				ih_arg;
} fmn_intrhand_t;

/*
 * Global FMN stuff
 */
typedef struct fmn_info {
	const fmn_queue_id_t *		fmn_qidtab;
	const fmn_station_info_t *	fmn_stinfo;
	const char			(*fmn_bucket_names)[12];
	size_t				fmn_intr_vec;
	uint8_t				fmn_nstid;
	uint8_t				fmn_nbucket;	// or vc (XLP)
	volatile uint32_t		fmn_coremask;
	volatile uint32_t		fmn_nthread;

	/*
	 * There are no per-CPU handlers.
	 */
	volatile fmn_intrhand_t		fmn_intrhand[RMIXL_FMN_NSTID];
} fmn_info_t;

static fmn_info_t fmn_info = {
	.fmn_intrhand = {
		[0 ... RMIXL_FMN_NSTID-1] = {
			.ih_func = fmn_stray_intr,
			.ih_arg = &fmn_info,
		},
	},
};

static char fmn_stid_ev_names[RMIXL_FMN_NSTID][32];

#if (MIPS64_XLR + MIPS64_XLS) > 0
static const char xlrxls_bucket_names[8][12] = {
	"bucket 0", "bucket 1", "bucket 2", "bucket 3",
	"bucket 4", "bucket 5", "bucket 6", "bucket 7",
};
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */

#ifdef MIPS64_XLP
static const struct xlp_stid_map {
	pcitag_t map_pcitag;
	uint16_t map_product_id;
	uint8_t map_stid;
} xlp_stid_map[] = {
#define	MAP_ENTRY3(a,b,c) { \
	.map_pcitag = RMIXLP_##a##_PCITAG, \
	.map_product_id = PCI_PRODUCT_NETLOGIC_XLP_##b, \
	.map_stid = RMIXLP_FMN_STID_##c }
#define	MAP_ENTRY1(a)	MAP_ENTRY3(a, a, a)
	MAP_ENTRY3(PCIPORT0, PCIROOT, PCIE0),
	MAP_ENTRY3(PCIPORT1, PCIROOT, PCIE1),
	MAP_ENTRY3(PCIPORT2, PCIROOT, PCIE2),
	MAP_ENTRY3(PCIPORT3, PCIROOT, PCIE3),
	MAP_ENTRY1(NAE),
	MAP_ENTRY1(NAE),
	MAP_ENTRY1(NAE),
	MAP_ENTRY1(NAE),
	MAP_ENTRY1(POE),
	MAP_ENTRY1(FMN),
	MAP_ENTRY1(DMA),
	MAP_ENTRY1(SAE),
	MAP_ENTRY1(PKE),
	MAP_ENTRY1(CDE),
	MAP_ENTRY1(RXE),
	MAP_ENTRY1(SRIO),
#undef MAP_ENTRY1
#undef MAP_ENTRY3
};

/* use this table for XLPxxx */
static fmn_queue_id_t xlp_xxx_qidtab[1024] = {
	/* There are 4 VC queues per thread and are added dynamicly */
	[ 128 ...  255]	= RMIXLP_FMN_STID_POPQ,
	/* I/O Push Queues start here and are determinted dynamicly */
};

/* use this table for XLPxxx */
static fmn_station_info_t xlp_xxx_stinfo[RMIXLP_FMN_NSTID] = {
	[RMIXLP_FMN_STID_CPU]    = { "cpu" },
	[RMIXLP_FMN_STID_POPQ]    = { "popq",	128,  255 },
	[RMIXLP_FMN_STID_PCIE0]   = { "pcie0" },
	[RMIXLP_FMN_STID_PCIE1]   = { "pcie1" },
	[RMIXLP_FMN_STID_PCIE2]   = { "pcie2" },
	[RMIXLP_FMN_STID_PCIE3]   = { "pcie3" },
	[RMIXLP_FMN_STID_DMA]     = { "dma" },
	[RMIXLP_FMN_STID_PKE]     = { "pke" },
	[RMIXLP_FMN_STID_SAE]     = { "sae" },
	[RMIXLP_FMN_STID_CDE]     = { "cde" },
	[RMIXLP_FMN_STID_POE]     = { "poe" },
	[RMIXLP_FMN_STID_NAE]     = { "nae" },
	[RMIXLP_FMN_STID_NAE_FREEIN] = { "freein" },
	[RMIXLP_FMN_STID_FMN]     = { "fmn" },
	[RMIXLP_FMN_STID_RXE]     = { "rxe" },
	[RMIXLP_FMN_STID_SRIO]    = { "srio" },
};

static const char xlp_bucket_names[4][12] = {
	"vc 0", "vc 1", "vc 2", "vc 3",
};

static void
fmn_init_xlp_claim_qids(const fmn_station_info_t * const si, size_t stid)
{
	for (size_t qid = si->si_qid_first; qid <= si->si_qid_last; qid++) {
		xlp_xxx_qidtab[qid++] = stid;
	}
}
/*
 * link to TX station ID table for RMI XLP type chip
 */
static void
fmn_init_xlp(fmn_info_t *fmn)
{
	fmn->fmn_nbucket = 4;	// 4 VCs per thread
	fmn->fmn_bucket_names = xlp_bucket_names;
	fmn->fmn_nstid = RMIXLP_FMN_NSTID;
	fmn->fmn_qidtab = xlp_xxx_qidtab;
	fmn->fmn_stinfo = xlp_xxx_stinfo;

	for (size_t i = 0; i < __arraycount(xlp_stid_map); i++) {
		const struct xlp_stid_map * const map = &xlp_stid_map[i];
		fmn_station_info_t * const si = &xlp_xxx_stinfo[map->map_stid];
		pcireg_t id = rmixlp_read_4(map->map_pcitag, PCI_ID_REG);
		if (PCI_PRODUCT(id) != map->map_product_id) {
			continue;
		}

		pcireg_t statinfo = rmixlp_read_4(map->map_pcitag,
		    PCI_RMIXLP_STATID);
		size_t qid_count = PCI_RMIXLP_STATID_COUNT(statinfo);
		size_t qid_base = PCI_RMIXLP_STATID_BASE(statinfo);
		if (qid_base < 256 && qid_count > 0) {
			aprint_error("%s: pci device %#lx has "
			    "invalid station information (%#x); ignored\n",
			    __func__, map->map_pcitag, statinfo);
			qid_count = 0;
		}
		if (qid_count == 0) {
			si->si_qid_first = 1024;
			si->si_qid_last = 1023;
			continue;
		}

		si->si_qid_first = qid_base;
		si->si_qid_last = qid_base + qid_count - 1;

		fmn_init_xlp_claim_qids(si, map->map_stid);
	}

	/*
	 * Initialize the NAE Free-In FIFO qids (used for supply buffers
	 * for received packets).  There is one qid per interface.
	 */
	fmn_station_info_t * const si =
		    &xlp_xxx_stinfo[RMIXLP_FMN_STID_NAE_FREEIN];
	if (RMIXLP_8XX_P) {	
		si->si_qid_first = 1000;
		si->si_qid_last = 1019;
	} else {
		KASSERT(RMIXLP_3XX_P);
		si->si_qid_first = 496;
		si->si_qid_last = 504;
	}
	fmn_init_xlp_claim_qids(si, RMIXLP_FMN_STID_NAE_FREEIN);
}

static void
fmn_init_thread_xlp(fmn_info_t *fmn)
{
}
#endif /* MIPS64_XLP */

#ifdef MIPS64_XLS
/*
 * index CPU-dependent table by (global) bucket ID to obtain logical Station ID
 * see Table 12.1 in the XLS PRM
 */
/* use this table for XLS6xx, XLS4xx */
static const fmn_queue_id_t xls_4xx_qidtab[] = {
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

/* use this table for XLS6xx, XLS4xx */
static const fmn_station_info_t xls_4xx_stinfo[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	  0,   7, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	  8,  15, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	 16,  23, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	 24,  31, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	 64,  71, 8,  32, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0", 80,  87, 3,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_GMAC_Q1] = { "gmac_q1", 96, 103, 3,  32, 0, RMIXL_IO_DEV_GMAC_4 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	104, 107, 4,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLS_FMN_STID_CDE]     = { "cde",	108, 109, 4, 128, 0, RMIXL_IO_DEV_CDE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	120, 121, 2, 128, 0, RMIXL_IO_DEV_SAE },
};

/* use this table for XLS408Lite, XLS404Lite */
static const fmn_queue_id_t xls_4xx_lite_qidtab[] = {
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

/* use this table for XLS4xxLite */
static const fmn_station_info_t xls_4xx_lite_stinfo[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	  0,   7, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	  8,  15, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	 16,  23, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	 24,  31, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0", 80,  87, 3,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_GMAC_Q1] = { "gmac_q1", 96, 103, 3,  32, 0, RMIXL_IO_DEV_GMAC_4 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	104, 107, 4,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLS_FMN_STID_CDE]     = { "cde",	108, 109, 4, 128, 0, RMIXL_IO_DEV_CDE },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	116, 119, 4,  64, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	120, 121, 2, 128, 0, RMIXL_IO_DEV_SAE },
};

/* use this table for XLS2xx */
static const fmn_queue_id_t xls_2xx_qidtab[] = {
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

/* use this table for XLS2xx */
static const fmn_station_info_t xls_2xx_stinfo[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	  0,   7, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	  8,  15, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	 16,  23, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	 24,  31, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	 64,  71, 8,  32, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0", 96, 103, 3,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	104, 107, 4,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	120, 121, 2, 128, 0, RMIXL_IO_DEV_SAE },
};

/* use this table for XLS1xx */
static const fmn_queue_id_t xls_1xx_qidtab[] = {
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

/* use this table for XLS1xx */
static const fmn_station_info_t xls_1xx_stinfo[RMIXLS_FMN_NSTID] = {
	[RMIXLS_FMN_STID_CORE0]   = { "core0",	  0,   7, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE1]   = { "core1",	  8,  15, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE2]   = { "core2",	 16,  23, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_CORE3]   = { "core3",	 24,  31, 8,  32, 4, 0 },
	[RMIXLS_FMN_STID_PCIE]    = { "pcie",	 64,  67, 4,  32, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_GMAC_Q0] = { "gmac_q0", 96, 101, 3,  32, 0, RMIXL_IO_DEV_GMAC_0 },
	[RMIXLS_FMN_STID_DMA]     = { "dma",	104, 107, 4,  64, 0, RMIXL_IO_DEV_PCIE_BE },
	[RMIXLS_FMN_STID_SAE]     = { "sae",	120, 121, 2, 128, 0, RMIXL_IO_DEV_SAE },
};

static const struct xls_stid_map {
	uint8_t				map_impl;
	const fmn_queue_id_t *		map_qidtab;
	const fmn_station_info_t *	map_stinfo;
} xls_stid_map[] = {
	{ MIPS_XLS104, xls_1xx_qidtab, xls_1xx_stinfo },
	{ MIPS_XLS108, xls_1xx_qidtab, xls_1xx_stinfo },
	{ MIPS_XLS204, xls_2xx_qidtab, xls_2xx_stinfo },
	{ MIPS_XLS208, xls_2xx_qidtab, xls_2xx_stinfo },
	{ MIPS_XLS404, xls_4xx_qidtab, xls_4xx_stinfo },
	{ MIPS_XLS408, xls_4xx_qidtab, xls_4xx_stinfo },
	{ MIPS_XLS416, xls_4xx_qidtab, xls_4xx_stinfo },
	{ MIPS_XLS608, xls_4xx_qidtab, xls_4xx_stinfo },
	{ MIPS_XLS616, xls_4xx_qidtab, xls_4xx_stinfo },
	{ MIPS_XLS404LITE, xls_4xx_lite_qidtab, xls_4xx_lite_stinfo },
	{ MIPS_XLS408LITE, xls_4xx_lite_qidtab, xls_4xx_lite_stinfo },
};


/*
 * link to TX station ID table for RMI XLS type chip
 */
static void
fmn_init_xls(fmn_info_t *fmn)
{
	const uint8_t impl = MIPS_PRID_IMPL(mips_options.mips_cpu_id);

	for (size_t i = 0; i < __arraycount(xls_stid_map); i++) {
		const struct xls_stid_map * const map = &xls_stid_map[i];
		if (map->map_impl == impl) {
			fmn->fmn_nbucket = 8;	// 4 buckets per core
			fmn->fmn_bucket_names = xlrxls_bucket_names;
			fmn->fmn_nstid = RMIXLS_FMN_NSTID;
			fmn->fmn_qidtab = map->map_qidtab;
			fmn->fmn_stinfo = map->map_stinfo;
			return;
		}
	}

	panic("%s: unknown PRID IMPL %#x\n", __func__, impl);
}
#endif /* MIPS64_XLS */

#ifdef MIPS64_XLR
/* use this table for XLRxxx */
static const fmn_queue_id_t xlr_xxx_qidtab[] = {
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

/*
 * use this table for XLRxxx
 * caution:
 * - the XGMII/SPI4 stations si_regbase are 'special'
 * - the RGMII station si_regbase is 'special'
 */
static const fmn_station_info_t xlr_xxx_stinfo[RMIXLR_FMN_NSTID] = {
	[RMIXLR_FMN_STID_CORE0]   = { "core0",	  0,   7, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE1]   = { "core1",	  8,  15, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE2]   = { "core2",	 16,  23, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE3]   = { "core3",	 24,  31, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE4]   = { "core4",	 32,  39, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE5]   = { "core5",	 40,  47, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE6]   = { "core6",	 48,  55, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_CORE7]   = { "core7",	 56,  63, 8,  32, 4, 0 },
	[RMIXLR_FMN_STID_TXRX_0]  = { "txrx0",	 64,  79, 1,  16, 0, RMIXL_IO_DEV_XGMAC_A },
	[RMIXLR_FMN_STID_TXRX_1]  = { "txrx1",	 80,  95, 1,  16, 0, RMIXL_IO_DEV_XGMAC_B },
	[RMIXLR_FMN_STID_RGMII]   = { "rgmii",	 96, 103, 8,  32, 0, RMIXL_IO_DEV_GMAC_A },
	[RMIXLR_FMN_STID_DMA]     = { "dma",	104, 107, 4,  64, 0, RMIXL_IO_DEV_DMA },
	[RMIXLR_FMN_STID_FREE_0]  = { "free0",	112, 113, 2, 128, 0, RMIXL_IO_DEV_XGMAC_A },
	[RMIXLR_FMN_STID_FREE_1]  = { "free1",	114, 115, 2, 128, 0, RMIXL_IO_DEV_XGMAC_B },
	[RMIXLR_FMN_STID_SAE]     = { "sae",	120, 124, 5,  32, 0, RMIXL_IO_DEV_SAE },
};

/*
 * link to TX station ID table for RMI XLR type chip
 */
static void
fmn_init_xlr(fmn_info_t *fmn)
{
	fmn->fmn_nbucket = 8;	// 4 buckets per core
	fmn->fmn_bucket_names = xlrxls_bucket_names;
	fmn->fmn_nstid = RMIXLR_FMN_NSTID;
	fmn->fmn_qidtab = xlr_xxx_qidtab;
	fmn->fmn_stinfo = xlr_xxx_stinfo;
}
#endif /* MIPS64_XLR */

#if (MIPS64_XLR + MIPS64_XLS) > 0
/*
 * fmn_init_noncore_xlrxls
 *
 *	initialize bucket sizes and (minimum) credits for non-core stations
 *	to ZERO.  configured through memory write operations instead of CP2
 */
static void
fmn_init_noncore_xlrxls(fmn_info_t *fmn)
{
	for (const fmn_station_info_t *si = fmn->fmn_stinfo;
	     si < &fmn->fmn_stinfo[fmn->fmn_nstid];
	     si++) {
		u_int regoff = si->si_regbase;
		if (regoff != 0) {
			regoff += RMIXL_FMN_BS_FIRST;
			for (size_t bucket=0;
			     bucket < si->si_buckets_max;
			     bucket++) {
				RMIXL_IOREG_WRITE(regoff, 0);
				regoff += sizeof(uint32_t);
			}
		}
	}
}

/*
 * fmn_init_thread_xlrxls
 *
 *	- configure FMN
 *	- initialize bucket sizes and (minimum) credits for a core
 */
static void
fmn_init_thread_xlrxls(fmn_info_t *fmn)
{
	const fmn_station_info_t *si = fmn->fmn_stinfo + 1;
	uint32_t sts1;
	uint32_t cfg;

	/*
	 * Let's see if we've initialized this core before.
	 */
	const uint32_t new = __BIT(RMIXL_CPU_CORE(curcpu()->ci_cpuid));
	for (;;) {
		const uint32_t old = fmn->fmn_coremask;
		if (old & new) {
			fmn_init_noncore_xlrxls(fmn);
			return;
		}
		if (old == atomic_cas_32(&fmn->fmn_coremask, old, old | new))
			break;
	}

	const uint32_t cp0_status = rmixl_cp2_enable();

	/* check/clear any pre-existing status1 error(s) */
	sts1 = mips_mfc2(RMIXL_COP_2_MSG_STS, 1);
	if ((sts1 & RMIXL_MSG_STS1_ERRS) != 0)
		mips_mtc2(RMIXL_COP_2_MSG_STS, 1, sts1);

	/* set up MsgConfig reg */
	cfg = __SHIFTIN(1, RMIXL_MSG_CFG0_WM)		/* watermark */
	    |  __SHIFTIN(fmn->fmn_intr_vec, RMIXL_MSG_CFG0_IV)
							/* irq */
	    |  __SHIFTIN(1, RMIXL_MSG_CFG0_ITM)		/* thread mask */
	    |   RMIXL_MSG_CFG0_WIE			/* watermark intr enb */
	    |   RMIXL_MSG_CFG0_EIE;			/* rx not empty intr enb */
	mips_dmtc2(RMIXL_COP_2_MSG_CFG, 0, cfg);

	/* disable trace mode, credit overrun intr, messaging errors intr */
	mips_dmtc2(RMIXL_COP_2_MSG_CFG, 0, 0);

	/* XXX using 4 buckets per core */
	KASSERT(4 <= si->si_buckets_max);

	/*
	 * initialize default sizes for core buckets
	 * zero sizes for unused buckets
	 */
	KASSERT(si->si_buckets_max == 8);
	uint32_t sz = si->si_bucket_size_dflt;
	KASSERT((sz & ~RMIXL_MSG_BSZ_SIZE) == 0);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 0, sz);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 1, sz);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 2, sz);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 3, sz);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 4, 0);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 5, 0);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 6, 0);
	mips_mtc2(RMIXL_COP_2_MSG_BSZ, 7, 0);

	/*
	 * configure minimum credits for each core, 4 buckets
	 * zero all unused credit counters for this core
	 */
	uint32_t cr = si->si_credits_min;

	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS,   0, cr);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS,   4,  0);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+1, 0, cr);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+1, 4,  0);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+2, 0, cr);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+2, 4,  0);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+3, 0, cr);
	FMN_COP2_4SEL_WRITE(RMIXL_COP_2_CREDITS+3, 4,  0);

	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+4,     0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+5,     0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+6,     0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+7,     0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+8,     0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+9,     0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+10,    0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+11,    0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+12,    0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+13,    0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+14,    0);
	FMN_COP2_8SEL_WRITE(RMIXL_COP_2_CREDITS+15,    0);

	sts1 = mips_mfc2(RMIXL_COP_2_MSG_STS, 1);
	KASSERT((sts1 & RMIXL_MSG_STS1_ERRS) == 0);

	rmixl_cp2_restore(cp0_status);
}
#endif /* (MIPS64_XLR + MIPS64_XLS) > 0 */

#if (MIPS64_XLP) > 0
static void	fmn_init_thread_xlp(fmn_info_t *);
#endif
static int	fmn_intr(void *);
static void	fmn_softint(void *);

#ifdef FMN_DEBUG
void	rmixl_fmn_cp2_dump(void);
void	rmixl_fmn_cc_dump(void);
#endif

/*
 * This is called from cpu_configure before autoconf starts.
 */
void
rmixl_fmn_init(void)
{
	fmn_info_t * const fmn = &fmn_info;

	/*
	 * do chip-dependent FMN initialization
	 */
	switch(cpu_rmixl_chip_type(mips_options.mips_cpu)) {
#ifdef MIPS64_XLR
	case CIDFL_RMI_TYPE_XLR:
		fmn_init_xlr(fmn);
		break;
#endif
#ifdef MIPS64_XLS
	case CIDFL_RMI_TYPE_XLS:
		fmn_init_xls(fmn);
		break;
#endif
#ifdef MIPS64_XLP
	case CIDFL_RMI_TYPE_XLP:
		fmn_init_xlp(fmn);
		break;
#endif
	default:
		panic("%s: RMI chip type %#x unknown", __func__,
			cpu_rmixl_chip_type(mips_options.mips_cpu));
	}
}

void
rmixl_fmn_cpu_attach(struct cpu_info *ci)
{
	fmn_info_t * const fmn = &fmn_info;
	struct cpu_softc * const sc = ci->ci_softc;

	KASSERT(sc->sc_fmn_si == NULL);
	sc->sc_fmn_si = softint_establish(SOFTINT_NET|SOFTINT_MPSAFE,
	    fmn_softint, sc);
	KASSERT(sc->sc_fmn_si != NULL);

	KASSERT(sc->sc_dev != NULL);

	const char * const xname = device_xname(sc->sc_dev);
	KASSERT(xname != NULL);

	for (size_t i = 1; i < fmn_info.fmn_nstid; i++) {
		if (fmn_stid_ev_names[i][0] != '\0') {
			evcnt_attach_dynamic(&sc->sc_fmn_stid_evcnts[i],
			    EVCNT_TYPE_MISC, NULL, xname, fmn_stid_ev_names[i]);
		}
	}

	for (size_t i = 0; i < fmn_info.fmn_nbucket; i++) {
		evcnt_attach_dynamic(&sc->sc_fmn_cpu_evcnts[i],
		    EVCNT_TYPE_MISC, NULL, xname, fmn->fmn_bucket_names[i]);
	}
}

/*
 * This must be done in the context of the thread itself.
 */
void
rmixl_fmn_init_thread(void)
{
	fmn_info_t * const fmn = &fmn_info;
	struct cpu_info * const ci = curcpu();

	KASSERT(fmn->fmn_stinfo[0].si_name == NULL);
	for (size_t i = 1; i < fmn_info.fmn_nstid; i++) {
		if (fmn->fmn_stinfo[i].si_name != NULL) {
			snprintf(fmn_stid_ev_names[i],
			    sizeof(fmn_stid_ev_names[i]),
			    "fmn %s rx msgs", fmn->fmn_stinfo[i].si_name);
		}
	}

	if (CPU_IS_PRIMARY(ci)) {
		KASSERT(rmixl_intr_lock != NULL);
		/*
		 * establish dispatcher for FMN interrupt
		 */
		mutex_enter(rmixl_intr_lock);
		fmn->fmn_intr_vec = rmixl_intr_get_vec(IPL_VM);
		void * const ih = rmixl_vec_establish(fmn->fmn_intr_vec, NULL,
		    IPL_VM, fmn_intr, NULL, true);
		if (ih == NULL)
			panic("%s: rmixl_vec_establish failed", __func__);
		mutex_exit(rmixl_intr_lock);
	}
	/*
	 * do chip-dependent per-core FMN initialization
	 */
	switch(cpu_rmixl_chip_type(mips_options.mips_cpu)) {
#ifdef MIPS64_XLR
	case CIDFL_RMI_TYPE_XLR:
		fmn_init_thread_xlrxls(fmn);
		break;
#endif
#ifdef MIPS64_XLS
	case CIDFL_RMI_TYPE_XLS:
		fmn_init_thread_xlrxls(fmn);
		break;
#endif
#ifdef MIPS64_XLP
	case CIDFL_RMI_TYPE_XLP:
		fmn_init_thread_xlp(fmn);
		break;
#endif
	default:
		panic("%s: RMI chip type %#x unknown", __func__,
			cpu_rmixl_chip_type(mips_options.mips_cpu));
	}
}

void *
rmixl_fmn_intr_establish(size_t txstid, rmixl_fmn_intr_handler_t func,
	void *arg)
{
	fmn_info_t * const fmn = &fmn_info;
	volatile fmn_intrhand_t *ih = &fmn->fmn_intrhand[txstid];

	if (atomic_cas_ptr(&ih->ih_arg, fmn, arg) == fmn) {
		/*
		 * Now that the argument is updated, we can change func
		 */
		membar_producer();
		(void) atomic_swap_ptr(&ih->ih_func, func);
	} else {
#ifdef DEBUG
		panic("%s: intrhand[%zu] busy", __func__, txstid);
#endif
		ih = NULL;
	}

	return __UNVOLATILE(ih);
}

void
rmixl_fmn_intr_disestablish(void *cookie)
{
	fmn_info_t * const fmn = &fmn_info;
	volatile fmn_intrhand_t * const ih = cookie;
	rmixl_fmn_intr_handler_t func = ih->ih_func;

	if (atomic_cas_ptr(&ih->ih_func, func, fmn_stray_intr) == func) {
		/*
		 * Now that the old interrupt handler is no longer being
		 * called, we can swap out the argument.
		 */
		membar_producer();
		(void) atomic_swap_ptr(&ih->ih_arg, fmn);
#ifdef DEBUG
	} else {
		panic("%s: intrhand[%zd] not in use",
		    __func__, ih - fmn->fmn_intrhand);
#endif
	}
}

void
rmixl_fmn_intr_poll(u_int bucket, rmixl_fmn_rxmsg_t *rxmsg)
{
	uint32_t bit = 1 << bucket;
	uint32_t cp0_status;

	KASSERT(bucket < 8);

	cp0_status = rmixl_cp2_enable();

	for(;;) {
		rmixl_msgwait(bit);
		if (rmixl_fmn_msg_recv(bucket, rxmsg))
			break;
		DELAY(10);		/* XXX */
	}

	rmixl_cp2_restore(cp0_status);
}

size_t
rmixl_fmn_qid_to_stid(size_t qid)
{
	return fmn_info.fmn_qidtab[qid];
}

const char *
rmixl_fmn_stid_name(size_t stid)
{
	KASSERT(stid != 0);
	KASSERT(stid < fmn_info.fmn_nstid);
	return fmn_info.fmn_stinfo[stid].si_name;
}

static int
fmn_intr(void *arg)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	struct cpu_softc * const sc = curcpu()->ci_softc;

	KASSERT(sc->sc_fmn_si != NULL);
	softint_schedule(sc->sc_fmn_si);
	if (!is_xlp_p) {
		/*
		 * On the XLR and XLS, we can only stop interrupts on a per
		 * core basis but then there are no per-thread resources so
		 * it doesn't really hurt.
		 */
		const uint32_t cp0_status = rmixl_cp2_enable();
		uint32_t msg_cfg = mips_mfc2(RMIXL_COP_2_MSG_CFG, 0);
		msg_cfg &= ~(RMIXL_MSG_CFG0_EIE|RMIXL_MSG_CFG0_WIE);
		mips_mtc2(RMIXL_COP_2_MSG_CFG, 0, msg_cfg);
		rmixl_cp2_restore(cp0_status);
	}
	return 1;
}

static void
fmn_softint(void *arg)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	fmn_info_t * const fmn = &fmn_info;
	struct cpu_softc * const sc = curcpu()->ci_softc;
	uint32_t mask = 0;
	uint32_t processed = 0;

	const uint32_t cp0_status = rmixl_cp2_enable();

	for (;;) {
		if (mask == 0) {
			if (is_xlp_p) {
				mask = __SHIFTOUT(
				    ~mips_mfc2(RMIXLP_COP_2_MSG_RX_STS,0),
				    RMIXLP_MSG_RX_STS_RXQVCE);
			} else {
				mask = __SHIFTOUT(
				    ~mips_mfc2(RMIXL_COP_2_MSG_STS,0),
				    RMIXL_MSG_STS0_RFBE);
			}
			if (mask == 0)
				break;
		}

		DPRINTF(("%s: non-empty q-mask %#x\n", __func__, mask));

		const u_int rxq = ffs(mask) - 1;
		processed = (1 << rxq);

		sc->sc_fmn_cpu_evcnts[rxq].ev_count++;

		rmixl_fmn_rxmsg_t rxmsg;
		if (!rmixl_fmn_msg_recv(rxq, &rxmsg))
			continue;

		const size_t txstid = fmn->fmn_qidtab[rxmsg.rxsid];
		volatile fmn_intrhand_t * const ih = &fmn->fmn_intrhand[txstid];

		membar_consumer(); // make sure arg is loaded before func
		void * const ih_arg = ih->ih_arg;

		membar_consumer(); // make sure arg is loaded before func
		rmixl_fmn_intr_handler_t ih_func = ih->ih_func;

		if ((*ih_func)(ih_arg, &rxmsg) != 0)
			sc->sc_fmn_stid_evcnts[txstid].ev_count++;
	}

	if (is_xlp_p) {
		/*
		 * We need to set VC_PEND again so interrupts can get posted
		 * for the VCs we processed.  This register is global for all
		 * threads on a core so only ACK the VCs we processed.
		 */
		uint32_t msg_sts1 = mips_mfc2(RMIXLP_COP_2_MSG_STS1, 0);
		msg_sts1 |= __SHIFTOUT(processed, RMIXLP_MSG_STS1_VC_PEND);
		mips_mtc2(RMIXLP_COP_2_MSG_STS1, 0, msg_sts1);
	}

	rmixl_cp2_restore(cp0_status);
}

static bool
rmixl_fmn_clear_to_send_p(void)
{
	KASSERT(!cpu_rmixlp(mips_options.mips_cpu));
	uint32_t msg_sts;
	for (u_int try=16; try--; ) {
		msg_sts = mips_mfc2(RMIXL_COP_2_MSG_STS, 0);
		if ((msg_sts & (RMIXL_MSG_STS0_LPF|RMIXL_MSG_STS0_SPF|RMIXL_MSG_STS0_SMP)) == 0)
			return true;
		DELAY(10);		/* XXX ??? */
	}
	DIAG_PRF(("%s: cpu%u: sts=%#x: can't send\n",
	    __func__, cpu_number(), msg_sts));
	return false;
}

bool
rmixl_fmn_msg_send(u_int size, u_int code, u_int dest_id, u_int dest_vc,
	const rmixl_fmn_msg_t *msg)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	bool rv = false;	/* assume failure */

	KASSERT(1 <= size && size <= 4);
	KASSERT(code < 0x100);
	KASSERT(dest_id < (is_xlp_p ? 0x1000 : 0x80));
	KASSERT(dest_vc == 0 || (is_xlp_p && (dest_id & 0x380) == 0x80));

	const uint32_t cp0_status = rmixl_cp2_enable();

	switch (size) {
	case 1:
		mips_dmtc2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		break;
	case 2:
		mips_dmtc2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		mips_dmtc2(RMIXL_COP_2_TXBUF, 1, msg->data[1]);
		break;
	case 3:
		mips_dmtc2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		mips_dmtc2(RMIXL_COP_2_TXBUF, 1, msg->data[1]);
		mips_dmtc2(RMIXL_COP_2_TXBUF, 2, msg->data[2]);
		break;
	case 4:
		mips_dmtc2(RMIXL_COP_2_TXBUF, 0, msg->data[0]);
		mips_dmtc2(RMIXL_COP_2_TXBUF, 1, msg->data[1]);
		mips_dmtc2(RMIXL_COP_2_TXBUF, 2, msg->data[2]);
		mips_dmtc2(RMIXL_COP_2_TXBUF, 3, msg->data[3]);
		break;
	default:
		DIAG_PRF(("%s: bad size %d", __func__, size));
		goto out;
	}

	if (!is_xlp_p && !rmixl_fmn_clear_to_send_p()) {
		goto out;
	}
	uint32_t desc;
	if (is_xlp_p) {
		desc = RMIXLP_MSGSND_DESC(size, code, dest_id, dest_vc);
	} else {
		desc = RMIXL_MSGSND_DESC(size, code, dest_id);
	}
	DPRINTF(("%s: cpu%u, desc %#x\n", __func__, cpu_number(), desc));
	rv = -1;	/* assume failure */
	for (size_t try=16; try--; ) {
		if (is_xlp_p) {
			if ((rv == rmixlp_msgsnd(desc)) != false)
				break;
			uint32_t tx_sts = mips_mfc2(RMIXLP_COP_2_MSG_TX_STS, 0);
			const uint32_t bad_tx_sts = RMIXLP_MSG_TX_STS_PS
			    | RMIXLP_MSG_TX_STS_IQC | RMIXLP_MSG_TX_STS_OQC;
			KASSERT((tx_sts & bad_tx_sts) != 0);
			tx_sts &= ~bad_tx_sts;
			mips_mtc2(RMIXLP_COP_2_MSG_TX_STS, 0, tx_sts);
		} else {
			rmixl_msgsnd(desc);

			uint32_t msg_sts0 = mips_mfc2(RMIXL_COP_2_MSG_STS, 0);
			uint32_t msg_sts1 = mips_mfc2(RMIXL_COP_2_MSG_STS, 1);

			if ((msg_sts0 & RMIXL_MSG_STS0_SCF) == 0
			    && (msg_sts1 & RMIXL_MSG_STS1_ERRS) == 0) {
				rv = 0;
				break;
			}

			/* clear status1 error(s) */
			if ((msg_sts1 & RMIXL_MSG_STS1_ERRS) != 0) {
				msg_sts1 = mips_mfc2(RMIXL_COP_2_MSG_STS, 1);
				mips_mtc2(RMIXL_COP_2_MSG_STS, 1, msg_sts1);
			}
			DIAG_PRF(("%s: src=%ld, dst=%d, sts=%#x/%#x: send error, try %zu\n",
				__func__, curcpu()->ci_cpuid, dest_id, msg_sts0, msg_sts1, try));
		}
		DELAY(10);
	}
 out:
	rmixl_cp2_restore(cp0_status);

	return rv;
}

static int
fmn_msgld_rmixlp(u_int rxq, rmixl_fmn_rxmsg_t *rxmsg)
{
	bool rv = false;
	for (u_int try=16; !rv && try; try--) {
		rv = rmixlp_msgld(rxq);
	}
	if (!rv)
		return rv;

	uint32_t msg_sts = mips_mfc2(RMIXLP_COP_2_MSG_RX_STS, 0);
	rxmsg->rxsid = __SHIFTOUT(msg_sts, RMIXLP_MSG_RX_STS_SID);
	rxmsg->code = __SHIFTOUT(msg_sts, RMIXLP_MSG_RX_STS_SC);
	rxmsg->size = __SHIFTOUT(msg_sts, RMIXLP_MSG_RX_STS_SM1) + 1;
	return true;
}

static bool
fmn_msgld_rmixl(u_int rxq, rmixl_fmn_rxmsg_t *rxmsg)
{
	uint32_t msg_sts;

	for (u_int try=16; try--; ) {
		msg_sts = mips_mfc2(RMIXL_COP_2_MSG_STS, 0);
		if ((msg_sts & RMIXL_MSG_STS0_LPF) == 0) {
			break;
		}
	}
	if (msg_sts & RMIXL_MSG_STS0_LPF) {
		DIAG_PRF(("%s: cpu%u, rxq=%d, sts=%#x: Load Pending Fail\n",
			__func__, cpu_number(), rxq, msg_sts));
		return false;
	}
	rmixl_msgld(rxq);
	msg_sts = mips_mfc2(RMIXL_COP_2_MSG_STS, 0);
	DPRINTF(("%s: cpu%u, rxq=%d, sts=%#x\n",
		__func__, cpu_number(), rxq, msg_sts));
	if (msg_sts & (RMIXL_MSG_STS0_LEF|RMIXL_MSG_STS0_LPF))
		return false;

	rxmsg->rxsid = __SHIFTOUT(msg_sts, RMIXL_MSG_STS0_RMSID);
	rxmsg->code = __SHIFTOUT(msg_sts, RMIXL_MSG_STS0_RMSC);
	rxmsg->size = __SHIFTOUT(msg_sts, RMIXL_MSG_STS0_RMS) + 1;
	return true;
}

/*
 * rmixl_fmn_msg_recv
 *
 *	- assume cp2 access is already enabled
 */
bool
rmixl_fmn_msg_recv(u_int rxq, rmixl_fmn_rxmsg_t *rxmsg)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);
	bool rv = false;

	KASSERT(!is_xlp_p || rxq < 4);

	if (is_xlp_p) {
		rv = fmn_msgld_rmixlp(rxq, rxmsg);
	} else {
		rv = fmn_msgld_rmixl(rxq, rxmsg);
	}
	if (rv) {
		switch(rxmsg->size) {
		case 1:
			rxmsg->msg.data[0] = mips_dmfc2(RMIXL_COP_2_RXBUF, 0);
			break;
		case 2:
			rxmsg->msg.data[0] = mips_dmfc2(RMIXL_COP_2_RXBUF, 0);
			rxmsg->msg.data[1] = mips_dmfc2(RMIXL_COP_2_RXBUF, 1);
			break;
		case 3:
			rxmsg->msg.data[0] = mips_dmfc2(RMIXL_COP_2_RXBUF, 0);
			rxmsg->msg.data[1] = mips_dmfc2(RMIXL_COP_2_RXBUF, 1);
			rxmsg->msg.data[2] = mips_dmfc2(RMIXL_COP_2_RXBUF, 2);
			break;
		case 4:
			rxmsg->msg.data[0] = mips_dmfc2(RMIXL_COP_2_RXBUF, 0);
			rxmsg->msg.data[1] = mips_dmfc2(RMIXL_COP_2_RXBUF, 1);
			rxmsg->msg.data[2] = mips_dmfc2(RMIXL_COP_2_RXBUF, 2);
			rxmsg->msg.data[3] = mips_dmfc2(RMIXL_COP_2_RXBUF, 3);
			break;
		default:
			/* "impossible" due to bitfield width */
			panic("%s: bad size %d", __func__, rxmsg->size);
		}
	}

	return rv;
}

#ifdef FMN_DEBUG
void
rmixl_fmn_cp2_dump(void)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);

	const uint32_t cp0_status = rmixl_cp2_enable();

	COP2_PRINT_8(RMIXL_COP_2_TXBUF, 0);
	COP2_PRINT_8(RMIXL_COP_2_TXBUF, 1);
	COP2_PRINT_8(RMIXL_COP_2_TXBUF, 2);
	COP2_PRINT_8(RMIXL_COP_2_TXBUF, 3);

	COP2_PRINT_8(RMIXL_COP_2_RXBUF, 0);
	COP2_PRINT_8(RMIXL_COP_2_RXBUF, 1);
	COP2_PRINT_8(RMIXL_COP_2_RXBUF, 2);
	COP2_PRINT_8(RMIXL_COP_2_RXBUF, 3);

	if (is_xlp_p) {
		COP2_PRINT_4(RMIXLP_COP_2_MSG_TX_STS, 0);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_RX_STS, 0);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_STS1, 0);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_CFG, 0);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_ERR, 0);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_ERR, 1);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_ERR, 2);
		COP2_PRINT_4(RMIXLP_COP_2_MSG_ERR, 3);
		COP2_PRINT_8(RMIXLP_COP_2_CREDITS, 0);
		COP2_PRINT_4(RMIXLP_COP_2_CREDITS, 1);
	} else {

		COP2_PRINT_4(RMIXL_COP_2_MSG_STS, 0);
		COP2_PRINT_4(RMIXL_COP_2_MSG_STS, 1);

		COP2_PRINT_4(RMIXL_COP_2_MSG_CFG, 0);
		COP2_PRINT_4(RMIXL_COP_2_MSG_CFG, 1);

		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 0);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 1);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 2);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 3);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 4);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 5);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 6);
		COP2_PRINT_4(RMIXL_COP_2_MSG_BSZ, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 0, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 1, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 2, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 3, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 4, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 5, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 6, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 7, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 8, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 9, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 10, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 11, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 12, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 13, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 14, 7);

		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 0);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 1);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 2);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 3);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 4);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 5);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 6);
		COP2_PRINT_4(RMIXL_COP_2_CREDITS + 15, 7);
	}

	rmixl_cp2_restore(cp0_status);
}


void
rmixl_fmn_cc_dump(void)
{
	const bool is_xlp_p = cpu_rmixlp(mips_options.mips_cpu);

	if (is_xlp_p) {
	} else {
		uint32_t cc[4][8];

		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS,   0, &cc[0][0]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS,   4, &cc[0][4]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS+1, 0, &cc[1][0]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS+1, 4, &cc[1][4]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS+2, 0, &cc[2][0]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS+2, 4, &cc[2][4]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS+3, 0, &cc[3][0]);
		FMN_COP2_4SEL_READ(RMIXL_COP_2_CREDITS+3, 4, &cc[3][4]);

		printf("%s: cpu%u\n", __func__, cpu_number());
		for (size_t i=0; i < 4; i++) {
			for (size_t j=0; j < 8; j++)
				printf(" %#x,", cc[i][j]);
			printf("\n");
		}
	}
}
#endif	/* FMN_DEBUG */
