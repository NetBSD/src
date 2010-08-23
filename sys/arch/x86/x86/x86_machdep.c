/*	$NetBSD: x86_machdep.c,v 1.43 2010/08/23 16:20:45 jruoho Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007 YAMAMOTO Takashi,
 * Copyright (c) 2005, 2008, 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julio M. Merino Vidal.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: x86_machdep.c,v 1.43 2010/08/23 16:20:45 jruoho Exp $");

#include "opt_modular.h"
#include "opt_physmem.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kcore.h>
#include <sys/errno.h>
#include <sys/kauth.h>
#include <sys/mutex.h>
#include <sys/cpu.h>
#include <sys/intr.h>
#include <sys/atomic.h>
#include <sys/module.h>
#include <sys/sysctl.h>
#include <sys/extent.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/machdep.h>
#include <x86/pio.h>

#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>

#include <machine/bootinfo.h>
#include <machine/vmparam.h>

#include <uvm/uvm_extern.h>

void (*x86_cpu_idle)(void);
static bool x86_cpu_idle_ipi;
static char x86_cpu_idle_text[16];

int check_pa_acc(paddr_t, vm_prot_t);

/* --------------------------------------------------------------------- */

/*
 * Main bootinfo structure.  This is filled in by the bootstrap process
 * done in locore.S based on the information passed by the boot loader.
 */
struct bootinfo bootinfo;

/* --------------------------------------------------------------------- */

static kauth_listener_t x86_listener;

/*
 * Given the type of a bootinfo entry, looks for a matching item inside
 * the bootinfo structure.  If found, returns a pointer to it (which must
 * then be casted to the appropriate bootinfo_* type); otherwise, returns
 * NULL.
 */
void *
lookup_bootinfo(int type)
{
	bool found;
	int i;
	struct btinfo_common *bic;

	bic = (struct btinfo_common *)(bootinfo.bi_data);
	found = FALSE;
	for (i = 0; i < bootinfo.bi_nentries && !found; i++) {
		if (bic->type == type)
			found = TRUE;
		else
			bic = (struct btinfo_common *)
			    ((uint8_t *)bic + bic->len);
	}

	return found ? bic : NULL;
}

/*
 * check_pa_acc: check if given pa is accessible.
 */
int
check_pa_acc(paddr_t pa, vm_prot_t prot)
{
	extern phys_ram_seg_t mem_clusters[VM_PHYSSEG_MAX];
	extern int mem_cluster_cnt;
	int i;

	for (i = 0; i < mem_cluster_cnt; i++) {
		const phys_ram_seg_t *seg = &mem_clusters[i];
		paddr_t lstart = seg->start;

		if (lstart <= pa && pa - lstart <= seg->size) {
			return 0;
		}
	}

	return kauth_authorize_machdep(kauth_cred_get(),
	    KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL);
}

#ifdef MODULAR
/*
 * Push any modules loaded by the boot loader.
 */
void
module_init_md(void)
{
	struct btinfo_modulelist *biml;
	struct bi_modulelist_entry *bi, *bimax;

	biml = lookup_bootinfo(BTINFO_MODULELIST);
	if (biml == NULL) {
		aprint_debug("No module info at boot\n");
		return;
	}

	bi = (struct bi_modulelist_entry *)((uint8_t *)biml + sizeof(*biml));
	bimax = bi + biml->num;
	for (; bi < bimax; bi++) {
		if (bi->type != BI_MODULE_ELF) {
			aprint_debug("Skipping non-ELF module\n");
			continue;
		}
		aprint_debug("Prep module path=%s len=%d pa=%x\n", bi->path,
		    bi->len, bi->base);
		KASSERT(trunc_page(bi->base) == bi->base);
		(void)module_prime((void *)((uintptr_t)bi->base + KERNBASE),
		    bi->len);
	}
}
#endif	/* MODULAR */

void
cpu_need_resched(struct cpu_info *ci, int flags)
{
	struct cpu_info *cur;
	lwp_t *l;

	KASSERT(kpreempt_disabled());
	cur = curcpu();
	l = ci->ci_data.cpu_onproc;
	ci->ci_want_resched |= flags;

	if (__predict_false((l->l_pflag & LP_INTR) != 0)) {
		/*
		 * No point doing anything, it will switch soon.
		 * Also here to prevent an assertion failure in
		 * kpreempt() due to preemption being set on a
		 * soft interrupt LWP.
		 */
		return;
	}

	if (l == ci->ci_data.cpu_idlelwp) {
		if (ci == cur)
			return;
#ifndef XEN /* XXX review when Xen gets MP support */
		if (x86_cpu_idle_ipi != false)
			x86_send_ipi(ci, 0);
#endif
		return;
	}

	if ((flags & RESCHED_KPREEMPT) != 0) {
#ifdef __HAVE_PREEMPTION
		atomic_or_uint(&l->l_dopreempt, DOPREEMPT_ACTIVE);
		if (ci == cur) {
			softint_trigger(1 << SIR_PREEMPT);
		} else {
			x86_send_ipi(ci, X86_IPI_KPREEMPT);
		}
#endif
	} else {
		aston(l, X86_AST_PREEMPT);
		if (ci == cur) {
			return;
		}
		if ((flags & RESCHED_IMMED) != 0) {
			x86_send_ipi(ci, 0);
		}
	}
}

void
cpu_signotify(struct lwp *l)
{

	KASSERT(kpreempt_disabled());
	aston(l, X86_AST_GENERIC);
	if (l->l_cpu != curcpu())
		x86_send_ipi(l->l_cpu, 0);
}

void
cpu_need_proftick(struct lwp *l)
{

	KASSERT(kpreempt_disabled());
	KASSERT(l->l_cpu == curcpu());

	l->l_pflag |= LP_OWEUPC;
	aston(l, X86_AST_GENERIC);
}

bool
cpu_intr_p(void)
{
	int idepth;

	kpreempt_disable();
	idepth = curcpu()->ci_idepth;
	kpreempt_enable();
	return (idepth >= 0);
}

#ifdef __HAVE_PREEMPTION
/*
 * Called to check MD conditions that would prevent preemption, and to
 * arrange for those conditions to be rechecked later.
 */
bool
cpu_kpreempt_enter(uintptr_t where, int s)
{
	struct cpu_info *ci;
	struct pcb *pcb;
	lwp_t *l;

	KASSERT(kpreempt_disabled());

	l = curlwp;
	ci = curcpu();

	/*
	 * If SPL raised, can't go.  Note this implies that spin
	 * mutexes at IPL_NONE are _not_ valid to use.
	 */
	if (s > IPL_PREEMPT) {
		softint_trigger(1 << SIR_PREEMPT);
		aston(l, X86_AST_PREEMPT);	/* paranoid */
		return false;
	}

	/* Must save cr2 or it could be clobbered. */
	pcb = lwp_getpcb(l);
	pcb->pcb_cr2 = rcr2();

	return true;
}

/*
 * Called after returning from a kernel preemption, and called with
 * preemption disabled.
 */
void
cpu_kpreempt_exit(uintptr_t where)
{
	extern char x86_copyfunc_start, x86_copyfunc_end;
	struct pcb *pcb;

	KASSERT(kpreempt_disabled());

	/*
	 * If we interrupted any of the copy functions we must reload
	 * the pmap when resuming, as they cannot tolerate it being
	 * swapped out.
	 */
	if (where >= (uintptr_t)&x86_copyfunc_start &&
	    where < (uintptr_t)&x86_copyfunc_end) {
		pmap_load();
	}

	/* Restore cr2 only after the pmap, as pmap_load can block. */
	pcb = lwp_getpcb(curlwp);
	lcr2(pcb->pcb_cr2);
}

/*
 * Return true if preemption is disabled for MD reasons.  Must be called
 * with preemption disabled, and thus is only for diagnostic checks.
 */
bool
cpu_kpreempt_disabled(void)
{

	return curcpu()->ci_ilevel > IPL_NONE;
}
#endif	/* __HAVE_PREEMPTION */

SYSCTL_SETUP(sysctl_machdep_cpu_idle, "sysctl machdep cpu_idle")
{
	const struct sysctlnode	*mnode, *node;

	sysctl_createv(NULL, 0, NULL, &mnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	sysctl_createv(NULL, 0, &mnode, &node,
		       CTLFLAG_PERMANENT, CTLTYPE_STRING, "idle-mechanism",
		       SYSCTL_DESCR("Mechanism used for the idle loop."),
		       NULL, 0, x86_cpu_idle_text, 0,
		       CTL_CREATE, CTL_EOL);
}

void
x86_cpu_idle_init(void)
{

#ifndef XEN
	if ((cpu_feature[1] & CPUID2_MONITOR) == 0 ||
	    cpu_vendor == CPUVENDOR_AMD)
		x86_cpu_idle_set(x86_cpu_idle_halt, "halt", true);
	else
		x86_cpu_idle_set(x86_cpu_idle_mwait, "mwait", false);
#else
	x86_cpu_idle_set(x86_cpu_idle_xen, "xen", false);
#endif
}

void
x86_cpu_idle_get(void (**func)(void), char *text, size_t len)
{

	*func = x86_cpu_idle;

	(void)strlcpy(text, x86_cpu_idle_text, len);
}

void
x86_cpu_idle_set(void (*func)(void), const char *text, bool ipi)
{

	x86_cpu_idle = func;
	x86_cpu_idle_ipi = ipi;

	(void)strlcpy(x86_cpu_idle_text, text, sizeof(x86_cpu_idle_text));
}

#ifndef XEN

#define KBTOB(x)	((size_t)(x) * 1024UL)
#define MBTOB(x)	((size_t)(x) * 1024UL * 1024UL)

extern paddr_t avail_start, avail_end;

static int
add_mem_cluster(phys_ram_seg_t *seg_clusters, int seg_cluster_cnt,
	struct extent *iomem_ex,
	uint64_t seg_start, uint64_t seg_end, uint32_t type)
{
	uint64_t new_physmem = 0;
	phys_ram_seg_t *cluster;
	int i;

#ifdef i386
#ifdef PAE
#define TOPLIMIT	0x1000000000ULL	/* 64GB */
#else
#define TOPLIMIT	0x100000000ULL	/* 4GB */
#endif
#else
#define TOPLIMIT	0x100000000000ULL /* 16TB */
#endif

	if (seg_end > TOPLIMIT) {
		aprint_verbose("WARNING: skipping large memory map entry: "
		    "0x%"PRIx64"/0x%"PRIx64"/0x%x\n",
		    seg_start,
		    (seg_end - seg_start),
		    type);
		return seg_cluster_cnt;
	}

	/*
	 * XXX Chop the last page off the size so that
	 * XXX it can fit in avail_end.
	 */
	if (seg_end == TOPLIMIT)
		seg_end -= PAGE_SIZE;

	if (seg_end <= seg_start)
		return seg_cluster_cnt;

	for (i = 0; i < seg_cluster_cnt; i++) {
		cluster = &seg_clusters[i];
		if ((cluster->start == round_page(seg_start))
		    && (cluster->size == trunc_page(seg_end) - cluster->start))
		{
#ifdef DEBUG_MEMLOAD
			printf("WARNING: skipping duplicate segment entry\n");
#endif
			return seg_cluster_cnt;
		}
	}

	/*
	 * Allocate the physical addresses used by RAM
	 * from the iomem extent map.  This is done before
	 * the addresses are page rounded just to make
	 * sure we get them all.
	 */
	if (seg_start < 0x100000000ULL) {
		uint64_t io_end;

		if (seg_end > 0x100000000ULL)
			io_end = 0x100000000ULL;
		else
			io_end = seg_end;

		if (iomem_ex != NULL && extent_alloc_region(iomem_ex, seg_start,
		    io_end - seg_start, EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN't ALLOCATE MEMORY SEGMENT "
			    "(0x%"PRIx64"/0x%"PRIx64"/0x%x) FROM "
			    "IOMEM EXTENT MAP!\n",
			    seg_start, seg_end - seg_start, type);
			return seg_cluster_cnt;
		}
	}

	/*
	 * If it's not free memory, skip it.
	 */
	if (type != BIM_Memory)
		return seg_cluster_cnt;

	/* XXX XXX XXX */
	if (seg_cluster_cnt >= VM_PHYSSEG_MAX)
		panic("%s: too many memory segments (increase VM_PHYSSEG_MAX)",
			__func__);

#ifdef PHYSMEM_MAX_ADDR
	if (seg_start >= MBTOB(PHYSMEM_MAX_ADDR))
		return seg_cluster_cnt;
	if (seg_end > MBTOB(PHYSMEM_MAX_ADDR))
		seg_end = MBTOB(PHYSMEM_MAX_ADDR);
#endif  

	seg_start = round_page(seg_start);
	seg_end = trunc_page(seg_end);

	if (seg_start == seg_end)
		return seg_cluster_cnt;

	cluster = &seg_clusters[seg_cluster_cnt];
	cluster->start = seg_start;
	if (iomem_ex != NULL)
		new_physmem = physmem + atop(seg_end - seg_start);

#ifdef PHYSMEM_MAX_SIZE
	if (iomem_ex != NULL) {
		if (physmem >= atop(MBTOB(PHYSMEM_MAX_SIZE)))
			return seg_cluster_cnt;
		if (new_physmem > atop(MBTOB(PHYSMEM_MAX_SIZE))) {
			seg_end = seg_start + MBTOB(PHYSMEM_MAX_SIZE) - ptoa(physmem);
			new_physmem = atop(MBTOB(PHYSMEM_MAX_SIZE));
		}
	}
#endif  

	cluster->size = seg_end - seg_start;

	if (iomem_ex != NULL) {
		if (avail_end < seg_end)
			avail_end = seg_end;
		physmem = new_physmem;
	}
	seg_cluster_cnt++;

	return seg_cluster_cnt;
}

int
initx86_parse_memmap(struct btinfo_memmap *bim, struct extent *iomem_ex)
{
	uint64_t seg_start, seg_end;
	uint64_t addr, size;
	uint32_t type;
	int x;

	KASSERT(bim != NULL);
	KASSERT(bim->num > 0);

#ifdef DEBUG_MEMLOAD
	printf("BIOS MEMORY MAP (%d ENTRIES):\n", bim->num);
#endif
	for (x = 0; x < bim->num; x++) {
		addr = bim->entry[x].addr;
		size = bim->entry[x].size;
		type = bim->entry[x].type;
#ifdef DEBUG_MEMLOAD
		printf("    addr 0x%"PRIx64"  size 0x%"PRIx64"  type 0x%x\n",
			addr, size, type);
#endif

		/*
		 * If the segment is not memory, skip it.
		 */
		switch (type) {
		case BIM_Memory:
		case BIM_ACPI:
		case BIM_NVS:
			break;
		default:
			continue;
		}

		/*
		 * If the segment is smaller than a page, skip it.
		 */
		if (size < NBPG)
			continue;

		seg_start = addr;
		seg_end = addr + size;

		/*
		 *   Avoid Compatibility Holes.
		 * XXX  Holes within memory space that allow access
		 * XXX to be directed to the PC-compatible frame buffer
		 * XXX (0xa0000-0xbffff), to adapter ROM space
		 * XXX (0xc0000-0xdffff), and to system BIOS space
		 * XXX (0xe0000-0xfffff).
		 * XXX  Some laptop(for example,Toshiba Satellite2550X)
		 * XXX report this area and occurred problems,
		 * XXX so we avoid this area.
		 */
		if (seg_start < 0x100000 && seg_end > 0xa0000) {
			printf("WARNING: memory map entry overlaps "
			    "with ``Compatibility Holes'': "
			    "0x%"PRIx64"/0x%"PRIx64"/0x%x\n", seg_start,
			    seg_end - seg_start, type);
			mem_cluster_cnt = add_mem_cluster(
				mem_clusters, mem_cluster_cnt, iomem_ex,
				seg_start, 0xa0000, type);
			mem_cluster_cnt = add_mem_cluster(
				mem_clusters, mem_cluster_cnt, iomem_ex,
				0x100000, seg_end, type);
		} else
			mem_cluster_cnt = add_mem_cluster(
				mem_clusters, mem_cluster_cnt, iomem_ex,
				seg_start, seg_end, type);
	}

	return 0;
}

int
initx86_fake_memmap(struct extent *iomem_ex)
{
	phys_ram_seg_t *cluster;
	KASSERT(mem_cluster_cnt == 0);

	/*
	 * Allocate the physical addresses used by RAM from the iomem
	 * extent map.  This is done before the addresses are
	 * page rounded just to make sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, 0, KBTOB(biosbasemem),
	    EX_NOWAIT))
	{
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM "
		    "IOMEM EXTENT MAP!\n");
	}

	cluster = &mem_clusters[0];
	cluster->start = 0;
	cluster->size = trunc_page(KBTOB(biosbasemem));
	physmem += atop(cluster->size);

	if (extent_alloc_region(iomem_ex, IOM_END, KBTOB(biosextmem),
	    EX_NOWAIT))
	{
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE EXTENDED MEMORY FROM "
		    "IOMEM EXTENT MAP!\n");
	}

#if NISADMA > 0 
	/*
	 * Some motherboards/BIOSes remap the 384K of RAM that would
	 * normally be covered by the ISA hole to the end of memory
	 * so that it can be used.  However, on a 16M system, this
	 * would cause bounce buffers to be allocated and used.
	 * This is not desirable behaviour, as more than 384K of
	 * bounce buffers might be allocated.  As a work-around,
	 * we round memory down to the nearest 1M boundary if
	 * we're using any isadma devices and the remapped memory
	 * is what puts us over 16M.
	 */
	if (biosextmem > (15*1024) && biosextmem < (16*1024)) {
		char pbuf[9];

		format_bytes(pbuf, sizeof(pbuf),
		    biosextmem - (15*1024));
		printf("Warning: ignoring %s of remapped memory\n",
		    pbuf);
		biosextmem = (15*1024);
	}
#endif
	cluster = &mem_clusters[1];
	cluster->start = IOM_END;
	cluster->size = trunc_page(KBTOB(biosextmem));
	physmem += atop(cluster->size);

	mem_cluster_cnt = 2;

	avail_end = IOM_END + trunc_page(KBTOB(biosextmem));

	return 0;
}

#ifdef amd64
extern vaddr_t kern_end;
extern vaddr_t module_start, module_end;
#endif

int
initx86_load_memmap(paddr_t first_avail)
{
	uint64_t seg_start, seg_end;
	uint64_t seg_start1, seg_end1;
	int first16q, x;
#ifdef VM_FREELIST_FIRST4G
	int first4gq;
#endif

	/*
	 * If we have 16M of RAM or less, just put it all on
	 * the default free list.  Otherwise, put the first
	 * 16M of RAM on a lower priority free list (so that
	 * all of the ISA DMA'able memory won't be eaten up
	 * first-off).
	 */
#define ADDR_16M (16 * 1024 * 1024)

	if (avail_end <= ADDR_16M)
		first16q = VM_FREELIST_DEFAULT;
	else
		first16q = VM_FREELIST_FIRST16;

#ifdef VM_FREELIST_FIRST4G
	/*
	 * If we have 4G of RAM or less, just put it all on
	 * the default free list.  Otherwise, put the first
	 * 4G of RAM on a lower priority free list (so that
	 * all of the 32bit PCI DMA'able memory won't be eaten up
	 * first-off).
	 */
#define ADDR_4G (4ULL * 1024 * 1024 * 1024)
	if (avail_end <= ADDR_4G)
		first4gq = VM_FREELIST_DEFAULT;
	else
		first4gq = VM_FREELIST_FIRST4G;
#endif /* defined(VM_FREELIST_FIRST4G) */

	/* Make sure the end of the space used by the kernel is rounded. */
	first_avail = round_page(first_avail);

#ifdef amd64
	kern_end = KERNBASE + first_avail;
	module_start = kern_end;
	module_end = KERNBASE + NKL2_KIMG_ENTRIES * NBPD_L2;
#endif

	/*
	 * Now, load the memory clusters (which have already been
	 * rounded and truncated) into the VM system.
	 *
	 * NOTE: WE ASSUME THAT MEMORY STARTS AT 0 AND THAT THE KERNEL
	 * IS LOADED AT IOM_END (1M).
	 */
	for (x = 0; x < mem_cluster_cnt; x++) {
		const phys_ram_seg_t *cluster = &mem_clusters[x];

		seg_start = cluster->start;
		seg_end = cluster->start + cluster->size;
		seg_start1 = 0;
		seg_end1 = 0;

		/*
		 * Skip memory before our available starting point.
		 */
		if (seg_end <= avail_start)
			continue;

		if (avail_start >= seg_start && avail_start < seg_end) {
			if (seg_start != 0)
				panic("init_x86_64: memory doesn't start at 0");
			seg_start = avail_start;
			if (seg_start == seg_end)
				continue;
		}

		/*
		 * If this segment contains the kernel, split it
		 * in two, around the kernel.
		 */
		if (seg_start <= IOM_END && first_avail <= seg_end) {
			seg_start1 = first_avail;
			seg_end1 = seg_end;
			seg_end = IOM_END;
			KASSERT(seg_end < seg_end1);
		}

		/* First hunk */
		if (seg_start != seg_end) {
			if (seg_start < ADDR_16M &&
			    first16q != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end > ADDR_16M)
					tmp = ADDR_16M;
				else
					tmp = seg_end;

				if (tmp != seg_start) {
#ifdef DEBUG_MEMLOAD
					printf("loading first16q 0x%"PRIx64
					    "-0x%"PRIx64
					    " (0x%"PRIx64"-0x%"PRIx64")\n",
					    seg_start, tmp,
					    (uint64_t)atop(seg_start),
					    (uint64_t)atop(tmp));
#endif
					uvm_page_physload(atop(seg_start),
					    atop(tmp), atop(seg_start),
					    atop(tmp), first16q);
				}
				seg_start = tmp;
			}

#ifdef VM_FREELIST_FIRST4G
			if (seg_start < ADDR_4G &&
			    first4gq != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end > ADDR_4G)
					tmp = ADDR_4G;
				else
					tmp = seg_end;

				if (tmp != seg_start) {
#ifdef DEBUG_MEMLOAD
					printf("loading first4gq 0x%"PRIx64
					    "-0x%"PRIx64
					    " (0x%"PRIx64"-0x%"PRIx64")\n",
					    seg_start, tmp,
					    (uint64_t)atop(seg_start),
					    (uint64_t)atop(tmp));
#endif
					uvm_page_physload(atop(seg_start),
					    atop(tmp), atop(seg_start),
					    atop(tmp), first4gq);
				}
				seg_start = tmp;
			}
#endif /* defined(VM_FREELIST_FIRST4G) */

			if (seg_start != seg_end) {
#ifdef DEBUG_MEMLOAD
				printf("loading default 0x%"PRIx64"-0x%"PRIx64
				    " (0x%"PRIx64"-0x%"PRIx64")\n",
				    seg_start, seg_end,
				    (uint64_t)atop(seg_start),
				    (uint64_t)atop(seg_end));
#endif
				uvm_page_physload(atop(seg_start),
				    atop(seg_end), atop(seg_start),
				    atop(seg_end), VM_FREELIST_DEFAULT);
			}
		}

		/* Second hunk */
		if (seg_start1 != seg_end1) {
			if (seg_start1 < ADDR_16M &&
			    first16q != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end1 > ADDR_16M)
					tmp = ADDR_16M;
				else
					tmp = seg_end1;

				if (tmp != seg_start1) {
#ifdef DEBUG_MEMLOAD
					printf("loading first16q 0x%"PRIx64
					    "-0x%"PRIx64
					    " (0x%"PRIx64"-0x%"PRIx64")\n",
					    seg_start1, tmp,
					    (uint64_t)atop(seg_start1),
					    (uint64_t)atop(tmp));
#endif
					uvm_page_physload(atop(seg_start1),
					    atop(tmp), atop(seg_start1),
					    atop(tmp), first16q);
				}
				seg_start1 = tmp;
			}

#ifdef VM_FREELIST_FIRST4G
			if (seg_start1 < ADDR_4G &&
			    first4gq != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end1 > ADDR_4G)
					tmp = ADDR_4G;
				else
					tmp = seg_end1;

				if (tmp != seg_start1) {
#ifdef DEBUG_MEMLOAD
					printf("loading first4gq 0x%"PRIx64
					    "-0x%"PRIx64
					    " (0x%"PRIx64"-0x%"PRIx64")\n",
					    seg_start1, tmp,
					    (uint64_t)atop(seg_start1),
					    (uint64_t)atop(tmp));
#endif
					uvm_page_physload(atop(seg_start1),
					    atop(tmp), atop(seg_start1),
					    atop(tmp), first4gq);
				}
				seg_start1 = tmp;
			}
#endif /* defined(VM_FREELIST_FIRST4G) */

			if (seg_start1 != seg_end1) {
#ifdef DEBUG_MEMLOAD
				printf("loading default 0x%"PRIx64"-0x%"PRIx64
				    " (0x%"PRIx64"-0x%"PRIx64")\n",
				    seg_start1, seg_end1,
				    (uint64_t)atop(seg_start1),
				    (uint64_t)atop(seg_end1));
#endif
				uvm_page_physload(atop(seg_start1),
				    atop(seg_end1), atop(seg_start1),
				    atop(seg_end1), VM_FREELIST_DEFAULT);
			}
		}
	}

	return 0;
}
#endif

void
x86_reset(void)
{
	uint8_t b;
	/*
	 * The keyboard controller has 4 random output pins, one of which is
	 * connected to the RESET pin on the CPU in many PCs.  We tell the
	 * keyboard controller to pulse this line a couple of times.
	 */
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);
	outb(IO_KBD + KBCMDP, KBC_PULSE0);
	delay(100000);

	/*
	 * Attempt to force a reset via the Reset Control register at
	 * I/O port 0xcf9.  Bit 2 forces a system reset when it
	 * transitions from 0 to 1.  Bit 1 selects the type of reset
	 * to attempt: 0 selects a "soft" reset, and 1 selects a
	 * "hard" reset.  We try a "hard" reset.  The first write sets
	 * bit 1 to select a "hard" reset and clears bit 2.  The
	 * second write forces a 0 -> 1 transition in bit 2 to trigger
	 * a reset.
	 */
	outb(0xcf9, 0x2);
	outb(0xcf9, 0x6);
	DELAY(500000);  /* wait 0.5 sec to see if that did it */

	/*
	 * Attempt to force a reset via the Fast A20 and Init register
	 * at I/O port 0x92.  Bit 1 serves as an alternate A20 gate.
	 * Bit 0 asserts INIT# when set to 1.  We are careful to only
	 * preserve bit 1 while setting bit 0.  We also must clear bit
	 * 0 before setting it if it isn't already clear.
	 */
	b = inb(0x92);
	if (b != 0xff) {
		if ((b & 0x1) != 0)
			outb(0x92, b & 0xfe);
		outb(0x92, b | 0x1);
		DELAY(500000);  /* wait 0.5 sec to see if that did it */
	}
}

static int
x86_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	int result;

	result = KAUTH_RESULT_DEFER;

	switch (action) {
	case KAUTH_MACHDEP_IOPERM_GET:
	case KAUTH_MACHDEP_LDT_GET:
	case KAUTH_MACHDEP_LDT_SET:
	case KAUTH_MACHDEP_MTRR_GET:
		result = KAUTH_RESULT_ALLOW;

		break;

	default:
		break;
	}

	return result;
}

void
machdep_init(void)
{

	x86_listener = kauth_listen_scope(KAUTH_SCOPE_MACHDEP,
	    x86_listener_cb, NULL);
}
