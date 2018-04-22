/*	$NetBSD: octeonvar.h,v 1.5.18.1 2018/04/22 07:20:18 pgoyette Exp $	*/

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
/*
 * subbits = __BITS64_GET(XXX, bits);
 * bits = __BITS64_SET(XXX, subbits);
 */
#ifndef	__BITS64_GET
#define	__BITS64_GET(name, bits)	\
	    (((uint64_t)(bits) & name) >> name##_SHIFT)
#endif
#ifndef	__BITS64_SET
#define	__BITS64_SET(name, subbits)	\
	    (((uint64_t)(subbits) << name##_SHIFT) & name)
#endif

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

#define NIRQS	64

struct cpu_softc {
	struct cpu_info *cpu_ci;

	uint64_t cpu_int0_sum0;
	uint64_t cpu_int1_sum0;
	uint64_t cpu_int2_sum0;

	uint64_t cpu_int0_en0;
	uint64_t cpu_int1_en0;
	uint64_t cpu_int2_en0;

	uint64_t cpu_int0_en1;
	uint64_t cpu_int1_en1;
	uint64_t cpu_int2_en1;

	uint64_t cpu_int32_en;

	struct evcnt cpu_intr_evs[NIRQS];

	uint64_t cpu_int0_enable0;
	uint64_t cpu_int1_enable0;
	uint64_t cpu_int2_enable0;

	void *cpu_wdog_sih;		// wdog softint handler
	uint64_t cpu_wdog;
	uint64_t cpu_pp_poke;

#ifdef MULTIPROCESSOR
	uint64_t cpu_mbox_set;
	uint64_t cpu_mbox_clr;
#endif
};

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
struct octeon_cvmseg_map {
	/* 0-3 */
	uint64_t		csm_xxx_0;
	uint64_t		csm_xxx_1;
	uint64_t		csm_xxx_2;
	uint64_t		csm_pow_intr;

	/* 4-19 */
	struct octeon_cvmseg_ether_map {
		uint64_t	csm_ether_fau_req;
		uint64_t	csm_ether_fau_done;
		uint64_t	csm_ether_fau_cmdptr;
		uint64_t	csm_ether_xxx_3;
	} csm_ether[4/* XXX */];

	/* 20-32 */
	uint64_t	xxx_20_32[32 - 20];
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
struct octeon_fau_map {
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

#define	OCTEON_POW_GROUP_PIP		0
#define	OCTEON_POW_GROUP_XXX_1		1
#define	OCTEON_POW_GROUP_XXX_2		2
#define	OCTEON_POW_GROUP_XXX_3		3
#define	OCTEON_POW_GROUP_XXX_4		4
#define	OCTEON_POW_GROUP_XXX_5		5
#define	OCTEON_POW_GROUP_XXX_6		6
#define	OCTEON_POW_GROUP_CORE1_SEND	7
#define	OCTEON_POW_GROUP_CORE1_TASK_0	8
#define	OCTEON_POW_GROUP_CORE1_TASK_1	9
#define	OCTEON_POW_GROUP_CORE1_TASK_2	10
#define	OCTEON_POW_GROUP_CORE1_TASK_3	11
#define	OCTEON_POW_GROUP_CORE1_TASK_4	12
#define	OCTEON_POW_GROUP_CORE1_TASK_5	13
#define	OCTEON_POW_GROUP_CORE1_TASK_6	14
#define	OCTEON_POW_GROUP_CORE1_TASK_7	15

#ifdef _KERNEL
extern struct octeon_config	octeon_configuration;
#ifdef MULTIPROCESSOR
extern kcpuset_t *cpus_booted;
extern struct cpu_softc		octeon_cpu1_softc;
#endif

void	octeon_bus_io_init(bus_space_tag_t, void *);
void	octeon_bus_mem_init(bus_space_tag_t, void *);
void	octeon_cal_timer(int);
void	octeon_dma_init(struct octeon_config *);
void	octeon_intr_init(struct cpu_info *);
void	octeon_iointr(int, vaddr_t, uint32_t);
void	octeon_pci_init(pci_chipset_tag_t, struct octeon_config *);
void	*octeon_intr_establish(int, int, int (*)(void *), void *);
void	octeon_intr_disestablish(void *cookie);

void	octeon_reset_vector(void);
uint64_t mips_cp0_cvmctl_read(void);
void	 mips_cp0_cvmctl_write(uint64_t);

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
/* XXX backward compatibility */
#if 1
#define	OCT_SYNCIOBDMA		OCTEON_SYNCIOBDMA
#define	OCT_SYNCW		OCTEON_SYNCW
#define	OCT_SYNC		OCTEON_SYNC
#define	OCT_SYNCWS		OCTEON_SYNCWS
#define	OCT_SYNCS		OCTEON_SYNCS
#endif

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

/* XXX backward compatibility */
#if 1
#define octeon_read_csr(address) \
	octeon_xkphys_read_8(address)
#define octeon_write_csr(address, value) \
	octeon_xkphys_write_8(address, value)
#endif

static __inline void
octeon_iobdma_write_8(uint64_t value)
{
	uint64_t addr = UINT64_C(0xffffffffffffa200);

	octeon_xkphys_write_8(addr, value);
}

static __inline uint64_t
octeon_cvmseg_read_8(size_t offset)
{
	return octeon_xkphys_read_8(UINT64_C(0xffffffffffff8000) + offset);
}

static __inline void
octeon_cvmseg_write_8(size_t offset, uint64_t value)
{
	octeon_xkphys_write_8(UINT64_C(0xffffffffffff8000) + offset, value);
}

/* XXX */
static __inline uint32_t
octeon_disable_interrupt(uint32_t *new)
{
	uint32_t s, tmp;
        
	__asm __volatile (
		_ASM_PROLOGUE
		"	mfc0	%[s], $12		\n"
		"	and	%[tmp], %[s], ~1	\n"
		"	mtc0	%[tmp], $12		\n"
		_ASM_EPILOGUE
		: [s]"=&r"(s), [tmp]"=&r"(tmp));
	if (new)
		*new = tmp;
	return s;
}

/* XXX */
static __inline void
octeon_restore_status(uint32_t s)
{
	__asm __volatile (
		_ASM_PROLOGUE
		"	mtc0	%[s], $12		\n"
		_ASM_EPILOGUE
		:: [s]"r"(s));
}

static __inline uint64_t
octeon_get_cycles(void)
{ 
#if defined(__mips_o32)
	uint32_t s, lo, hi;
  
	s = octeon_disable_interrupt((void *)0);
	__asm __volatile (
		_ASM_PROLOGUE_MIPS64
		"	dmfc0	%[lo], $9, 6		\n"
		"	add	%[hi], %[lo], $0	\n"
		"	srl	%[hi], 32		\n"
		"	sll	%[lo], 32		\n"
		"	srl	%[lo], 32		\n"
		_ASM_EPILOGUE
		: [lo]"=&r"(lo), [hi]"=&r"(hi));
	octeon_restore_status(s);
	return ((uint64_t)hi << 32) + (uint64_t)lo;
#else
	uint64_t tmp;

	__asm __volatile (
		_ASM_PROLOGUE_MIPS64
		"	dmfc0	%[tmp], $9, 6		\n"
		_ASM_EPILOGUE
		: [tmp]"=&r"(tmp));
	return tmp;
#endif
}

/* -------------------------------------------------------------------------- */

/* ---- event counter */

#if defined(OCTEON_ETH_DEBUG)
#define	OCTEON_EVCNT_INC(sc, name) \
	do { (sc)->sc_ev_##name.ev_count++; } while (0)
#define	OCTEON_EVCNT_ADD(sc, name, n) \
	do { (sc)->sc_ev_##name.ev_count += (n); } while (0)
#define	OCTEON_EVCNT_ATTACH_EVCNTS(sc, entries, devname) \
do {								\
	int i;							\
	const struct octeon_evcnt_entry *ee;			\
								\
	for (i = 0; i < (int)__arraycount(entries); i++) {	\
		ee = &(entries)[i];				\
		evcnt_attach_dynamic(				\
		    (struct evcnt *)((uintptr_t)(sc) + ee->ee_offset), \
		    ee->ee_type, ee->ee_parent, devname,	\
		    ee->ee_name);				\
	}							\
} while (0)
#else
#define	OCTEON_EVCNT_INC(sc, name)
#define	OCTEON_EVCNT_ADD(sc, name, n)
#define	OCTEON_EVCNT_ATTACH_EVCNTS(sc, entries, devname)
#endif

struct octeon_evcnt_entry {
	size_t		ee_offset;
	int		ee_type;
	struct evcnt	*ee_parent;
	const char	*ee_name;
};

#define	OCTEON_EVCNT_ENTRY(_sc_type, _var, _ev_type, _parent, _name) \
	{							\
		.ee_offset = offsetof(_sc_type, sc_ev_##_var),	\
		.ee_type = EVCNT_TYPE_##_ev_type,		\
		.ee_parent = _parent,				\
		.ee_name = _name				\
	}

#endif	/* _MIPS_OCTEON_OCTEONVAR_H_ */
