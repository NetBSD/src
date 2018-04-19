/*	$NetBSD: cpuvar.h,v 1.19 2018/04/19 21:50:07 christos Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_CPUVAR_H_
#define _POWERPC_BOOKE_CPUVAR_H_

#include <sys/bus.h>
#include <prop/proplib.h>
#include <powerpc/psl.h>

struct cpunode_softc {
	device_t sc_dev;
	u_int sc_children;
};

struct cpu_softc {
	struct cpu_info *cpu_ci;
	struct evcnt *cpu_evcnt_intrs;
	bus_space_tag_t cpu_bst;
	bus_space_tag_t cpu_le_bst;
	bus_space_handle_t cpu_bsh;
	bus_addr_t cpu_clock_gtbcr;

	paddr_t cpu_highmem;

	u_int cpu_pcpls[5];
	struct evcnt cpu_evcnt_spurious_intr;

	struct evcnt cpu_ev_late_clock;
	u_long cpu_ticks_per_clock_intr;
	struct evcnt cpu_ev_exec_trap_sync;

	uint64_t cpu_spl_tb[NIPL][NIPL];
};

struct cpunode_locators {
	const char *cnl_name;
	bus_addr_t cnl_addr;
	bus_size_t cnl_size;
	uint8_t cnl_instance;
	uint8_t cnl_nintr;
	uint8_t cnl_intrs[4];
	uint32_t cnl_flags;
	uint16_t cnl_ids[6];
};

struct cpunode_attach_args {
	const char *cna_busname;
	bus_space_tag_t cna_memt;
	bus_space_tag_t cna_le_memt;
	bus_dma_tag_t cna_dmat;
	struct cpunode_locators cna_locs;
	u_int cna_childmask;
};

struct mainbus_attach_args {
	const char *ma_name;
	bus_space_tag_t ma_memt;
	bus_space_tag_t ma_le_memt;
	bus_dma_tag_t ma_dmat;
	int ma_node;
};

struct generic_attach_args {
	const char *ga_name;
	bus_space_tag_t ga_bst;
	bus_dma_tag_t ga_dmat;
	bus_addr_t ga_addr;
	bus_size_t ga_size;
	int ga_cs;
	int ga_irq;
};

#ifndef __BSD_PT_ENTRY_T
#define __BSD_PT_ENTRY_T	__uint32_t
typedef __BSD_PT_ENTRY_T	pt_entry_t;
#define PRIxPTE			PRIx32
#endif

#include <uvm/pmap/tlb.h>

struct tlb_md_io_ops {
	/*
	 * We need mapiodev to be first so we can easily override it in
	 * early boot by doing cpu_md_ops.tlb_md_ops = (const struct
	 * tlb_md_ops *) &<variable containing mapiodev pointer>.
	 */
	void *(*md_tlb_mapiodev)(paddr_t, psize_t, bool);
	void (*md_tlb_unmapiodev)(vaddr_t, vsize_t);
	int (*md_tlb_ioreserve)(vaddr_t, vsize_t, uint32_t);
	int (*md_tlb_iorelease)(vaddr_t);
};

struct cpu_md_ops {
	const struct cpunode_locators *md_cpunode_locs;
	void (*md_cpu_attach)(device_t, u_int);

	void (*md_device_register)(device_t, void *);
	void (*md_cpu_startup)(void);
	void (*md_cpu_reset)(void);
	void (*md_cpunode_attach)(device_t, device_t, void *);

	const struct tlb_md_ops *md_tlb_ops;
	const struct tlb_md_io_ops *md_tlb_io_ops;
};


#ifdef _KERNEL

static __inline register_t
wrtee(register_t msr)
{
	register_t old_msr;
	__asm("mfmsr\t%0" : "=r"(old_msr));

	if (__builtin_constant_p(msr)) {
		__asm __volatile("wrteei\t%0" :: "n"((msr & PSL_EE) ? 1 : 0));
	} else {
		__asm __volatile("wrtee\t%0" :: "r"(msr));
	}
	return old_msr;
}

uint32_t ufetch_32(const void *);

struct trapframe;
void	booke_sstep(struct trapframe *);

void	booke_cpu_startup(const char *);	/* model name */
extern struct powerpc_bus_dma_tag booke_bus_dma_tag;

extern struct cpu_info cpu_info[];
#ifdef MULTIPROCESSOR
extern volatile struct cpu_hatch_data cpu_hatch_data;
#endif

void	cpu_evcnt_attach(struct cpu_info *);
uint32_t cpu_read_4(bus_size_t);
uint8_t	cpu_read_1(bus_size_t);
void	cpu_write_4(bus_size_t, uint32_t);
void	cpu_write_1(bus_size_t, uint8_t);

void	dump_splhist(struct cpu_info *, void (*)(const char *, ...));
void	calc_delayconst(void);

struct intrsw;
void	exception_init(const struct intrsw *);

void	*tlb_mapiodev(paddr_t, psize_t, bool);
void	tlb_unmapiodev(vaddr_t, vsize_t);
int	tlb_ioreserve(vaddr_t, vsize_t, pt_entry_t);
int	tlb_iorelease(vaddr_t);

extern struct cpu_md_ops cpu_md_ops;

void	board_info_init(void);
void	board_info_add_number(const char *, uint64_t);
void	board_info_add_data(const char *, const void *, size_t);
void	board_info_add_string(const char *, const char *);
void	board_info_add_bool(const char *);
void	board_info_add_object(const char *, void *);
uint64_t board_info_get_number(const char *);
bool	board_info_get_bool(const char *);
void	*board_info_get_object(const char *);
const void *
	board_info_get_data(const char *, size_t *);

/* trap.c */
void	dump_trapframe(const struct trapframe *, void (*)(const char *, ...));

extern char root_string[];
extern paddr_t msgbuf_paddr;
extern prop_dictionary_t board_properties;
extern psize_t pmemsize;
#endif

#endif /* !_POWERPC_BOOKE_CPUVAR_H_ */
