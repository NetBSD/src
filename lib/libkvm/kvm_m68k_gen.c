/*	$NetBSD: kvm_m68k_gen.c,v 1.2 2025/12/24 20:37:04 andvar Exp $	*/

/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: kvm_m68k_gen.c,v 1.2 2025/12/24 20:37:04 andvar Exp $");
#endif /* LIBC_SCCS and not lint */

/*
 * Generic m68k machine dependent routines for kvm.
 *
 * Note: This file has to build on ALL m68k machines,
 * so do NOT include any <machine / *.h> files here.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/kcore.h>

#include <unistd.h>
#include <limits.h>
#include <kvm.h>
#include <stdlib.h>

#include <m68k/cpu.h>
#include <m68k/kcore.h>
#include <m68k/mmu_51.h>
#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>

#include "kvm_private.h"
#include "kvm_m68k.h"

static int	_kvm_gen68k_initvtop(kvm_t *);
static void	_kvm_gen68k_freevtop(kvm_t *);
static int	_kvm_gen68k_kvatop(kvm_t *, vaddr_t, paddr_t *);
static off_t	_kvm_gen68k_pa2off(kvm_t *, u_long);

struct kvm_ops _kvm_ops_gen68k = {
	_kvm_gen68k_initvtop,
	_kvm_gen68k_freevtop,
	_kvm_gen68k_kvatop,
	_kvm_gen68k_pa2off
};

/*
 * For the table indices, shift first, then mask.  The ti_pa_mask
 * is computed from the number of entries the table indexes, which
 * in turn defines how precise the address field in each table
 * entry.
 */
struct table_index {
	uint32_t	ti_shift;
	uint32_t	ti_mask;
	uint32_t	ti_pa_mask;
};

#define	TABLE_INDEX_COUNT	4

struct kvm_gen68k_context {
	uint32_t	pg_frame;	/* page frame computed from mask */
	uint32_t	va_mask;	/* VA significant bits from IS */

	struct table_index ti[TABLE_INDEX_COUNT];
};

static inline uint32_t
table_entry_paddr(uint32_t table_pa, unsigned int idx)
{
	return table_pa + (idx * sizeof(uint32_t));
}

static int
read_table_entry(kvm_t *kd, int level, paddr_t table_pa, unsigned int idx,
    uint32_t *entry)
{
	uint32_t entry_pa = table_entry_paddr(table_pa, idx);

	if (_kvm_pread(kd, kd->pmfd, entry, sizeof(*entry),
		       _kvm_gen68k_pa2off(kd, entry_pa)) != sizeof(*entry)) {
		_kvm_err(kd, 0,
		    "failed to read level-%c entry @ pa=0x%08x",
		    'A' + level, entry_pa);
		return -1;
	}
	return 0;
}

static inline unsigned int
table_index(const struct kvm_gen68k_context * const ctx, int level,
    uint32_t va)
{
	return (va >> ctx->ti[level].ti_shift) & ctx->ti[level].ti_mask;
}

static inline uint32_t
table_entry_extract_pa(const struct kvm_gen68k_context * const ctx, int level,
    uint32_t entry)
{
	return entry & ctx->ti[level].ti_pa_mask;
}

static int
mmu_tree_walk(kvm_t *kd, uint32_t va, uint32_t *pap)
{
	const cpu_kcore_hdr_t * const h = kd->cpu_data;
	const struct gen68k_kcore_hdr * const gen = &h->un._gen68k;
	const struct vmstate * const vmst = kd->vmst;
	const struct kvm_gen68k_context * const ctx = vmst->private;
	uint32_t entry, type, table_pa;
	uint32_t adj_va = va & ctx->va_mask;
	unsigned int idx;
	int level;

	/* We only handle SHORT descriptors. */
	if (__SHIFTOUT(gen->srp[0], DT51_TYPE) != DT51_SHORT) {
		_kvm_err(kd, 0,
		    "SRP does not point to a SHORT descriptor");
		return -1;
	}

	table_pa = gen->srp[1];
	for (level = 0; level < TABLE_INDEX_COUNT; level++) {
		idx = table_index(ctx, level, adj_va);
		if (read_table_entry(kd, level, table_pa, idx, &entry) < 0) {
			return -1;
		}
		type = __SHIFTOUT(entry, DT51_TYPE);
		switch (type) {
		case DT51_INVALID:
			_kvm_err(kd, 0,
			    "INVALID level-%c entry @ table_pa=0x%08x idx=%d",
			    'A' + level, table_pa, idx);
			return -1;

		case DT51_PAGE:
			*pap = entry & ctx->pg_frame;
			return 0;

		case DT51_SHORT:
			table_pa = table_entry_extract_pa(ctx, level, entry);
			break;

		case DT51_LONG:
		default:
			/*
			 * Our kernel doesn't use DT51_LONG, but if it
			 * did, we could read the value at entry_pa+4
			 * to get the 32-bit Table Physical Address field.
			 */
			_kvm_err(kd, 0,
			    "LONG level-%c entry @ table_pa=0x%08x idx=%d",
			    'A' + level, table_pa, idx);
			return -1;
		}
	}
	_kvm_err(kd, 0,
	    "exhausted %d levels for kva=0x%08x\n",
	    TABLE_INDEX_COUNT, va);
	return -1;
}

static int
_kvm_gen68k_initvtop(kvm_t *kd)
{
	struct vmstate *vmst = kd->vmst;

	struct kvm_gen68k_context *ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		goto bad;
	}

	cpu_kcore_hdr_t *h = kd->cpu_data;
	struct gen68k_kcore_hdr *gen = &h->un._gen68k;

	vmst->pgshift = __SHIFTOUT(gen->tcr, TCR51_PS);
	if (vmst->pgshift < 8 || vmst->pgshift > 15) {
		/* Invalid PS field in TCR. */
		goto bad;
	}
	vmst->pgofset = (1U << vmst->pgshift) - 1;
	ctx->pg_frame = ~vmst->pgofset;

	unsigned int ti[TABLE_INDEX_COUNT];
	unsigned int is  = __SHIFTOUT(gen->tcr, TCR51_IS);
	ti[0] = __SHIFTOUT(gen->tcr, TCR51_TIA);
	ti[1] = __SHIFTOUT(gen->tcr, TCR51_TIB);
	ti[2] = __SHIFTOUT(gen->tcr, TCR51_TIC);
	ti[3] = __SHIFTOUT(gen->tcr, TCR51_TID);

	/* All of the shifts should add up to 32. */
	if (is + ti[0] + ti[1] + ti[2] + ti[3] + vmst->pgshift != 32) {
		/* This must add up to 32. */
		goto bad;
	}
	if (is > 15 || ti[0] > 15 || ti[1] > 15 || ti[2] > 15 || ti[3] > 15) {
		/* None of these values may exceed 15. */
		goto bad;
	}

	/*
	 * The MMU works by successively shifting the address to the
	 * left and using a mux to take the next field from bits 31-N
	 * as indicated by the TCR.  The initial shift is intended to
	 * lop off the upper N bits of the address, presumably so that
	 * software can use them for some other purpose (aaah, how fondly
	 * we remember the 24-bit addresses of the 68000/68010).  We sort
	 * of work successively the other direction because it's more
	 * convenient for software to do it that way, but we do need to
	 * pre-compute some shifts and masks first.
	 */

	ctx->va_mask = 0xffffffffU;
	int curshift = 32;
	unsigned int count;
	if (is) {
		ctx->va_mask = (ctx->va_mask << is) >> is;
		curshift -= is;
	}
	for (int i = 0; i < TABLE_INDEX_COUNT; i++) {
		/* MMU stops processing once it gets to a TIx == 0. */
		if (ti[i] == 0) {
			break;
		}
		curshift -= ti[i];
		count = 1U << ti[i];
		ctx->ti[i].ti_shift = curshift;
		ctx->ti[i].ti_mask = (count - 1);
		/*
		 * XXX Doesn't deal with entries that point to long
		 * XXX descriptors.
		 */
		ctx->ti[i].ti_pa_mask = 0xffffffffU &
		    ~((count * sizeof(uint32_t)) - 1);
	}

	/* At this point, what remains should be pgshift. */
	if (curshift != vmst->pgshift) {
		goto bad;
	}

	vmst->private = ctx;
	return 0;

 bad:
	if (ctx != NULL) {
		free(ctx);
	}
	return -1;
}

static void
_kvm_gen68k_freevtop(kvm_t *kd)
{
	struct kvm_gen68k_context *ctx = kd->vmst->private;
	kd->vmst->private = NULL;

	free(ctx);
}

static int
_kvm_gen68k_kvatop(kvm_t *kd, vaddr_t va, paddr_t *pa)
{
	cpu_kcore_hdr_t *h = kd->cpu_data;
	const struct gen68k_kcore_hdr * const gen = &h->un._gen68k;
	const struct vmstate * const vmst = kd->vmst;
	const struct kvm_gen68k_context * const ctx = vmst->private;
	uint32_t trans_pa;

	if (ISALIVE(kd)) {
		_kvm_err(kd, 0, "vatop called in live kernel!");
		return 0;
	}

	vaddr_t offset = va & vmst->pgofset;
	va &= ctx->pg_frame;

	/*
	 * We may be called before address translation is initialized.
	 * This is typically used to find the dump magic number.  This
	 * means we do not yet have the kernel page tables available,
	 * so we must to a simple relocation.
	 */
	if (va >= h->kernbase && va < gen->relocend) {
		trans_pa = (va - h->kernbase) + gen->reloc;
		goto out;
	}

	if (mmu_tree_walk(kd, va, &trans_pa) < 0) {
		/* _kvm_err() already set. */
		return 0;
	}
 out:
	*pa = trans_pa + offset;
	return h->page_size - offset;
}

/*
 * Translate a physical address to a file-offset in the crash dump.
 */
static off_t
_kvm_gen68k_pa2off(kvm_t *kd, u_long pa)
{
	cpu_kcore_hdr_t *h = kd->cpu_data;
	struct gen68k_kcore_hdr *gen = &h->un._gen68k;
	phys_ram_seg_t *rsp = gen->ram_segs;
	off_t off = 0;
	int i;

	for (i = 0; i < GEN68K_NPHYS_RAM_SEGS && rsp[i].size != 0; i++) {
		if (pa >= rsp[i].start &&
		    pa < (rsp[i].start + rsp[i].size)) {
			pa -= rsp[i].start;
			break;
		}
		off += rsp[i].size;
	}
	return (kd->dump_off + off + pa);
}
