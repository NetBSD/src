/*	$NetBSD: octeonvar.h,v 1.18 2022/01/26 11:48:54 andvar Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _MIPS_OCTEON_OCTEONVAR_H_
#define _MIPS_OCTEON_OCTEONVAR_H_

#include <sys/bus.h>
#include <sys/evcnt.h>
#include <sys/kcpuset.h>
#include <mips/locore.h>
#include <dev/pci/pcivar.h>

#include <mips/cavium/octeonreg.h>
#include <mips/cache_octeon.h>

/* XXX elsewhere */
#define	_ASM_PROLOGUE \
		"	.set push			\n" \
		"	.set noreorder			\n"
#define	_ASM_PROLOGUE_MIPS64 \
		_ASM_PROLOGUE				\
		"	.set mips64			\n"
#define	_ASM_PROLOGUE_OCTEON \
		_ASM_PROLOGUE				\
		"	.set arch=octeon		\n"
#define	_ASM_EPILOGUE \
		"	.set pop			\n"

#ifdef _KERNEL
extern int	octeon_core_ver;
#endif /* _KERNEL */
#define	OCTEON_1		1
#define	OCTEON_PLUS		10	/* arbitrary, keep sequence for others */
#define	OCTEON_2		2
#define	OCTEON_3		3

struct octeon_config {
	struct mips_bus_space mc_iobus_bust;
	struct mips_bus_space mc_bootbus_bust;
	struct mips_pci_chipset mc_pc;

	struct mips_bus_dma_tag mc_iobus_dmat;
	struct mips_bus_dma_tag mc_bootbus_dmat;
	struct mips_bus_dma_tag mc_core1_dmat;
	struct mips_bus_dma_tag mc_fpa_dmat;

	struct extent *mc_io_ex;
	struct extent *mc_mem_ex;

	int	mc_mallocsafe;
};

#define NIRQS	128
#define NBANKS	2

struct cpu_softc {
	struct cpu_info *cpu_ci;

	uint64_t cpu_ip2_sum0;
	uint64_t cpu_ip3_sum0;
	uint64_t cpu_ip4_sum0;

	uint64_t cpu_int_sum1;

	uint64_t cpu_ip2_en[NBANKS];
	uint64_t cpu_ip3_en[NBANKS];
	uint64_t cpu_ip4_en[NBANKS];

	uint64_t cpu_ip2_enable[NBANKS];
	uint64_t cpu_ip3_enable[NBANKS];
	uint64_t cpu_ip4_enable[NBANKS];

	struct evcnt cpu_intr_evs[NIRQS];

	void *cpu_wdog_sih;		// wdog softint handler
	uint64_t cpu_wdog;
	uint64_t cpu_pp_poke;

#ifdef MULTIPROCESSOR
	uint64_t cpu_mbox_set;
	uint64_t cpu_mbox_clr;
#endif
} __aligned(OCTEON_CACHELINE_SIZE);

/*
 * FPA map
 */

#define	OCTEON_POOL_NO_PKT	0
#define	OCTEON_POOL_NO_WQE	1
#define	OCTEON_POOL_NO_CMD	2
#define	OCTEON_POOL_NO_SG	3
#define	OCTEON_POOL_NO_XXX_4	4
#define	OCTEON_POOL_NO_XXX_5	5
#define	OCTEON_POOL_NO_XXX_6	6
#define	OCTEON_POOL_NO_DUMP	7	/* FPA debug dump */

#define	OCTEON_POOL_SIZE_PKT	2048	/* 128 x 16 */
#define	OCTEON_POOL_SIZE_WQE	128	/* 128 x 1 */
#define	OCTEON_POOL_SIZE_CMD	1024	/* 128 x 8 */
#define	OCTEON_POOL_SIZE_SG	512	/* 128 x 4 */
#define	OCTEON_POOL_SIZE_XXX_4	0
#define	OCTEON_POOL_SIZE_XXX_5	0
#define	OCTEON_POOL_SIZE_XXX_6	0
#define	OCTEON_POOL_SIZE_XXX_7	0

#define	OCTEON_POOL_NELEMS_PKT		4096
#define	OCTEON_POOL_NELEMS_WQE		4096
#define	OCTEON_POOL_NELEMS_CMD		32
#define	OCTEON_POOL_NELEMS_SG		1024
#define	OCTEON_POOL_NELEMS_XXX_4	0
#define	OCTEON_POOL_NELEMS_XXX_5	0
#define	OCTEON_POOL_NELEMS_XXX_6	0
#define	OCTEON_POOL_NELEMS_XXX_7	0

/*
 * CVMSEG (``scratch'') memory map
 */

#define CVMSEG_LM_RNM_SIZE	16	/* limited by CN70XX hardware (why?) */
#define CVMSEG_LM_ETHER_COUNT	4	/* limits number of cnmac devices */

struct octeon_cvmseg_map {
	uint64_t		csm_pow_intr;

	struct octeon_cvmseg_ether_map {
		uint64_t	csm_ether_fau_done;
	} csm_ether[CVMSEG_LM_ETHER_COUNT];

	uint64_t	csm_rnm[CVMSEG_LM_RNM_SIZE];
} __packed;
#define	OCTEON_CVMSEG_OFFSET(entry) \
	offsetof(struct octeon_cvmseg_map, entry)
#define	OCTEON_CVMSEG_ETHER_OFFSET(n, entry) \
	(offsetof(struct octeon_cvmseg_map, csm_ether) + \
	 sizeof(struct octeon_cvmseg_ether_map) * (n) + \
	 offsetof(struct octeon_cvmseg_ether_map, entry))

/*
 * FAU register map
 *
 * => FAU registers exist in FAU unit
 * => devices (PKO) can access these registers
 * => CPU can read those values after loading them into CVMSEG
 */
struct octfau_map {
	struct {
		/* PKO command index */
		uint64_t	_fau_map_port_pkocmdidx;
		/* send requested */
		uint64_t	_fau_map_port_txreq;
		/* send completed */
		uint64_t	_fau_map_port_txdone;
		/* XXX */
		uint64_t	_fau_map_port_pad;
	} __packed _fau_map_port[3];
};

/*
 * POW qos/group map
 */

#define	OCTEON_POW_QOS_PIP		0
#define	OCTEON_POW_QOS_CORE1		1
#define	OCTEON_POW_QOS_XXX_2		2
#define	OCTEON_POW_QOS_XXX_3		3
#define	OCTEON_POW_QOS_XXX_4		4
#define	OCTEON_POW_QOS_XXX_5		5
#define	OCTEON_POW_QOS_XXX_6		6
#define	OCTEON_POW_QOS_XXX_7		7

#define	OCTEON_POW_GROUP_MAX		16

#ifdef _KERNEL
extern struct octeon_config	octeon_configuration;

const char	*octeon_cpu_model(mips_prid_t);

void		octeon_bus_io_init(bus_space_tag_t, void *);
void		octeon_bus_mem_init(bus_space_tag_t, void *);
void		octeon_cal_timer(int);
void		octeon_dma_init(struct octeon_config *);
void		octeon_intr_init(struct cpu_info *);
void		octeon_iointr(int, vaddr_t, uint32_t);
void		octpci_init(pci_chipset_tag_t, struct octeon_config *);
void		*octeon_intr_establish(int, int, int (*)(void *), void *);
void		octeon_intr_disestablish(void *cookie);

int		octeon_ioclock_speed(void);
void		octeon_soft_reset(void);

void		octeon_reset_vector(void);
uint64_t	mips_cp0_cvmctl_read(void);
void		mips_cp0_cvmctl_write(uint64_t);
#endif /* _KERNEL */

#if defined(__mips_n32)
#define ffs64	__builtin_ffsll
#elif defined(_LP64)
#define ffs64	__builtin_ffsl
#else
#error unknown ABI
#endif

/* 
 * Prefetch
 *
 *	OCTEON_PREF		normal (L1 and L2)
 *	OCTEON_PREF_L1		L1 only
 *	OCTEON_PREF_L2		L2 only
 *	OCTEON_PREF_DWB		don't write back
 *	OCTEON_PREF_PFS		prepare for store
 */
#define __OCTEON_PREF_N(n, base, offset)			\
	__asm __volatile (					\
		"	.set	push				\
		"	.set	arch=octeon			\n" \
		"	pref	"#n", "#offset"(%[base])	\n" \
		"	.set	pop				\
		: : [base] "d" (base)				\
	)
#define __OCTEON_PREF_0(base, offset)	__OCTEON_PREF_N(0, base, offset)
#define __OCTEON_PREF_4(base, offset)	__OCTEON_PREF_N(4, base, offset)
#define __OCTEON_PREF_28(base, offset)	__OCTEON_PREF_N(28, base, offset)
#define __OCTEON_PREF_29(base, offset)	__OCTEON_PREF_N(29, base, offset)
#define __OCTEON_PREF_30(base, offset)	__OCTEON_PREF_N(30, base, offset)
#define OCTEON_PREF(base, offset)	__OCTEON_PREF_0(base, offset)
#define OCTEON_PREF_L1(base, offset)	__OCTEON_PREF_4(base, offset)
#define OCTEON_PREF_L2(base, offset)	__OCTEON_PREF_28(base, offset)
#define OCTEON_PREF_DWB(base, offset)	__OCTEON_PREF_29(base, offset)
#define OCTEON_PREF_PFS(base, offset)	__OCTEON_PREF_30(base, offset)

/*
 * Sync
 */
#define OCTEON_SYNCCOMMON(name) \
	__asm __volatile ( \
		_ASM_PROLOGUE_OCTEON			\
		"	"#name"				\n" \
		_ASM_EPILOGUE				\
		::: "memory")
#define OCTEON_SYNCIOBDMA	OCTEON_SYNCCOMMON(synciobdma)
#define OCTEON_SYNCW		OCTEON_SYNCCOMMON(syncw)
#define OCTEON_SYNC		OCTEON_SYNCCOMMON(sync)
#define OCTEON_SYNCWS		OCTEON_SYNCCOMMON(syncws)
#define OCTEON_SYNCS		OCTEON_SYNCCOMMON(syncs)

/* octeon core does not use cca to determine cacheability */
#define OCTEON_CCA_NONE UINT64_C(0)

static __inline uint64_t
octeon_xkphys_read_8(paddr_t address)
{
	return mips3_ld(MIPS_PHYS_TO_XKPHYS(OCTEON_CCA_NONE, address));
}

static __inline void
octeon_xkphys_write_8(paddr_t address, uint64_t value)
{
	mips3_sd(MIPS_PHYS_TO_XKPHYS(OCTEON_CCA_NONE, address), value);
}

static __inline void
octeon_iobdma_write_8(uint64_t value)
{

	octeon_xkphys_write_8(OCTEON_IOBDMA_ADDR, value);
}

static __inline uint64_t
octeon_cvmseg_read_8(size_t offset)
{

	return octeon_xkphys_read_8(OCTEON_CVMSEG_LM + offset);
}

static __inline void
octeon_cvmseg_write_8(size_t offset, uint64_t value)
{

	octeon_xkphys_write_8(OCTEON_CVMSEG_LM + offset, value);
}

#endif	/* _MIPS_OCTEON_OCTEONVAR_H_ */
