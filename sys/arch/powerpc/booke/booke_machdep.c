/*	$NetBSD: booke_machdep.c,v 1.21 2014/09/22 21:35:15 matt Exp $	*/
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

#define	__INTR_PRIVATE
#define	_POWERPC_BUS_DMA_PRIVATE

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: booke_machdep.c,v 1.21 2014/09/22 21:35:15 matt Exp $");

#include "opt_modular.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/mount.h>
#include <sys/msgbuf.h>
#include <sys/kernel.h>
#include <sys/reboot.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <powerpc/pcb.h>
#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#include <powerpc/booke/cpuvar.h>

/*
 * Global variables used here and there
 */
paddr_t msgbuf_paddr;
psize_t pmemsize;
struct vm_map *phys_map;

#ifdef MODULAR
register_t cpu_psluserset = PSL_USERSET;
register_t cpu_pslusermod = PSL_USERMOD;
register_t cpu_pslusermask = PSL_USERMASK;
#endif

static bus_addr_t booke_dma_phys_to_bus_mem(bus_dma_tag_t, bus_addr_t);
static bus_addr_t booke_dma_bus_mem_to_phys(bus_dma_tag_t, bus_addr_t);


struct powerpc_bus_dma_tag booke_bus_dma_tag = {
	._dmamap_create = _bus_dmamap_create,
	._dmamap_destroy = _bus_dmamap_destroy,
	._dmamap_load = _bus_dmamap_load,
	._dmamap_load_mbuf = _bus_dmamap_load_mbuf,
	._dmamap_load_uio = _bus_dmamap_load_uio,
	._dmamap_load_raw = _bus_dmamap_load_raw,
	._dmamap_unload = _bus_dmamap_unload,
	/*
	 * The caches on BookE are coherent so we don't need to do any special
	 * cache synchronization.
	 */
	//._dmamap_sync = _bus_dmamap_sync,
	._dmamem_alloc = _bus_dmamem_alloc,
	._dmamem_free = _bus_dmamem_free,
	._dmamem_map = _bus_dmamem_map,
	._dmamem_unmap = _bus_dmamem_unmap,
	._dmamem_mmap = _bus_dmamem_mmap,
	._dma_phys_to_bus_mem = booke_dma_phys_to_bus_mem,
	._dma_bus_mem_to_phys = booke_dma_bus_mem_to_phys,
};

static bus_addr_t
booke_dma_phys_to_bus_mem(bus_dma_tag_t t, bus_addr_t a)
{
	return a;
}

static bus_addr_t
booke_dma_bus_mem_to_phys(bus_dma_tag_t t, bus_addr_t a)
{
	return a;
}

struct cpu_md_ops cpu_md_ops;

struct cpu_softc cpu_softc[] = {
	[0] = {
		.cpu_ci = &cpu_info[0],
	},
#ifdef MULTIPROCESSOR
	[CPU_MAXNUM-1] = {
		.cpu_ci = &cpu_info[CPU_MAXNUM-1],
	},
#endif
};
struct cpu_info cpu_info[] = {
	[0] = {
		.ci_curlwp = &lwp0,
		.ci_tlb_info = &pmap_tlb0_info,
		.ci_softc = &cpu_softc[0],
		.ci_cpl = IPL_HIGH,
		.ci_idepth = -1,
	},
#ifdef MULTIPROCESSOR
	[CPU_MAXNUM-1] = {
		.ci_curlwp = NULL,
		.ci_tlb_info = &pmap_tlb0_info,
		.ci_softc = &cpu_softc[CPU_MAXNUM-1],
		.ci_cpl = IPL_HIGH,
		.ci_idepth = -1,
	},
#endif
};
__CTASSERT(__arraycount(cpu_info) == __arraycount(cpu_softc));

/*
 * This should probably be in autoconf!				XXX
 */
char machine[] = MACHINE;		/* from <machine/param.h> */
char machine_arch[] = MACHINE_ARCH;	/* from <machine/param.h> */

char bootpath[256];

#if NKSYMS || defined(DDB) || defined(MODULAR)
void *startsym, *endsym;
#endif

#if defined(MULTIPROCESSOR)
volatile struct cpu_hatch_data cpu_hatch_data __cacheline_aligned;
#endif

int fake_mapiodev = 1;

void
booke_cpu_startup(const char *model)
{
	vaddr_t 	minaddr, maxaddr;
	char 		pbuf[9];

	cpu_setmodel("%s", model);

	printf("%s%s", copyright, version);

	format_bytes(pbuf, sizeof(pbuf), ctob((uint64_t)physmem));
	printf("total memory = %s\n", pbuf);

	minaddr = 0;
	/*
	 * Allocate a submap for physio
	 */
	phys_map = uvm_km_suballoc(kernel_map, &minaddr, &maxaddr,
				 VM_PHYS_SIZE, 0, false, NULL);

	/*
	 * No need to allocate an mbuf cluster submap.  Mbuf clusters
	 * are allocated via the pool allocator, and we use direct-mapped
	 * pool pages.
	 */

	format_bytes(pbuf, sizeof(pbuf), ptoa(uvmexp.free));
	printf("avail memory = %s\n", pbuf);

	/*
	 * Register the tlb's evcnts
	 */
	pmap_tlb_info_evcnt_attach(curcpu()->ci_tlb_info);

	/*
	 * Set up the board properties database.
	 */
	board_info_init();

	/*
	 * Now that we have VM, malloc()s are OK in bus_space.
	 */
	bus_space_mallocok();
	fake_mapiodev = 0;

#ifdef MULTIPROCESSOR
	for (size_t i = 1; i < __arraycount(cpu_info); i++) {
		struct cpu_info * const ci = &cpu_info[i];
		struct cpu_softc * const cpu = &cpu_softc[i];
		cpu->cpu_ci = ci;
		cpu->cpu_bst = cpu_softc[0].cpu_bst;
		cpu->cpu_le_bst = cpu_softc[0].cpu_le_bst;
		cpu->cpu_bsh = cpu_softc[0].cpu_bsh;
		cpu->cpu_highmem = cpu_softc[0].cpu_highmem;
		ci->ci_softc = cpu;
		ci->ci_tlb_info = &pmap_tlb0_info;
		ci->ci_cpl = IPL_HIGH;
		ci->ci_idepth = -1;
		ci->ci_pmap_kern_segtab = curcpu()->ci_pmap_kern_segtab;
	}

	kcpuset_create(&cpuset_info.cpus_running, true);
	kcpuset_create(&cpuset_info.cpus_hatched, true);
	kcpuset_create(&cpuset_info.cpus_paused, true);
	kcpuset_create(&cpuset_info.cpus_resumed, true);
	kcpuset_create(&cpuset_info.cpus_halted, true);
#endif /* MULTIPROCESSOR */
}

static void
dumpsys(void)
{

	printf("dumpsys: TBD\n");
}

/*
 * Halt or reboot the machine after syncing/dumping according to howto.
 */
void
cpu_reboot(int howto, char *what)
{
	static int syncing;
	static char str[256];
	char *ap = str, *ap1 = ap;

	boothowto = howto;
	if (!cold && !(howto & RB_NOSYNC) && !syncing) {
		syncing = 1;
		vfs_shutdown();		/* sync */
		resettodr();		/* set wall clock */
	}

	splhigh();

	if (!cold && (howto & RB_DUMP))
		dumpsys();

	doshutdownhooks();

	pmf_system_shutdown(boothowto);

	if ((howto & RB_POWERDOWN) == RB_POWERDOWN) {
	  /* Power off here if we know how...*/
	}

	if (howto & RB_HALT) {
		printf("halted\n\n");

		goto reboot;	/* XXX for now... */

#ifdef DDB
		printf("dropping to debugger\n");
		while(1)
			Debugger();
#endif
	}

	printf("rebooting\n\n");
	if (what && *what) {
		if (strlen(what) > sizeof str - 5)
			printf("boot string too large, ignored\n");
		else {
			strcpy(str, what);
			ap1 = ap = str + strlen(str);
			*ap++ = ' ';
		}
	}
	*ap++ = '-';
	if (howto & RB_SINGLE)
		*ap++ = 's';
	if (howto & RB_KDB)
		*ap++ = 'd';
	*ap++ = 0;
	if (ap[-2] == '-')
		*ap1 = 0;

	/* flush cache for msgbuf */
	dcache_wb(msgbuf_paddr, round_page(MSGBUFSIZE));

 reboot:
	__asm volatile("msync; isync");
	(*cpu_md_ops.md_cpu_reset)();

	printf("%s: md_cpu_reset() failed!\n", __func__);
#ifdef DDB
	for (;;)
		Debugger();
#else
	for (;;)
		/* nothing */;
#endif
}

/*
 * mapiodev:
 *
 * 	Allocate vm space and mapin the I/O address. Use reserved TLB
 * 	mapping if one is found.
 */
void *
mapiodev(paddr_t pa, psize_t len, bool prefetchable)
{
	const vsize_t off = pa & PAGE_MASK;

	/*
	 * See if we have reserved TLB entry for the pa. This needs to be
	 * true for console as we can't use uvm during early bootstrap.
	 */
	void * const p = tlb_mapiodev(pa, len, prefetchable);
	if (p != NULL)
		return p;

	if (fake_mapiodev)
		panic("mapiodev: no TLB entry reserved for %llx+%llx",
		    (long long)pa, (long long)len);

	const paddr_t orig_pa = pa;
	const psize_t orig_len = len;
	vsize_t align = 0;
	pa = trunc_page(pa);
	len = round_page(off + len);
	/*
	 * If we are allocating a large amount (>= 1MB) try to get an
	 * aligned VA region for it so try to do a large mapping for it.
	 */
	if ((len & (len - 1)) == 0 && len >= 0x100000)
		align = len;

	vaddr_t va = uvm_km_alloc(kernel_map, len, align, UVM_KMF_VAONLY);

	if (va == 0 && align > 0) {
		/*
		 * Large aligned request failed.  Let's just get anything.
		 */
		align = 0;
		va = uvm_km_alloc(kernel_map, len, align, UVM_KMF_VAONLY);
	}
	if (va == 0)
		return NULL;

	if (align) {
		/*
		 * Now try to map that via one big TLB entry.
		 */
		pt_entry_t pte = pte_make_kenter_pa(pa, NULL,
		    VM_PROT_READ|VM_PROT_WRITE,
		    prefetchable ? 0 : PMAP_NOCACHE);
		if (!tlb_ioreserve(va, len, pte)) {
			void * const p0 = tlb_mapiodev(orig_pa, orig_len,
			    prefetchable);
			KASSERT(p0 != NULL);
			return p0;
		}
	}

	for (va += len, pa += len; len > 0; len -= PAGE_SIZE) {
		va -= PAGE_SIZE;
		pa -= PAGE_SIZE;
		pmap_kenter_pa(va, pa, VM_PROT_READ|VM_PROT_WRITE,
		    prefetchable ? 0 : PMAP_NOCACHE);
	}
	pmap_update(pmap_kernel());
	return (void *)(va + off);
}

void
unmapiodev(vaddr_t va, vsize_t len)
{
	/* Nothing to do for reserved (ie. not uvm_km_alloc'd) mappings. */
	if (va < VM_MIN_KERNEL_ADDRESS || va > VM_MAX_KERNEL_ADDRESS) {
		tlb_unmapiodev(va, len);
		return;
	}

	len = round_page((va & PAGE_MASK) + len);
	va = trunc_page(va);

	pmap_kremove(va, len);
	uvm_km_free(kernel_map, va, len, UVM_KMF_VAONLY);
}

void
cpu_evcnt_attach(struct cpu_info *ci)
{
	struct cpu_softc * const cpu = ci->ci_softc;
	const char * const xname = ci->ci_data.cpu_name;

	evcnt_attach_dynamic_nozero(&ci->ci_ev_clock, EVCNT_TYPE_INTR,
		NULL, xname, "clock");
	evcnt_attach_dynamic_nozero(&cpu->cpu_ev_late_clock, EVCNT_TYPE_INTR,
		NULL, xname, "late clock");
	evcnt_attach_dynamic_nozero(&cpu->cpu_ev_exec_trap_sync, EVCNT_TYPE_TRAP,
		NULL, xname, "exec pages synced (trap)");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_traps, EVCNT_TYPE_TRAP,
		NULL, xname, "traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_kdsi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "kernel DSI traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_udsi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "user DSI traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_udsi_fatal, EVCNT_TYPE_TRAP,
		&ci->ci_ev_udsi, xname, "user DSI failures");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_kisi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "kernel ISI traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_isi, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "user ISI traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_isi_fatal, EVCNT_TYPE_TRAP,
		&ci->ci_ev_isi, xname, "user ISI failures");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_scalls, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "system call traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_pgm, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "PGM traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_debug, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "debug traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_fpu, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "FPU unavailable traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_fpusw, EVCNT_TYPE_MISC,
		&ci->ci_ev_fpu, xname, "FPU context switches");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_ali, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "user alignment traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_ali_fatal, EVCNT_TYPE_TRAP,
		&ci->ci_ev_ali, xname, "user alignment traps");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_umchk, EVCNT_TYPE_TRAP,
		&ci->ci_ev_umchk, xname, "user MCHK failures");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_vec, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "SPE unavailable");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_vecsw, EVCNT_TYPE_MISC,
	    &ci->ci_ev_vec, xname, "SPE context switches");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_ipi, EVCNT_TYPE_INTR,
		NULL, xname, "IPIs");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_tlbmiss_soft, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "soft tlb misses");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_dtlbmiss_hard, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "data tlb misses");
	evcnt_attach_dynamic_nozero(&ci->ci_ev_itlbmiss_hard, EVCNT_TYPE_TRAP,
		&ci->ci_ev_traps, xname, "inst tlb misses");
}

#ifdef MULTIPROCESSOR
register_t
cpu_hatch(void)
{
	struct cpuset_info * const csi = &cpuset_info;
	const size_t id = cpu_number();

	/*
	 * We've hatched so tell the spinup code.
	 */
	kcpuset_set(csi->cpus_hatched, id);

	/*
	 * Loop until running bit for this cpu is set.
	 */
	while (!kcpuset_isset(csi->cpus_running, id)) {
		continue;
	}

	/*
	 * Now that we are active, start the clocks.
	 */
	cpu_initclocks();

	/*
	 * Return sp of the idlelwp.  Which we should be already using but ...
	 */
	return curcpu()->ci_curpcb->pcb_sp;
}

void
cpu_boot_secondary_processors(void)
{
	volatile struct cpuset_info * const csi = &cpuset_info;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	kcpuset_t *running;

	kcpuset_create(&running, true);

	for (CPU_INFO_FOREACH(cii, ci)) {
		/*
		 * Skip this CPU if it didn't sucessfully hatch.
		 */
		if (!kcpuset_isset(csi->cpus_hatched, cpu_index(ci)))
			continue;

		KASSERT(!CPU_IS_PRIMARY(ci));
		KASSERT(ci->ci_data.cpu_idlelwp);

		kcpuset_set(running, cpu_index(ci));
	}
	KASSERT(kcpuset_match(csi->cpus_hatched, running));
	if (!kcpuset_iszero(running)) {
		kcpuset_merge(csi->cpus_running, running);
	}
	kcpuset_destroy(running);
}
#endif

uint32_t
cpu_read_4(bus_addr_t a)
{
	struct cpu_softc * const cpu = curcpu()->ci_softc;
//	printf(" %s(%p, %x, %x)", __func__, cpu->cpu_bst, cpu->cpu_bsh, a);
	return bus_space_read_4(cpu->cpu_bst, cpu->cpu_bsh, a);
}

uint8_t
cpu_read_1(bus_addr_t a)
{
	struct cpu_softc * const cpu = curcpu()->ci_softc;
//	printf(" %s(%p, %x, %x)", __func__, cpu->cpu_bst, cpu->cpu_bsh, a);
	return bus_space_read_1(cpu->cpu_bst, cpu->cpu_bsh, a);
}

void
cpu_write_4(bus_addr_t a, uint32_t v)
{
	struct cpu_softc * const cpu = curcpu()->ci_softc;
	bus_space_write_4(cpu->cpu_bst, cpu->cpu_bsh, a, v);
}

void
cpu_write_1(bus_addr_t a, uint8_t v)
{
	struct cpu_softc * const cpu = curcpu()->ci_softc;
	bus_space_write_1(cpu->cpu_bst, cpu->cpu_bsh, a, v);
}

void
booke_sstep(struct trapframe *tf)
{
	KASSERT(tf->tf_srr1 & PSL_DE);
	const uint32_t insn = ufetch_32((const void *)tf->tf_srr0);
	register_t dbcr0 = DBCR0_IAC1 | DBCR0_IDM;
	register_t dbcr1 = DBCR1_IAC1US_USER | DBCR1_IAC1ER_DS1;
	if ((insn >> 28) == 4) {
		uint32_t iac2 = 0;
		if ((insn >> 26) == 0x12) {
			const int32_t off = (((int32_t)insn << 6) >> 6) & ~3;
			iac2 = ((insn & 2) ? 0 : tf->tf_srr0) + off;
			dbcr0 |= DBCR0_IAC2;
		} else if ((insn >> 26) == 0x10) {
			const int16_t off = insn & ~3;
			iac2 = ((insn & 2) ? 0 : tf->tf_srr0) + off;
			dbcr0 |= DBCR0_IAC2;
		} else if ((insn & 0xfc00fffe) == 0x4c000420) {
			iac2 = tf->tf_ctr;
			dbcr0 |= DBCR0_IAC2;
		} else if ((insn & 0xfc00fffe) == 0x4c000020) {
			iac2 = tf->tf_lr;
			dbcr0 |= DBCR0_IAC2;
		}
		if (dbcr0 & DBCR0_IAC2) {
			dbcr1 |= DBCR1_IAC2US_USER | DBCR1_IAC2ER_DS1;
			mtspr(SPR_IAC2, iac2);
		}
	}
	mtspr(SPR_IAC1, tf->tf_srr0 + 4);
	mtspr(SPR_DBCR1, dbcr1);
	mtspr(SPR_DBCR0, dbcr0);
}

#ifdef DIAGNOSTIC
static inline void
swap_data(uint64_t *data, size_t a, size_t b)
{
	uint64_t swap = data[a];
	data[a] = data[b];
	data[b] = swap;
}

static void
sort_data(uint64_t *data, size_t count)
{
#if 0
	/*
	 * Mostly classic bubble sort
	 */
	do {
		size_t new_count = 0;
		for (size_t i = 1; i < count; i++) {
			if (tbs[i - 1] > tbs[i]) {
				swap_tbs(tbs, i - 1, i);
				new_count = i;
			}
		}
		count = new_count;
	} while (count > 0);
#else
	/*
	 * Comb sort
	 */
	size_t gap = count;
	bool swapped = false;
	while (gap > 1 || swapped) {
		if (gap > 1) {
			/*
			 * phi = (1 + sqrt(5)) / 2 [golden ratio]
			 * N = 1 / (1 - e^-phi)) = 1.247330950103979
			 *
			 * We want to but can't use floating point to calculate
			 *	gap = (size_t)((double)gap / N)
			 *
			 * So we will use the multicative inverse of N
			 * (module 65536) to achieve the division.
			 *
			 * iN = 2^16 / 1.24733... = 52540
			 * x / N == (x * iN) / 65536 
			 */
			gap = (gap * 52540) / 65536;
		}

		swapped = false;

		for (size_t i = 0; gap + i < count; i++) {
			if (data[i] > data[i + gap]) {
				swap_data(data, i, i + gap);
				swapped = true;
			}
		}
	}
#endif
}
#endif

void
dump_splhist(struct cpu_info *ci, void (*pr)(const char *, ...))
{
#ifdef DIAGNOSTIC
	struct cpu_softc * const cpu = ci->ci_softc;
	uint64_t tbs[NIPL*NIPL];
	size_t ntbs = 0;
	for (size_t to = 0; to < NIPL; to++) {
		for (size_t from = 0; from < NIPL; from++) {
			uint64_t tb = cpu->cpu_spl_tb[to][from];
			if (tb == 0)
				continue;
			tbs[ntbs++] = (tb << 8) | (to << 4) | from;
		}
	}
	sort_data(tbs, ntbs);

	if (pr == NULL)
		pr = printf;
	uint64_t last_tb = 0;
	for (size_t i = 0; i < ntbs; i++) {
		uint64_t tb = tbs[i];
		size_t from = tb & 15;
		size_t to = (tb >> 4) & 15;
		tb >>= 8;
		(*pr)("%s(%zu) from %zu at %"PRId64"",
		     from < to ? "splraise" : "splx",
		     to, from, tb);
		if (last_tb && from != IPL_NONE)
			(*pr)(" (+%"PRId64")", tb - last_tb);
		(*pr)("\n");
		last_tb = tb;
	}
#endif
}
