/*      $NetBSD: bootinfo.c,v 1.11 2025/11/06 15:27:10 thorpej Exp $        */

/*-
 * Copyright (c) 2023, 2025 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bootinfo.c,v 1.11 2025/11/06 15:27:10 thorpej Exp $");

#include "opt_md.h"

#include <sys/types.h>
#include <sys/cpu.h>
#include <sys/rnd.h>
#include <sys/rndsource.h>

#include <uvm/uvm_extern.h>

#ifdef MEMORY_DISK_DYNAMIC
#include <dev/md.h>
#endif

#include <machine/bootinfo.h>
#include <machine/vmparam.h>

#include "gftty.h"
#if NGFTTY > 0
#include <dev/goldfish/gfttyvar.h>
#endif

static struct bi_mem_info	bid_mem_segments[VM_PHYSSEG_MAX];
static struct bi_mem_info	bid_mem_segments_avail[VM_PHYSSEG_MAX];

static struct bootinfo_data	bootinfo_data_store;

#define	PA_TO_VA(pa)	(VM_MIN_KERNEL_ADDRESS + ((pa) - reloff))
#define	VA_TO_PA(va)	((((vaddr_t)(va)) - VM_MIN_KERNEL_ADDRESS) + reloff)
#define	RELOC(v, t)	*((t *)VA_TO_PA(&(v)))

#if NGFTTY > 0
static bool
bootinfo_set_console(struct bootinfo_data *bid, paddr_t pa)
{
	if (! bid->bootinfo_console_addr_valid) {
		bid->bootinfo_console_addr = pa;
		bid->bootinfo_console_addr_valid = true;
		return true;
	}
	return false;
}
#endif

/*
 * We use always_inline for the parsers that are called from
 * bootinfo_startup1() to avoid having a global reference to
 * relocate.
 */

static inline struct bi_record * __attribute__((always_inline))
bootinfo_next(struct bi_record *bi)
{
	uintptr_t addr = (uintptr_t)bi;

	addr += bi->bi_size;
	return (struct bi_record *)addr;
}

static inline int __attribute__((always_inline))
bootinfo_get_cpu(struct bi_record *bi)
{
	switch (bootinfo_get_u32(bi)) {
	case BI_CPU_68020:	return CPU_68020;
	case BI_CPU_68030:	return CPU_68030;
	case BI_CPU_68040:	return CPU_68040;
	case BI_CPU_68060:	return CPU_68060;
	default:		return -666;
	}
}

static inline int __attribute__((always_inline))
bootinfo_get_fpu(struct bi_record *bi)
{
	switch (bootinfo_get_u32(bi)) {
	case BI_FPU_68881:	return FPU_68881;
	case BI_FPU_68882:	return FPU_68882;
	case BI_FPU_68040:	return FPU_68040;
	case BI_FPU_68060:	return FPU_68060;
	default:		return FPU_UNKNOWN;
	}
}

static inline int __attribute__((always_inline))
bootinfo_get_mmu(struct bi_record *bi)
{
	switch (bootinfo_get_u32(bi)) {
	case BI_MMU_68851:	return MMU_68851;
	case BI_MMU_68030:	return MMU_68030;
	case BI_MMU_68040:	return MMU_68040;
	case BI_MMU_68060:	return MMU_68040;	/* XXX */
	case BI_MMU_SUN3:	return MMU_SUN;
	case BI_MMU_APOLLO:	/* XXX MMU_HP ??? */
	case BI_MMU_COLDFIRE:
	default:		return FPU_UNKNOWN;
	}
}

static inline void __attribute__((always_inline))
bootinfo_add_mem(struct bootinfo_data *bid, struct bi_record *bi)
{
	struct bi_mem_info *m = bootinfo_dataptr(bi);

	if (bid->bootinfo_mem_nsegments == VM_PHYSSEG_MAX) {
		bid->bootinfo_mem_segments_ignored++;
		bid->bootinfo_mem_segments_ignored_bytes += m->mem_size;
	}

	/*
	 * Make sure the start / size are properly aligned.
	 */
	if (m->mem_addr & PGOFSET) {
		m->mem_size -= m->mem_addr & PGOFSET;
		m->mem_addr = m68k_round_page(m->mem_addr);
	}
	m->mem_size = m68k_trunc_page(m->mem_size);
	bid->bootinfo_total_mem_pages += m->mem_size >> PGSHIFT;

	bid->bootinfo_mem_segments[bid->bootinfo_mem_nsegments++] = *m;
	bid->bootinfo_mem_segments_avail[bid->bootinfo_mem_nsegments_avail++]
	    = *m;
}

static inline void __attribute__((always_inline))
bootinfo_add_initrd(struct bootinfo_data *bid, struct bi_record *bi)
{
	struct bi_mem_info *rd = bootinfo_dataptr(bi);

	if (bid->bootinfo_initrd_size == 0) {
		bid->bootinfo_initrd_start = rd->mem_addr;
		bid->bootinfo_initrd_size  = rd->mem_size;
	}
}

static void
bootinfo_reserve_initrd(struct bootinfo_data *bid)
{
	if (bid->bootinfo_initrd_size == 0) {
		return;
	}

	paddr_t initrd_start = bid->bootinfo_initrd_start;
	paddr_t initrd_end   = bid->bootinfo_initrd_start +
			       bid->bootinfo_initrd_size;
	int i;

	/* Page-align the RAM disk start/end. */
	initrd_end = m68k_round_page(initrd_end);
	initrd_start = m68k_trunc_page(initrd_start);

	/*
	 * XXX All if this code assumes that the RAM disk fits within
	 * XXX a single memory segment.
	 */

	for (i = 0; i < bid->bootinfo_mem_nsegments_avail; i++) {
		/* Memory segment start/end already page-aligned. */
		paddr_t seg_start =
		    bid->bootinfo_mem_segments_avail[i].mem_addr;
		paddr_t seg_end = seg_start +
		    bid->bootinfo_mem_segments_avail[i].mem_size;

		if (initrd_start >= seg_end ||
		    initrd_end <= seg_start) {
			/* Does not fall within this segment. */
			continue;
		}

		if (initrd_start > seg_start && initrd_end < seg_end) {
			/* We need to split this segment. */
			/* XXX */
			printf("WARNING: ignoring RAM disk that splits "
			       "memory segment.\n");
			bid->bootinfo_initrd_size = 0;
			return;
		}

		printf("Reserving RAM disk pages %p - %p from memory "
		       "segment %d.\n", (void *)initrd_start,
		       (void *)(initrd_end - 1), i);

		if (initrd_start == seg_start) {
			seg_start = initrd_end;
		}

		if (initrd_end == seg_end) {
			seg_end = initrd_start;
		}

		/* Now adjust the segment. */
		bid->bootinfo_mem_segments_avail[i].mem_addr = seg_start;
		bid->bootinfo_mem_segments_avail[i].mem_size =
		    seg_end - seg_start;
		return;
	}
}

static inline void
bootinfo_gf_tty_consinit(struct bootinfo_data *bid, struct bi_record *bi)
{
#if NGFTTY > 0
	struct bi_virt_dev *vd = bootinfo_dataptr(bi);

	/*
	 * vd_mmio_base is the PA, but we're going to run mapped
	 * VA==PA for devices anyway once the MMU is turned on.
	 */
	if (bootinfo_set_console(bid, vd->vd_mmio_base)) {
		bootinfo_md_cnattach(gftty_cnattach,
		    vd->vd_mmio_base, 0x1000);
		printf("Initialized Goldfish TTY console @ 0x%08x\n",
		    vd->vd_mmio_base);
	}
#endif /* NGFTTY > 0 */
}

/*
 * bootinfo_startup1 --
 *	Parse the boot info during early start-up, before the MMU is
 *	turned on.  Returns the address of the end of the boot info.
 *
 *	Because the MMU is not yet enabled, we need to manually relocate
 *	global references.
 */
vaddr_t
bootinfo_startup1(struct bi_record *first, vaddr_t reloff)
{
	struct bootinfo_data *bid =
	    (struct bootinfo_data *)VA_TO_PA(&bootinfo_data_store);
	struct bi_record *bi;

	bid->bootinfo = first;
	bid->bootinfo_mem_segments =
	    (struct bi_mem_info *)VA_TO_PA(bid_mem_segments);
	bid->bootinfo_mem_segments_avail =
	    (struct bi_mem_info *)VA_TO_PA(bid_mem_segments_avail);

	for (bi = bid->bootinfo; bi->bi_tag != BI_LAST;
	     bi = bootinfo_next(bi)) {
		switch (bi->bi_tag) {
		case BI_MACHTYPE:
			bid->bootinfo_machtype = bootinfo_get_u32(bi);
			break;

		case BI_CPUTYPE:
			RELOC(cputype, int) = bootinfo_get_cpu(bi);
			break;

		case BI_FPUTYPE:
			RELOC(fputype, int) = bootinfo_get_fpu(bi);
			break;

		case BI_MMUTYPE:
			RELOC(mmutype, int) = bootinfo_get_mmu(bi);
			break;

		case BI_MEMCHUNK:
			bootinfo_add_mem(bid, bi);
			break;

		case BI_RAMDISK:
			bootinfo_add_initrd(bid, bi);
			break;

		default:
			break;
		}
	}

	/* Set bootinfo_end to be just past the BI_LAST record. */
	bid->bootinfo_end = (vaddr_t)bootinfo_next(bi);

	/* Initialize the physmem variable for the memory found. */
	RELOC(physmem, int) = bid->bootinfo_total_mem_pages;

	return bid->bootinfo_end;
}

/*
 * bootinfo_startup2 --
 *	Phase 2 of parsing the boot info during early start-up.  This
 *	time around, the MMU is enabled.
 *
 *	The "nextpa" argument is the address of the next page after
 *	the pre-MMU bootstrap memory allocations.
 */
void
bootinfo_startup2(paddr_t nextpa, paddr_t reloff)
{
	struct bootinfo_data *bid = &bootinfo_data_store;
	struct bi_record *bi;

	/* Re-initialize these to the virtual addresses. */
	bid->bootinfo = (struct bi_record *)PA_TO_VA(bid->bootinfo);
	bid->bootinfo_mem_segments = bid_mem_segments;
	bid->bootinfo_mem_segments_avail = bid_mem_segments_avail;

	for (bi = bid->bootinfo; bi->bi_tag != BI_LAST;
	     bi = bootinfo_next(bi)) {
		switch (bi->bi_tag) {
		case BI_VIRT_GF_TTY_BASE:
			bootinfo_gf_tty_consinit(bid, bi);
			break;

		default:
			break;
		}
	}

	/*
	 * Scoot the start of available forward to account for:
	 *
	 *	(1) The kernel text, data, and bss.
	 *
	 *	(2) The pages consumed by pmap bootstrap.
	 *
	 * XXX Assumes these come from the first memory segment.
	 */
	bid->bootinfo_mem_segments_avail[0].mem_size -=
	    nextpa - bid->bootinfo_mem_segments_avail[0].mem_addr;
	bid->bootinfo_mem_segments_avail[0].mem_addr = nextpa;

	/*
	 * Initialize avail_start and avail_end.
	 * XXX Assumes segments sorted in ascending address order.
	 * XXX Legacy nonsense that should go away.
	 */
	extern paddr_t avail_start, avail_end;
	int i = bid->bootinfo_mem_nsegments - 1;
	avail_start = bid->bootinfo_mem_segments_avail[0].mem_addr;
	avail_end   = bid->bootinfo_mem_segments_avail[i].mem_addr +
		      bid->bootinfo_mem_segments_avail[i].mem_size;

	/*
	 * If we have a RAM disk, we need to take it out of the
	 * available memory segments.
	 */
	bootinfo_reserve_initrd(bid);
}

/*
 * bootinfo_data --
 *	Return a pointer to the bootinfo_data.
 */
struct bootinfo_data *
bootinfo_data(void)
{
	return &bootinfo_data_store;
}

/*
 * bootinfo_enumerate --
 *	Enumerate through the boot info, invoking the specified callback
 *	for each record.  The callback returns true to keep searching,
 *	false, to stop.
 */
void
bootinfo_enumerate(bool (*cb)(struct bi_record *, void *), void *ctx)
{
	struct bi_record *bi = bootinfo_data_store.bootinfo;

	if (bi == NULL) {
		return;
	}

	for (; bi->bi_tag != BI_LAST; bi = bootinfo_next(bi)) {
		if ((*cb)(bi, ctx) == false) {
			break;
		}
	}
}

struct bootinfo_find_ctx {
	uint32_t tag;
	struct bi_record *result;
};

static bool
bootinfo_find_cb(struct bi_record *bi, void *v)
{
	struct bootinfo_find_ctx *ctx = v;

	if (bi->bi_tag == ctx->tag) {
		ctx->result = bi;
		return false;
	}

	return true;
}

/*
 * bootinfo_find --
 *	Scan through the boot info looking for the first instance of
 *	the specified tag.
 */
struct bi_record *
bootinfo_find(uint32_t tag)
{
	struct bootinfo_find_ctx ctx = {
		.tag = tag,
	};

	bootinfo_enumerate(bootinfo_find_cb, &ctx);
	return ctx.result;
}

/*
 * bootinfo_addr_is_console --
 *	Tests to see if the device at the specified address is
 *	the console device.
 */
bool
bootinfo_addr_is_console(paddr_t pa)
{
	struct bootinfo_data *bid = &bootinfo_data_store;

	return bid->bootinfo_console_addr_valid &&
	       bid->bootinfo_console_addr == pa;
}

/*
 * bootinfo_setup_initrd --
 *	Check for a BI_RAMDISK record and, if found, set it as
 *	the root file system.
 */
void
bootinfo_setup_initrd(void)
{
#ifdef MEMORY_DISK_DYNAMIC
	struct bootinfo_data *bid = &bootinfo_data_store;

	if (bid->bootinfo_initrd_size != 0) {
		paddr_t rdstart, rdend, rdpgoff;
		vaddr_t rdva, rdoff;
		vsize_t rdvsize;

		printf("Initializing root RAM disk @ %p - %p\n",
		    (void *)bid->bootinfo_initrd_start,
		    (void *)(bid->bootinfo_initrd_start +
			     bid->bootinfo_initrd_size - 1));

		rdend = m68k_round_page(bid->bootinfo_initrd_start +
		    bid->bootinfo_initrd_size);
		rdstart = m68k_trunc_page(bid->bootinfo_initrd_start);
		rdvsize = rdend - rdstart;
		rdpgoff = bid->bootinfo_initrd_start & PAGE_MASK;

		rdva = uvm_km_alloc(kernel_map, rdvsize, PAGE_SIZE,
		    UVM_KMF_VAONLY);
		if (rdva == 0) {
			printf("WARNING: Unable to allocate KVA for "
			       "RAM disk.\n");
			return;
		}
		for (rdoff = 0; rdoff < rdvsize; rdoff += PAGE_SIZE) {
			pmap_kenter_pa(rdva + rdoff, rdstart + rdoff,
			    VM_PROT_READ | VM_PROT_WRITE, 0);
		}
		md_root_setconf((void *)(rdva + rdpgoff),
		    bid->bootinfo_initrd_size);
	}
#endif /* MEMORY_DISK_DYNAMIC */
}

/*
 * bootinfo_setup_rndseed --
 *	Check for a BI_RNG_SEED record and, if found, use it to
 *	seed the kernel entropy pool.
 */
void
bootinfo_setup_rndseed(void)
{
	static struct krndsource bootinfo_rndsource;
	struct bi_record *bi = bootinfo_find(BI_RNG_SEED);
	if (bi != NULL) {
		struct bi_data *rnd = bootinfo_dataptr(bi);
		rnd_attach_source(&bootinfo_rndsource, "bootinfo",
		    RND_TYPE_RNG, RND_FLAG_DEFAULT);
		rnd_add_data(&bootinfo_rndsource,
		    rnd->data_bytes, rnd->data_length,
		    rnd->data_length * NBBY);
		explicit_memset(rnd->data_bytes, 0, rnd->data_length);
	}
}

/*
 * bootinfo_getarg --
 *	Get an argument from the BI_COMMAND_LINE bootinfo record.
 */
bool
bootinfo_getarg(const char *var, char *buf, size_t buflen)
{
	const size_t varlen = strlen(var);
	struct bi_record *bi = bootinfo_find(BI_COMMAND_LINE);

	if (bi == NULL) {
		return false;
	}

	const char *sp = bootinfo_dataptr(bi);
	const char *osp = sp;
	for (;;) {
		sp = strstr(sp, var);
		if (sp == NULL) {
			return false;
		}

		if (sp != osp &&
		    sp[-1] != ' ' && sp[-1] != '\t' && sp[-1] != '-') {
			continue;
		}
		sp += varlen;
		char ch = *sp++;
		if (ch != '=' && ch != ' ' && ch != '\t' && ch != '\0') {
			continue;
		}
		/* Found it. */
		break;
	}

	while (--buflen) {
		if (*sp == ' ' || *sp == '\t' || *sp == '\0') {
			break;
		}
		*buf++ = *sp++;
	}
	*buf = '\0';

	return true;
}
