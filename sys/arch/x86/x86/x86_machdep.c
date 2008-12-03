/*	$NetBSD: x86_machdep.c,v 1.26 2008/12/03 11:40:17 ad Exp $	*/

/*-
 * Copyright (c) 2002, 2006, 2007 YAMAMOTO Takashi,
 * Copyright (c) 2005, 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: x86_machdep.c,v 1.26 2008/12/03 11:40:17 ad Exp $");

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

#include <x86/cpu_msr.h>
#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/machdep.h>

#include <dev/isa/isareg.h>

#include <machine/bootinfo.h>
#include <machine/vmparam.h>

#include <uvm/uvm_extern.h>

int check_pa_acc(paddr_t, vm_prot_t);

/* --------------------------------------------------------------------- */

/*
 * Main bootinfo structure.  This is filled in by the bootstrap process
 * done in locore.S based on the information passed by the boot loader.
 */
struct bootinfo bootinfo;

/* --------------------------------------------------------------------- */

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

/*
 * This function is to initialize the mutex used by x86/msr_ipifuncs.c.
 */
void
x86_init(void)
{
#ifndef XEN
	msr_cpu_broadcast_initmtx();
#endif
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
		if (x86_cpu_idle == x86_cpu_idle_halt)
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
	((struct pcb *)l->l_addr)->pcb_cr2 = rcr2();

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
	lcr2(((struct pcb *)curlwp->l_addr)->pcb_cr2);
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

void (*x86_cpu_idle)(void);
static char x86_cpu_idle_text[16];

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
	if ((curcpu()->ci_feature2_flags & CPUID2_MONITOR) == 0 ||
	    cpu_vendor == CPUVENDOR_AMD) {
		strlcpy(x86_cpu_idle_text, "halt", sizeof(x86_cpu_idle_text));
		x86_cpu_idle = x86_cpu_idle_halt;
	} else {
		strlcpy(x86_cpu_idle_text, "mwait", sizeof(x86_cpu_idle_text));
		x86_cpu_idle = x86_cpu_idle_mwait;
	}
#else
	strlcpy(x86_cpu_idle_text, "xen", sizeof(x86_cpu_idle_text));
	x86_cpu_idle = x86_cpu_idle_xen;
#endif
}


#ifndef XEN

#define KBTOB(x)	((size_t)(x) * 1024UL)
#define MBTOB(x)	((size_t)(x) * 1024UL * 1024UL)

extern paddr_t avail_start, avail_end;

static void
add_mem_cluster(uint64_t seg_start, uint64_t seg_end, uint32_t type)
{
	extern struct extent *iomem_ex;
	uint64_t io_end, new_physmem;
	int i;

#ifdef i386
#define TOPLIMIT	0x100000000ULL
#else
#define TOPLIMIT	0x100000000000ULL
#endif

	if (seg_end > TOPLIMIT) {
		printf("WARNING: skipping large "
		    "memory map entry: "
		    "0x%"PRIx64"/0x%"PRIx64"/0x%x\n",
		    seg_start,
		    (seg_end - seg_start),
		    type);
		return;
	}

	/*
	 * XXX Chop the last page off the size so that
	 * XXX it can fit in avail_end.
	 */
	if (seg_end == TOPLIMIT)
		seg_end -= PAGE_SIZE;

	if (seg_end <= seg_start)
		return;

	for (i = 0; i < mem_cluster_cnt; i++) {
		if ((mem_clusters[i].start == round_page(seg_start))
		    && (mem_clusters[i].size
			== trunc_page(seg_end) - mem_clusters[i].start))
		{
#ifdef DEBUG_MEMLOAD
			printf("WARNING: skipping duplicate segment entry\n");
#endif
			return;
		}
	}

	/*
	 * Allocate the physical addresses used by RAM
	 * from the iomem extent map.  This is done before
	 * the addresses are page rounded just to make
	 * sure we get them all.
	 */
	if (seg_start < 0x100000000ULL) {
		if (seg_end > 0x100000000ULL)
			io_end = 0x100000000ULL;
		else
			io_end = seg_end;

		if (extent_alloc_region(iomem_ex, seg_start,
		    io_end - seg_start, EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN't ALLOCATE MEMORY SEGMENT "
			    "(0x%"PRIx64"/0x%"PRIx64"/0x%x) FROM "
			    "IOMEM EXTENT MAP!\n",
			    seg_start, seg_end - seg_start, type);
			return;
		}
	}

	/*
	 * If it's not free memory, skip it.
	 */
	if (type != BIM_Memory)
		return;

	/* XXX XXX XXX */
	if (mem_cluster_cnt >= VM_PHYSSEG_MAX)
		panic("%s: too many memory segments (increase VM_PHYSSEG_MAX)",
			__func__);

#ifdef PHYSMEM_MAX_ADDR
	if (seg_start >= MBTOB(PHYSMEM_MAX_ADDR))
		return;
	if (seg_end > MBTOB(PHYSMEM_MAX_ADDR))
		seg_end = MBTOB(PHYSMEM_MAX_ADDR);
#endif  

	seg_start = round_page(seg_start);
	seg_end = trunc_page(seg_end);

	if (seg_start == seg_end)
		return;

	mem_clusters[mem_cluster_cnt].start = seg_start;
	new_physmem = physmem + atop(seg_end - seg_start);

#ifdef PHYSMEM_MAX_SIZE
	if (physmem >= atop(MBTOB(PHYSMEM_MAX_SIZE)))
		return;
	if (new_physmem > atop(MBTOB(PHYSMEM_MAX_SIZE))) {
		seg_end = seg_start + MBTOB(PHYSMEM_MAX_SIZE) - ptoa(physmem);
		new_physmem = atop(MBTOB(PHYSMEM_MAX_SIZE));
	}
#endif  

	mem_clusters[mem_cluster_cnt].size = seg_end - seg_start;

	if (avail_end < seg_end)
		avail_end = seg_end;
	physmem = new_physmem;
	mem_cluster_cnt++;
}

int
initx86_parse_memmap(struct btinfo_memmap *bim)
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
		 * XXX (0xa0000-0xbffff),to adapter ROM space
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
			add_mem_cluster(seg_start, 0xa0000, type);
			add_mem_cluster(0x100000, seg_end, type);
		} else
			add_mem_cluster(seg_start, seg_end, type);
	}

	return 0;
}

int
initx86_fake_memmap(struct extent *iomem_ex)
{
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

	mem_clusters[0].start = 0;
	mem_clusters[0].size = trunc_page(KBTOB(biosbasemem));
	physmem += atop(mem_clusters[0].size);

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
	mem_clusters[1].start = IOM_END;
	mem_clusters[1].size = trunc_page(KBTOB(biosextmem));
	physmem += atop(mem_clusters[1].size);

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

	/*
	 * If we have 16M of RAM or less, just put it all on
	 * the default free list.  Otherwise, put the first
	 * 16M of RAM on a lower priority free list (so that
	 * all of the ISA DMA'able memory won't be eaten up
	 * first-off).
	 */
	if (avail_end <= (16 * 1024 * 1024))
		first16q = VM_FREELIST_DEFAULT;
	else
		first16q = VM_FREELIST_FIRST16;

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
		seg_start = mem_clusters[x].start;
		seg_end = mem_clusters[x].start + mem_clusters[x].size;
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
		}

		/* First hunk */
		if (seg_start != seg_end) {
			if (seg_start < (16 * 1024 * 1024) &&
			    first16q != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end > (16 * 1024 * 1024))
					tmp = (16 * 1024 * 1024);
				else
					tmp = seg_end;

				if (tmp != seg_start) {
#ifdef DEBUG_MEMLOAD
					printf("loading 0x%"PRIx64"-0x%"PRIx64
					    " (0x%lx-0x%lx)\n",
					    seg_start, tmp,
					    atop(seg_start), atop(tmp));
#endif
					uvm_page_physload(atop(seg_start),
					    atop(tmp), atop(seg_start),
					    atop(tmp), first16q);
				}
				seg_start = tmp;
			}

			if (seg_start != seg_end) {
#ifdef DEBUG_MEMLOAD
				printf("loading 0x%"PRIx64"-0x%"PRIx64
				    " (0x%lx-0x%lx)\n",
				    seg_start, seg_end,
				    atop(seg_start), atop(seg_end));
#endif
				uvm_page_physload(atop(seg_start),
				    atop(seg_end), atop(seg_start),
				    atop(seg_end), VM_FREELIST_DEFAULT);
			}
		}

		/* Second hunk */
		if (seg_start1 != seg_end1) {
			if (seg_start1 < (16 * 1024 * 1024) &&
			    first16q != VM_FREELIST_DEFAULT) {
				uint64_t tmp;

				if (seg_end1 > (16 * 1024 * 1024))
					tmp = (16 * 1024 * 1024);
				else
					tmp = seg_end1;

				if (tmp != seg_start1) {
#ifdef DEBUG_MEMLOAD
					printf("loading 0x%"PRIx64"-0x%"PRIx64
					    " (0x%lx-0x%lx)\n",
					    seg_start1, tmp,
					    atop(seg_start1), atop(tmp));
#endif
					uvm_page_physload(atop(seg_start1),
					    atop(tmp), atop(seg_start1),
					    atop(tmp), first16q);
				}
				seg_start1 = tmp;
			}

			if (seg_start1 != seg_end1) {
#ifdef DEBUG_MEMLOAD
				printf("loading 0x%"PRIx64"-0x%"PRIx64
				    " (0x%lx-0x%lx)\n",
				    seg_start1, seg_end1,
				    atop(seg_start1), atop(seg_end1));
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
