/*	$NetBSD: x86_machdep.c,v 1.105 2018/02/22 09:41:06 maxv Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: x86_machdep.c,v 1.105 2018/02/22 09:41:06 maxv Exp $");

#include "opt_modular.h"
#include "opt_physmem.h"
#include "opt_splash.h"
#include "opt_kaslr.h"
#include "opt_svs.h"

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
#include <sys/rnd.h>

#include <x86/cpuvar.h>
#include <x86/cputypes.h>
#include <x86/efi.h>
#include <x86/machdep.h>
#include <x86/nmi.h>
#include <x86/pio.h>

#include <dev/splash/splash.h>
#include <dev/isa/isareg.h>
#include <dev/ic/i8042reg.h>
#include <dev/mm.h>

#include <machine/bootinfo.h>
#include <machine/vmparam.h>
#include <machine/pmc.h>

#include <uvm/uvm_extern.h>

#include "tsc.h"

#include "acpica.h"
#if NACPICA > 0
#include <dev/acpi/acpivar.h>
#endif

#include "opt_md.h"
#if defined(MEMORY_DISK_HOOKS) && defined(MEMORY_DISK_DYNAMIC)
#include <dev/md.h>
#endif

void (*x86_cpu_idle)(void);
static bool x86_cpu_idle_ipi;
static char x86_cpu_idle_text[16];

#ifdef XEN
char module_machine_amd64_xen[] = "amd64-xen";
char module_machine_i386_xen[] = "i386-xen";
char module_machine_i386pae_xen[] = "i386pae-xen";
#endif


/* --------------------------------------------------------------------- */

/*
 * Main bootinfo structure.  This is filled in by the bootstrap process
 * done in locore.S based on the information passed by the boot loader.
 */
struct bootinfo bootinfo;

/* --------------------------------------------------------------------- */

bool bootmethod_efi;

static kauth_listener_t x86_listener;

extern paddr_t lowmem_rsvd, avail_start, avail_end;

vaddr_t msgbuf_vaddr;

struct msgbuf_p_seg msgbuf_p_seg[VM_PHYSSEG_MAX];

unsigned int msgbuf_p_cnt = 0;

void init_x86_msgbuf(void);

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

#ifdef notyet
/*
 * List the available bootinfo entries.
 */
static const char *btinfo_str[] = {
	BTINFO_STR
};

void
aprint_bootinfo(void)
{
	int i;
	struct btinfo_common *bic;

	aprint_normal("bootinfo:");
	bic = (struct btinfo_common *)(bootinfo.bi_data);
	for (i = 0; i < bootinfo.bi_nentries; i++) {
		if (bic->type >= 0 && bic->type < __arraycount(btinfo_str))
			aprint_normal(" %s", btinfo_str[bic->type]);
		else
			aprint_normal(" %d", bic->type);
		bic = (struct btinfo_common *)
		    ((uint8_t *)bic + bic->len);
	}
	aprint_normal("\n");
}
#endif

/*
 * mm_md_physacc: check if given pa is accessible.
 */
int
mm_md_physacc(paddr_t pa, vm_prot_t prot)
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

	/* setup module path for XEN kernels */
#ifdef XEN
#if defined(amd64)
	module_machine = module_machine_amd64_xen;
#elif defined(i386)
#ifdef PAE
	module_machine = module_machine_i386pae_xen;
#else
	module_machine = module_machine_i386_xen;
#endif
#endif
#endif

	biml = lookup_bootinfo(BTINFO_MODULELIST);
	if (biml == NULL) {
		aprint_debug("No module info at boot\n");
		return;
	}

	bi = (struct bi_modulelist_entry *)((uint8_t *)biml + sizeof(*biml));
	bimax = bi + biml->num;
	for (; bi < bimax; bi++) {
		switch (bi->type) {
		case BI_MODULE_ELF:
			aprint_debug("Prep module path=%s len=%d pa=%x\n",
			    bi->path, bi->len, bi->base);
			KASSERT(trunc_page(bi->base) == bi->base);
			module_prime(bi->path,
#ifdef KASLR
			    (void *)PMAP_DIRECT_MAP((uintptr_t)bi->base),
#else
			    (void *)((uintptr_t)bi->base + KERNBASE),
#endif
			    bi->len);
			break;
		case BI_MODULE_IMAGE:
#ifdef SPLASHSCREEN
			aprint_debug("Splash image path=%s len=%d pa=%x\n",
			    bi->path, bi->len, bi->base);
			KASSERT(trunc_page(bi->base) == bi->base);
			splash_setimage(
#ifdef KASLR
			    (void *)PMAP_DIRECT_MAP((uintptr_t)bi->base),
#else
			    (void *)((uintptr_t)bi->base + KERNBASE),
#endif
			    bi->len);
#endif
			break;
		case BI_MODULE_RND:
			aprint_debug("Random seed data path=%s len=%d pa=%x\n",
				     bi->path, bi->len, bi->base);
			KASSERT(trunc_page(bi->base) == bi->base);
			rnd_seed(
#ifdef KASLR
			    (void *)PMAP_DIRECT_MAP((uintptr_t)bi->base),
#else
			    (void *)((uintptr_t)bi->base + KERNBASE),
#endif
			     bi->len);
			break;
		case BI_MODULE_FS:
			aprint_debug("File-system image path=%s len=%d pa=%x\n",
			    bi->path, bi->len, bi->base);
			KASSERT(trunc_page(bi->base) == bi->base);
#if defined(MEMORY_DISK_HOOKS) && defined(MEMORY_DISK_DYNAMIC)
			md_root_setconf(
#ifdef KASLR
			    (void *)PMAP_DIRECT_MAP((uintptr_t)bi->base),
#else
			    (void *)((uintptr_t)bi->base + KERNBASE),
#endif
			    bi->len);
#endif
			break;	
		default:
			aprint_debug("Skipping non-ELF module\n");
			break;
		}
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
		if (x86_cpu_idle_ipi != false) {
			cpu_kick(ci);
		}
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
		return;
#endif
	}

	aston(l, X86_AST_PREEMPT);
	if (ci == cur) {
		return;
	}
	if ((flags & RESCHED_IMMED) != 0) {
		cpu_kick(ci);
	}
}

void
cpu_signotify(struct lwp *l)
{

	KASSERT(kpreempt_disabled());
	aston(l, X86_AST_GENERIC);
	if (l->l_cpu != curcpu())
		cpu_kick(l->l_cpu);
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
	struct pcb *pcb;
	lwp_t *l;

	KASSERT(kpreempt_disabled());
	l = curlwp;

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
	x86_cpu_idle_set(x86_cpu_idle_xen, "xen", true);
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

static struct {
	int freelist;
	uint64_t limit;
} x86_freelists[VM_NFREELIST] = {
	{ VM_FREELIST_DEFAULT, 0 },
#ifdef VM_FREELIST_FIRST1T
	/* 40-bit addresses needed for modern graphics. */
	{ VM_FREELIST_FIRST1T,	1ULL * 1024 * 1024 * 1024 * 1024 },
#endif
#ifdef VM_FREELIST_FIRST64G
	/* 36-bit addresses needed for oldish graphics. */
	{ VM_FREELIST_FIRST64G, 64ULL * 1024 * 1024 * 1024 },
#endif
#ifdef VM_FREELIST_FIRST4G
	/* 32-bit addresses needed for PCI 32-bit DMA and old graphics. */
	{ VM_FREELIST_FIRST4G,  4ULL * 1024 * 1024 * 1024 },
#endif
	/* 30-bit addresses needed for ancient graphics. */
	{ VM_FREELIST_FIRST1G,	1ULL * 1024 * 1024 * 1024 },
	/* 24-bit addresses needed for ISA DMA. */
	{ VM_FREELIST_FIRST16,	16 * 1024 * 1024 },
};

int
x86_select_freelist(uint64_t maxaddr)
{
	unsigned int i;

	if (avail_end <= maxaddr)
		return VM_NFREELIST;

	for (i = 0; i < __arraycount(x86_freelists); i++) {
		if ((x86_freelists[i].limit - 1) <= maxaddr)
			return x86_freelists[i].freelist;
	}

	panic("no freelist for maximum address %"PRIx64, maxaddr);
}

static int
x86_add_cluster(uint64_t seg_start, uint64_t seg_end, uint32_t type)
{
	extern struct extent *iomem_ex;
	const uint64_t endext = MAXIOMEM + 1;
	uint64_t new_physmem = 0;
	phys_ram_seg_t *cluster;
	int i;

	if (seg_end > MAXPHYSMEM) {
		aprint_verbose("WARNING: skipping large memory map entry: "
		    "0x%"PRIx64"/0x%"PRIx64"/0x%x\n",
		    seg_start, (seg_end - seg_start), type);
		return 0;
	}

	/*
	 * XXX: Chop the last page off the size so that it can fit in avail_end.
	 */
	if (seg_end == MAXPHYSMEM)
		seg_end -= PAGE_SIZE;

	if (seg_end <= seg_start)
		return 0;

	for (i = 0; i < mem_cluster_cnt; i++) {
		cluster = &mem_clusters[i];
		if ((cluster->start == round_page(seg_start)) &&
		    (cluster->size == trunc_page(seg_end) - cluster->start)) {
#ifdef DEBUG_MEMLOAD
			printf("WARNING: skipping duplicate segment entry\n");
#endif
			return 0;
		}
	}

	/*
	 * This cluster is used by RAM. If it is included in the iomem extent,
	 * allocate it from there, so that we won't unintentionally reuse it
	 * later with extent_alloc_region. A way to avoid collision (with UVM
	 * for example).
	 *
	 * This is done before the addresses are page rounded just to make
	 * sure we get them all.
	 */
	if (seg_start < endext) {
		uint64_t io_end;

		if (seg_end > endext)
			io_end = endext;
		else
			io_end = seg_end;

		if (iomem_ex != NULL && extent_alloc_region(iomem_ex, seg_start,
		    io_end - seg_start, EX_NOWAIT)) {
			/* XXX What should we do? */
			printf("WARNING: CAN't ALLOCATE MEMORY SEGMENT "
			    "(0x%"PRIx64"/0x%"PRIx64"/0x%x) FROM "
			    "IOMEM EXTENT MAP!\n",
			    seg_start, seg_end - seg_start, type);
			return 0;
		}
	}

	/* If it's not free memory, skip it. */
	if (type != BIM_Memory)
		return 0;

	if (mem_cluster_cnt >= VM_PHYSSEG_MAX) {
		printf("WARNING: too many memory segments"
		    "(increase VM_PHYSSEG_MAX)");
		return -1;
	}

#ifdef PHYSMEM_MAX_ADDR
	if (seg_start >= MBTOB(PHYSMEM_MAX_ADDR))
		return 0;
	if (seg_end > MBTOB(PHYSMEM_MAX_ADDR))
		seg_end = MBTOB(PHYSMEM_MAX_ADDR);
#endif

	seg_start = round_page(seg_start);
	seg_end = trunc_page(seg_end);

	if (seg_start == seg_end)
		return 0;

	cluster = &mem_clusters[mem_cluster_cnt];
	cluster->start = seg_start;
	if (iomem_ex != NULL)
		new_physmem = physmem + atop(seg_end - seg_start);

#ifdef PHYSMEM_MAX_SIZE
	if (iomem_ex != NULL) {
		if (physmem >= atop(MBTOB(PHYSMEM_MAX_SIZE)))
			return 0;
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
	mem_cluster_cnt++;

	return 0;
}

static int
x86_parse_clusters(struct btinfo_memmap *bim)
{
	uint64_t seg_start, seg_end;
	uint64_t addr, size;
	uint32_t type;
	int x;

	KASSERT(bim != NULL);
	KASSERT(bim->num > 0);

#ifdef DEBUG_MEMLOAD
	printf("MEMMAP: %s MEMORY MAP (%d ENTRIES):\n",
	    lookup_bootinfo(BTINFO_EFIMEMMAP) != NULL ? "UEFI" : "BIOS",
	    bim->num);
#endif

	for (x = 0; x < bim->num; x++) {
		addr = bim->entry[x].addr;
		size = bim->entry[x].size;
		type = bim->entry[x].type;
#ifdef DEBUG_MEMLOAD
		printf("MEMMAP: 0x%016" PRIx64 "-0x%016" PRIx64
		    ", size=0x%016" PRIx64 ", type=%d(%s)\n",
		    addr, addr + size - 1, size, type,
		    (type == BIM_Memory) ?  "Memory" :
		    (type == BIM_Reserved) ?  "Reserved" :
		    (type == BIM_ACPI) ? "ACPI" :
		    (type == BIM_NVS) ? "NVS" :
		    (type == BIM_PMEM) ? "Persistent" :
		    (type == BIM_PRAM) ? "Persistent (Legacy)" :
		    "unknown");
#endif

		/* If the segment is not memory, skip it. */
		switch (type) {
		case BIM_Memory:
		case BIM_ACPI:
		case BIM_NVS:
			break;
		default:
			continue;
		}

		/* If the segment is smaller than a page, skip it. */
		if (size < PAGE_SIZE)
			continue;

		seg_start = addr;
		seg_end = addr + size;

		/*
		 * XXX XXX: Avoid the ISA I/O MEM.
		 * 
		 * Some laptops (for example, Toshiba Satellite2550X) report
		 * this area as valid.
		 */
		if (seg_start < IOM_END && seg_end > IOM_BEGIN) {
			printf("WARNING: memory map entry overlaps "
			    "with ``Compatibility Holes'': "
			    "0x%"PRIx64"/0x%"PRIx64"/0x%x\n", seg_start,
			    seg_end - seg_start, type);

			if (x86_add_cluster(seg_start, IOM_BEGIN, type) == -1)
				break;
			if (x86_add_cluster(IOM_END, seg_end, type) == -1)
				break;
		} else {
			if (x86_add_cluster(seg_start, seg_end, type) == -1)
				break;
		}
	}

	return 0;
}

static int
x86_fake_clusters(void)
{
	extern struct extent *iomem_ex;
	phys_ram_seg_t *cluster;
	KASSERT(mem_cluster_cnt == 0);

	/*
	 * Allocate the physical addresses used by RAM from the iomem extent
	 * map. This is done before the addresses are page rounded just to make
	 * sure we get them all.
	 */
	if (extent_alloc_region(iomem_ex, 0, KBTOB(biosbasemem), EX_NOWAIT)) {
		/* XXX What should we do? */
		printf("WARNING: CAN'T ALLOCATE BASE MEMORY FROM "
		    "IOMEM EXTENT MAP!\n");
	}

	cluster = &mem_clusters[0];
	cluster->start = 0;
	cluster->size = trunc_page(KBTOB(biosbasemem));
	physmem += atop(cluster->size);

	if (extent_alloc_region(iomem_ex, IOM_END, KBTOB(biosextmem),
	    EX_NOWAIT)) {
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

		format_bytes(pbuf, sizeof(pbuf), biosextmem - (15*1024));
		printf("Warning: ignoring %s of remapped memory\n", pbuf);
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

/*
 * x86_load_region: load the physical memory region from seg_start to seg_end
 * into the VM system.
 */
static void
x86_load_region(uint64_t seg_start, uint64_t seg_end)
{
	unsigned int i;
	uint64_t tmp;

	i = __arraycount(x86_freelists);
	while (i--) {
		if (x86_freelists[i].limit <= seg_start)
			continue;
		if (x86_freelists[i].freelist == VM_FREELIST_DEFAULT)
			continue;
		tmp = MIN(x86_freelists[i].limit, seg_end);
		if (tmp == seg_start)
			continue;

#ifdef DEBUG_MEMLOAD
		printf("loading freelist %d 0x%"PRIx64"-0x%"PRIx64
		    " (0x%"PRIx64"-0x%"PRIx64")\n", x86_freelists[i].freelist,
		    seg_start, tmp, (uint64_t)atop(seg_start),
		    (uint64_t)atop(tmp));
#endif

		uvm_page_physload(atop(seg_start), atop(tmp), atop(seg_start),
		    atop(tmp), x86_freelists[i].freelist);
		seg_start = tmp;
	}

	if (seg_start != seg_end) {
#ifdef DEBUG_MEMLOAD
		printf("loading default 0x%"PRIx64"-0x%"PRIx64
		    " (0x%"PRIx64"-0x%"PRIx64")\n", seg_start, seg_end,
		    (uint64_t)atop(seg_start), (uint64_t)atop(seg_end));
#endif
		uvm_page_physload(atop(seg_start), atop(seg_end),
		    atop(seg_start), atop(seg_end), VM_FREELIST_DEFAULT);
	}
}

/*
 * init_x86_clusters: retrieve the memory clusters provided by the BIOS, and
 * initialize mem_clusters.
 */
void
init_x86_clusters(void)
{
	struct btinfo_memmap *bim;
	struct btinfo_efimemmap *biem;

	/*
	 * Check to see if we have a memory map from the BIOS (passed to us by
	 * the boot program).
	 */
#ifdef i386
	extern int biosmem_implicit;
	biem = lookup_bootinfo(BTINFO_EFIMEMMAP);
	if (biem != NULL)
		bim = efi_get_e820memmap();
	else
		bim = lookup_bootinfo(BTINFO_MEMMAP);
	if ((biosmem_implicit || (biosbasemem == 0 && biosextmem == 0)) &&
	    bim != NULL && bim->num > 0)
		x86_parse_clusters(bim);
#else
#if !defined(REALBASEMEM) && !defined(REALEXTMEM)
	biem = lookup_bootinfo(BTINFO_EFIMEMMAP);
	if (biem != NULL)
		bim = efi_get_e820memmap();
	else
		bim = lookup_bootinfo(BTINFO_MEMMAP);
	if (bim != NULL && bim->num > 0)
		x86_parse_clusters(bim);
#else
	(void)bim, (void)biem;
#endif
#endif

	if (mem_cluster_cnt == 0) {
		/*
		 * If x86_parse_clusters didn't find any valid segment, create
		 * fake clusters.
		 */
		x86_fake_clusters();
	}
}

/*
 * init_x86_vm: initialize the VM system on x86. We basically internalize as
 * many physical pages as we can, starting at lowmem_rsvd, but we don't
 * internalize the kernel physical pages (from pa_kstart to pa_kend).
 */
int
init_x86_vm(paddr_t pa_kend)
{
	extern struct bootspace bootspace;
	paddr_t pa_kstart = bootspace.head.pa;
	uint64_t seg_start, seg_end;
	uint64_t seg_start1, seg_end1;
	int x;
	unsigned i;

	for (i = 0; i < __arraycount(x86_freelists); i++) {
		if (avail_end < x86_freelists[i].limit)
			x86_freelists[i].freelist = VM_FREELIST_DEFAULT;
	}

	/*
	 * Now, load the memory clusters (which have already been rounded and
	 * truncated) into the VM system.
	 *
	 * NOTE: we assume that memory starts at 0.
	 */
	for (x = 0; x < mem_cluster_cnt; x++) {
		const phys_ram_seg_t *cluster = &mem_clusters[x];

		seg_start = cluster->start;
		seg_end = cluster->start + cluster->size;
		seg_start1 = 0;
		seg_end1 = 0;

		/* Skip memory before our available starting point. */
		if (seg_end <= lowmem_rsvd)
			continue;

		if (seg_start <= lowmem_rsvd && lowmem_rsvd < seg_end) {
			seg_start = lowmem_rsvd;
			if (seg_start == seg_end)
				continue;
		}

		/*
		 * If this segment contains the kernel, split it in two, around
		 * the kernel.
		 */
		if (seg_start <= pa_kstart && pa_kend <= seg_end) {
			seg_start1 = pa_kend;
			seg_end1 = seg_end;
			seg_end = pa_kstart;
			KASSERT(seg_end < seg_end1);
		}

		/* First hunk */
		if (seg_start != seg_end) {
			x86_load_region(seg_start, seg_end);
		}

		/* Second hunk */
		if (seg_start1 != seg_end1) {
			x86_load_region(seg_start1, seg_end1);
		}
	}

	return 0;
}

#endif /* !XEN */

void
init_x86_msgbuf(void)
{
	/* Message buffer is located at end of core. */
	psize_t sz = round_page(MSGBUFSIZE);
	psize_t reqsz = sz;
	uvm_physseg_t x;
		
 search_again:
        for (x = uvm_physseg_get_first();
	     uvm_physseg_valid_p(x);
	     x = uvm_physseg_get_next(x)) {

		if (ctob(uvm_physseg_get_avail_end(x)) == avail_end)
			break;
	}

	if (uvm_physseg_valid_p(x) == false)
		panic("init_x86_msgbuf: can't find end of memory");

	/* Shrink so it'll fit in the last segment. */
	if (uvm_physseg_get_avail_end(x) - uvm_physseg_get_avail_start(x) < atop(sz))
		sz = ctob(uvm_physseg_get_avail_end(x) - uvm_physseg_get_avail_start(x));

	msgbuf_p_seg[msgbuf_p_cnt].sz = sz;
	msgbuf_p_seg[msgbuf_p_cnt++].paddr = ctob(uvm_physseg_get_avail_end(x)) - sz;
	uvm_physseg_unplug(uvm_physseg_get_end(x) - atop(sz), atop(sz));

	/* Now find where the new avail_end is. */
	avail_end = ctob(uvm_physseg_get_highest_frame());

	if (sz == reqsz)
		return;

	reqsz -= sz;
	if (msgbuf_p_cnt == VM_PHYSSEG_MAX) {
		/* No more segments available, bail out. */
		printf("WARNING: MSGBUFSIZE (%zu) too large, using %zu.\n",
		    (size_t)MSGBUFSIZE, (size_t)(MSGBUFSIZE - reqsz));
		return;
	}

	sz = reqsz;
	goto search_again;
}

void
x86_reset(void)
{
	uint8_t b;

#if NACPICA > 0
	/*
	 * If ACPI is active, try to reset using the reset register
	 * defined in the FADT.
	 */
	if (acpi_active) {
		if (acpi_reset() == 0) {
			delay(500000); /* wait 0.5 sec to see if that did it */
		}
	}
#endif

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
	DELAY(500000);	/* wait 0.5 sec to see if that did it */

	/*
	 * Attempt to force a reset via the Fast A20 and Init register
	 * at I/O port 0x92. Bit 1 serves as an alternate A20 gate.
	 * Bit 0 asserts INIT# when set to 1. We are careful to only
	 * preserve bit 1 while setting bit 0. We also must clear bit
	 * 0 before setting it if it isn't already clear.
	 */
	b = inb(0x92);
	if (b != 0xff) {
		if ((b & 0x1) != 0)
			outb(0x92, b & 0xfe);
		outb(0x92, b | 0x1);
		DELAY(500000);	/* wait 0.5 sec to see if that did it */
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

/*
 * x86_startup: x86 common startup routine
 *
 * called by cpu_startup.
 */

void
x86_startup(void)
{
#if SVS
	svs_init(false);
#endif
#if !defined(XEN)
	nmi_init();
	pmc_init();
#endif
}

/* 
 * machine dependent system variables.
 */
static int
sysctl_machdep_booted_kernel(SYSCTLFN_ARGS)
{
	struct btinfo_bootpath *bibp;
	struct sysctlnode node;

	bibp = lookup_bootinfo(BTINFO_BOOTPATH);
	if(!bibp)
		return ENOENT; /* ??? */

	node = *rnode;
	node.sysctl_data = bibp->bootpath;
	node.sysctl_size = sizeof(bibp->bootpath);
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

static int
sysctl_machdep_bootmethod(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	char buf[5];

	node = *rnode;
	node.sysctl_data = buf;
	if (bootmethod_efi)
		memcpy(node.sysctl_data, "UEFI", 5);
	else
		memcpy(node.sysctl_data, "BIOS", 5);

	return sysctl_lookup(SYSCTLFN_CALL(&node));
}


static int
sysctl_machdep_diskinfo(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	extern struct bi_devmatch *x86_alldisks;
	extern int x86_ndisks;

	if (x86_alldisks == NULL)
		return EOPNOTSUPP;

	node = *rnode;
	node.sysctl_data = x86_alldisks;
	node.sysctl_size = sizeof(struct disklist) +
	    (x86_ndisks - 1) * sizeof(struct nativedisk_info);
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

#ifndef XEN
static int
sysctl_machdep_tsc_enable(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int error, val;

	val = *(int *)rnode->sysctl_data;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL)
		return error;

	if (val == 1) {
		tsc_user_enable();
	} else if (val == 0) {
		tsc_user_disable();
	} else {
		error = EINVAL;
	}
	if (error)
		return error;

	*(int *)rnode->sysctl_data = val;

	return 0;
}
#endif

static void
const_sysctl(struct sysctllog **clog, const char *name, int type,
    u_quad_t value, int tag)
{
	(sysctl_createv)(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT | CTLFLAG_IMMEDIATE,
		       type, name, NULL, NULL, value, NULL, 0,
		       CTL_MACHDEP, tag, CTL_EOL);
}

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{
	extern uint64_t tsc_freq;
#ifndef XEN
	extern int tsc_user_enabled;
#endif
	extern int sparse_dump;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "console_device", NULL,
		       sysctl_consdev, 0, NULL, sizeof(dev_t),
		       CTL_MACHDEP, CPU_CONSDEV, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "booted_kernel", NULL,
		       sysctl_machdep_booted_kernel, 0, NULL, 0,
		       CTL_MACHDEP, CPU_BOOTED_KERNEL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "bootmethod", NULL,
		       sysctl_machdep_bootmethod, 0, NULL, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "diskinfo", NULL,
		       sysctl_machdep_diskinfo, 0, NULL, 0,
		       CTL_MACHDEP, CPU_DISKINFO, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "cpu_brand", NULL,
		       NULL, 0, cpu_brand_string, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "sparse_dump", NULL,
		       NULL, 0, &sparse_dump, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_QUAD, "tsc_freq", NULL,
		       NULL, 0, &tsc_freq, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "pae",
		       SYSCTL_DESCR("Whether the kernel uses PAE"),
		       NULL, 0, &use_pae, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
#ifndef XEN
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_READWRITE,
		       CTLTYPE_INT, "tsc_user_enable",
		       SYSCTL_DESCR("RDTSC instruction enabled in usermode"),
		       sysctl_machdep_tsc_enable, 0, &tsc_user_enabled, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
#endif
#ifdef SVS
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_BOOL, "svs_enabled",
		       SYSCTL_DESCR("Whether the kernel uses SVS"),
		       NULL, 0, &svs_enabled, 0,
		       CTL_MACHDEP, CTL_CREATE, CTL_EOL);
#endif

	/* None of these can ever change once the system has booted */
	const_sysctl(clog, "fpu_present", CTLTYPE_INT, i386_fpu_present,
	    CPU_FPU_PRESENT);
	const_sysctl(clog, "osfxsr", CTLTYPE_INT, i386_use_fxsave,
	    CPU_OSFXSR);
	const_sysctl(clog, "sse", CTLTYPE_INT, i386_has_sse,
	    CPU_SSE);
	const_sysctl(clog, "sse2", CTLTYPE_INT, i386_has_sse2,
	    CPU_SSE2);

	const_sysctl(clog, "fpu_save", CTLTYPE_INT, x86_fpu_save,
	    CPU_FPU_SAVE);
	const_sysctl(clog, "fpu_save_size", CTLTYPE_INT, x86_fpu_save_size,
	    CPU_FPU_SAVE_SIZE);
	const_sysctl(clog, "xsave_features", CTLTYPE_QUAD, x86_xsave_features,
	    CPU_XSAVE_FEATURES);

#ifndef XEN
	const_sysctl(clog, "biosbasemem", CTLTYPE_INT, biosbasemem,
	    CPU_BIOSBASEMEM);
	const_sysctl(clog, "biosextmem", CTLTYPE_INT, biosextmem,
	    CPU_BIOSEXTMEM);
#endif
}
