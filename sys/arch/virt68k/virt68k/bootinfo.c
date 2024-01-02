/*      $NetBSD: bootinfo.c,v 1.1 2024/01/02 07:41:02 thorpej Exp $        */      

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bootinfo.c,v 1.1 2024/01/02 07:41:02 thorpej Exp $");

#include <sys/types.h>
#include <sys/cpu.h>

#include <machine/bootinfo.h>
#include <machine/vmparam.h>

#include "gftty.h"
#if NGFTTY > 0
#include <dev/goldfish/gfttyvar.h>
#endif

struct bi_record *	bootinfo;
vaddr_t			bootinfo_end;
uint32_t		bootinfo_machtype;
int			bootinfo_mem_segments_ignored;
size_t			bootinfo_mem_segments_ignored_bytes;
struct bi_mem_info	bootinfo_mem_segments[VM_PHYSSEG_MAX];
struct bi_mem_info	bootinfo_mem_segments_avail[VM_PHYSSEG_MAX];
int			bootinfo_mem_nsegments;

static paddr_t		bootinfo_console_addr;
static bool		bootinfo_console_addr_valid;

static bool
bootinfo_set_console(paddr_t pa)
{
	if (! bootinfo_console_addr_valid) {
		bootinfo_console_addr = pa;
		bootinfo_console_addr_valid = true;
		return true;
	}
	return false;
}

static inline struct bi_record *
bootinfo_next(struct bi_record *bi)
{
	uintptr_t addr = (uintptr_t)bi;

	addr += bi->bi_size;
	return (struct bi_record *)addr;
}

static inline int
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

static inline int
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

static inline int
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

static inline void
bootinfo_add_mem(struct bi_record *bi)
{
	struct bi_mem_info *m = bootinfo_dataptr(bi);

	if (bootinfo_mem_nsegments == VM_PHYSSEG_MAX) {
		bootinfo_mem_segments_ignored++;
		bootinfo_mem_segments_ignored_bytes += m->mem_size;
	}

	/*
	 * Make sure the start / size are properly aligned.
	 */
	if (m->mem_addr & PGOFSET) {
		m->mem_size -= m->mem_addr & PGOFSET;
		m->mem_addr = m68k_round_page(m->mem_addr);
	}
	m->mem_size = m68k_trunc_page(m->mem_size);
	physmem += m->mem_size >> PGSHIFT;

	bootinfo_mem_segments[bootinfo_mem_nsegments] = *m;
	bootinfo_mem_segments_avail[bootinfo_mem_nsegments] = *m;
	bootinfo_mem_nsegments++;
}

static inline void
bootinfo_gf_tty_consinit(struct bi_record *bi)
{
#if NGFTTY > 0
	struct bi_virt_dev *vd = bootinfo_dataptr(bi);

	/*
	 * vd_mmio_base is the PA, but we're going to run mapped
	 * VA==PA for devices anyway once the MMU is turned on.
	 */
	if (bootinfo_set_console(vd->vd_mmio_base)) {
		bootinfo_md_cnattach(gftty_cnattach,
		    vd->vd_mmio_base, 0x1000);
		printf("Initialized Goldfish TTY console @ 0x%08x\n",
		    vd->vd_mmio_base);
	}
#endif /* NGFTTY > 0 */
}

/*
 * bootinfo_start --
 *	Parse the boot info during early start-up.
 */
void
bootinfo_start(struct bi_record *first)
{
	struct bi_record *bi;

	bootinfo = first;

	for (bi = bootinfo; bi->bi_tag != BI_LAST; bi = bootinfo_next(bi)) {
		switch (bi->bi_tag) {
		case BI_MACHTYPE:
			bootinfo_machtype = bootinfo_get_u32(bi);
			break;

		case BI_CPUTYPE:
			cputype = bootinfo_get_cpu(bi);
			break;

		case BI_FPUTYPE:
			fputype = bootinfo_get_fpu(bi);
			break;

		case BI_MMUTYPE:
			mmutype = bootinfo_get_mmu(bi);
			break;

		case BI_MEMCHUNK:
			bootinfo_add_mem(bi);
			break;

		case BI_VIRT_GF_TTY_BASE:
			bootinfo_gf_tty_consinit(bi);
			break;

		default:
			break;
		}
	}

	/* Set bootinfo_end to be just past the BI_LAST record. */
	bootinfo_end = (vaddr_t)bootinfo_next(bi);
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
	struct bi_record *bi = bootinfo;

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
	return bootinfo_console_addr_valid && bootinfo_console_addr == pa;
}
