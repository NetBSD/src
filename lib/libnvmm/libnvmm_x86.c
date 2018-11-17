/*	$NetBSD: libnvmm_x86.c,v 1.4 2018/11/17 16:11:33 maxv Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <machine/vmparam.h>
#include <machine/pte.h>
#include <machine/psl.h>

#include "nvmm.h"

#include <x86/specialreg.h>

/* -------------------------------------------------------------------------- */

#define PTE32_L1_SHIFT	12
#define PTE32_L2_SHIFT	22

#define PTE32_L2_MASK	0xffc00000
#define PTE32_L1_MASK	0x003ff000

#define PTE32_L2_FRAME	(PTE32_L2_MASK)
#define PTE32_L1_FRAME	(PTE32_L2_FRAME|PTE32_L1_MASK)

#define pte32_l1idx(va)	(((va) & PTE32_L1_MASK) >> PTE32_L1_SHIFT)
#define pte32_l2idx(va)	(((va) & PTE32_L2_MASK) >> PTE32_L2_SHIFT)

typedef uint32_t pte_32bit_t;

static int
x86_gva_to_gpa_32bit(struct nvmm_machine *mach, uint64_t cr3,
    gvaddr_t gva, gpaddr_t *gpa, bool has_pse, nvmm_prot_t *prot)
{
	gpaddr_t L2gpa, L1gpa;
	uintptr_t L2hva, L1hva;
	pte_32bit_t *pdir, pte;

	/* We begin with an RWXU access. */
	*prot = NVMM_PROT_ALL;

	/* Parse L2. */
	L2gpa = (cr3 & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L2gpa, &L2hva) == -1)
		return -1;
	pdir = (pte_32bit_t *)L2hva;
	pte = pdir[pte32_l2idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if ((pte & PG_PS) && !has_pse)
		return -1;
	if (pte & PG_PS) {
		*gpa = (pte & PTE32_L2_FRAME);
		return 0;
	}

	/* Parse L1. */
	L1gpa = (pte & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L1gpa, &L1hva) == -1)
		return -1;
	pdir = (pte_32bit_t *)L1hva;
	pte = pdir[pte32_l1idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_PS)
		return -1;

	*gpa = (pte & PG_FRAME);
	return 0;
}

/* -------------------------------------------------------------------------- */

#define	PTE32_PAE_L1_SHIFT	12
#define	PTE32_PAE_L2_SHIFT	21
#define	PTE32_PAE_L3_SHIFT	30

#define	PTE32_PAE_L3_MASK	0xc0000000
#define	PTE32_PAE_L2_MASK	0x3fe00000
#define	PTE32_PAE_L1_MASK	0x001ff000

#define	PTE32_PAE_L3_FRAME	(PTE32_PAE_L3_MASK)
#define	PTE32_PAE_L2_FRAME	(PTE32_PAE_L3_FRAME|PTE32_PAE_L2_MASK)
#define	PTE32_PAE_L1_FRAME	(PTE32_PAE_L2_FRAME|PTE32_PAE_L1_MASK)

#define pte32_pae_l1idx(va)	(((va) & PTE32_PAE_L1_MASK) >> PTE32_PAE_L1_SHIFT)
#define pte32_pae_l2idx(va)	(((va) & PTE32_PAE_L2_MASK) >> PTE32_PAE_L2_SHIFT)
#define pte32_pae_l3idx(va)	(((va) & PTE32_PAE_L3_MASK) >> PTE32_PAE_L3_SHIFT)

typedef uint64_t pte_32bit_pae_t;

static int
x86_gva_to_gpa_32bit_pae(struct nvmm_machine *mach, uint64_t cr3,
    gvaddr_t gva, gpaddr_t *gpa, bool has_pse, nvmm_prot_t *prot)
{
	gpaddr_t L3gpa, L2gpa, L1gpa;
	uintptr_t L3hva, L2hva, L1hva;
	pte_32bit_pae_t *pdir, pte;

	/* We begin with an RWXU access. */
	*prot = NVMM_PROT_ALL;

	/* Parse L3. */
	L3gpa = (cr3 & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L3gpa, &L3hva) == -1)
		return -1;
	pdir = (pte_32bit_pae_t *)L3hva;
	pte = pdir[pte32_pae_l3idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if (pte & PG_PS)
		return -1;

	/* Parse L2. */
	L2gpa = (pte & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L2gpa, &L2hva) == -1)
		return -1;
	pdir = (pte_32bit_pae_t *)L2hva;
	pte = pdir[pte32_pae_l2idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if ((pte & PG_PS) && !has_pse)
		return -1;
	if (pte & PG_PS) {
		*gpa = (pte & PTE32_PAE_L2_FRAME);
		return 0;
	}

	/* Parse L1. */
	L1gpa = (pte & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L1gpa, &L1hva) == -1)
		return -1;
	pdir = (pte_32bit_pae_t *)L1hva;
	pte = pdir[pte32_pae_l1idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if (pte & PG_PS)
		return -1;

	*gpa = (pte & PG_FRAME);
	return 0;
}

/* -------------------------------------------------------------------------- */

#define PTE64_L1_SHIFT	12
#define PTE64_L2_SHIFT	21
#define PTE64_L3_SHIFT	30
#define PTE64_L4_SHIFT	39

#define PTE64_L4_MASK	0x0000ff8000000000
#define PTE64_L3_MASK	0x0000007fc0000000
#define PTE64_L2_MASK	0x000000003fe00000
#define PTE64_L1_MASK	0x00000000001ff000

#define PTE64_L4_FRAME	PTE64_L4_MASK
#define PTE64_L3_FRAME	(PTE64_L4_FRAME|PTE64_L3_MASK)
#define PTE64_L2_FRAME	(PTE64_L3_FRAME|PTE64_L2_MASK)
#define PTE64_L1_FRAME	(PTE64_L2_FRAME|PTE64_L1_MASK)

#define pte64_l1idx(va)	(((va) & PTE64_L1_MASK) >> PTE64_L1_SHIFT)
#define pte64_l2idx(va)	(((va) & PTE64_L2_MASK) >> PTE64_L2_SHIFT)
#define pte64_l3idx(va)	(((va) & PTE64_L3_MASK) >> PTE64_L3_SHIFT)
#define pte64_l4idx(va)	(((va) & PTE64_L4_MASK) >> PTE64_L4_SHIFT)

typedef uint64_t pte_64bit_t;

static inline bool
x86_gva_64bit_canonical(gvaddr_t gva)
{
	/* Bits 63:47 must have the same value. */
#define SIGN_EXTEND	0xffff800000000000ULL
	return (gva & SIGN_EXTEND) == 0 || (gva & SIGN_EXTEND) == SIGN_EXTEND;
}

static int
x86_gva_to_gpa_64bit(struct nvmm_machine *mach, uint64_t cr3,
    gvaddr_t gva, gpaddr_t *gpa, bool has_pse, nvmm_prot_t *prot)
{
	gpaddr_t L4gpa, L3gpa, L2gpa, L1gpa;
	uintptr_t L4hva, L3hva, L2hva, L1hva;
	pte_64bit_t *pdir, pte;

	/* We begin with an RWXU access. */
	*prot = NVMM_PROT_ALL;

	if (!x86_gva_64bit_canonical(gva))
		return -1;

	/* Parse L4. */
	L4gpa = (cr3 & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L4gpa, &L4hva) == -1)
		return -1;
	pdir = (pte_64bit_t *)L4hva;
	pte = pdir[pte64_l4idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if (pte & PG_PS)
		return -1;

	/* Parse L3. */
	L3gpa = (pte & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L3gpa, &L3hva) == -1)
		return -1;
	pdir = (pte_64bit_t *)L3hva;
	pte = pdir[pte64_l3idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if ((pte & PG_PS) && !has_pse)
		return -1;
	if (pte & PG_PS) {
		*gpa = (pte & PTE64_L3_FRAME);
		return 0;
	}

	/* Parse L2. */
	L2gpa = (pte & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L2gpa, &L2hva) == -1)
		return -1;
	pdir = (pte_64bit_t *)L2hva;
	pte = pdir[pte64_l2idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if ((pte & PG_PS) && !has_pse)
		return -1;
	if (pte & PG_PS) {
		*gpa = (pte & PTE64_L2_FRAME);
		return 0;
	}

	/* Parse L1. */
	L1gpa = (pte & PG_FRAME);
	if (nvmm_gpa_to_hva(mach, L1gpa, &L1hva) == -1)
		return -1;
	pdir = (pte_64bit_t *)L1hva;
	pte = pdir[pte64_l1idx(gva)];
	if ((pte & PG_V) == 0)
		return -1;
	if ((pte & PG_u) == 0)
		*prot &= ~NVMM_PROT_USER;
	if ((pte & PG_KW) == 0)
		*prot &= ~NVMM_PROT_WRITE;
	if (pte & PG_NX)
		*prot &= ~NVMM_PROT_EXEC;
	if (pte & PG_PS)
		return -1;

	*gpa = (pte & PG_FRAME);
	return 0;
}

static inline int
x86_gva_to_gpa(struct nvmm_machine *mach, struct nvmm_x64_state *state,
    gvaddr_t gva, gpaddr_t *gpa, nvmm_prot_t *prot)
{
	bool is_pae, is_lng, has_pse;
	uint64_t cr3;
	int ret;

	if ((state->crs[NVMM_X64_CR_CR0] & CR0_PG) == 0) {
		/* No paging. */
		*prot = NVMM_PROT_ALL;
		*gpa = gva;
		return 0;
	}

	is_pae = (state->crs[NVMM_X64_CR_CR4] & CR4_PAE) != 0;
	is_lng = (state->msrs[NVMM_X64_MSR_EFER] & EFER_LME) != 0;
	has_pse = (state->crs[NVMM_X64_CR_CR4] & CR4_PSE) != 0;
	cr3 = state->crs[NVMM_X64_CR_CR3];

	if (is_pae && is_lng) {
		/* 64bit */
		ret = x86_gva_to_gpa_64bit(mach, cr3, gva, gpa, has_pse, prot);
	} else if (is_pae && !is_lng) {
		/* 32bit PAE */
		ret = x86_gva_to_gpa_32bit_pae(mach, cr3, gva, gpa, has_pse,
		    prot);
	} else if (!is_pae && !is_lng) {
		/* 32bit */
		ret = x86_gva_to_gpa_32bit(mach, cr3, gva, gpa, has_pse, prot);
	} else {
		ret = -1;
	}

	if (ret == -1) {
		errno = EFAULT;
	}

	return ret;
}

int
nvmm_gva_to_gpa(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    gvaddr_t gva, gpaddr_t *gpa, nvmm_prot_t *prot)
{
	struct nvmm_x64_state state;
	int ret;

	if (gva & PAGE_MASK) {
		errno = EINVAL;
		return -1;
	}

	ret = nvmm_vcpu_getstate(mach, cpuid, &state,
	    NVMM_X64_STATE_CRS | NVMM_X64_STATE_MSRS);
	if (ret == -1)
		return -1;

	return x86_gva_to_gpa(mach, &state, gva, gpa, prot);
}

/* -------------------------------------------------------------------------- */

static inline bool
is_long_mode(struct nvmm_x64_state *state)
{
	return (state->msrs[NVMM_X64_MSR_EFER] & EFER_LME) != 0;
}

static inline bool
is_illegal(struct nvmm_io *io, nvmm_prot_t prot)
{
	return (io->in && !(prot & NVMM_PROT_WRITE));
}

static int
segment_apply(struct nvmm_x64_state_seg *seg, gvaddr_t *gva, size_t size)
{
	uint64_t limit;

	/*
	 * This is incomplete. We should check topdown, etc, really that's
	 * tiring.
	 */
	if (__predict_false(!seg->attrib.p)) {
		goto error;
	}

	limit = (seg->limit + 1);
	if (__predict_true(seg->attrib.gran)) {
		limit *= PAGE_SIZE;
	}

	if (__predict_false(*gva + seg->base + size > limit)) {
		goto error;
	}

	*gva += seg->base;
	return 0;

error:
	errno = EFAULT;
	return -1;
}

int
nvmm_assist_io(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_exit *exit, void (*cb)(struct nvmm_io *))
{
	struct nvmm_x64_state state;
	struct nvmm_io io;
	nvmm_prot_t prot;
	size_t remain, done;
	uintptr_t hva;
	gvaddr_t gva, off;
	gpaddr_t gpa;
	uint64_t rsi;
	uint8_t tmp[8];
	uint8_t *ptr, *ptr2;
	bool cross;
	int ret;

	if (__predict_false(exit->reason != NVMM_EXIT_IO)) {
		errno = EINVAL;
		return -1;
	}

	io.port = exit->u.io.port;
	io.in = (exit->u.io.type == NVMM_EXIT_IO_IN);
	io.size = exit->u.io.operand_size;

	ret = nvmm_vcpu_getstate(mach, cpuid, &state,
	    NVMM_X64_STATE_GPRS | NVMM_X64_STATE_SEGS |
	    NVMM_X64_STATE_CRS | NVMM_X64_STATE_MSRS);
	if (ret == -1)
		return -1;

	cross = false;

	if (!exit->u.io.str) {
		ptr = (uint8_t *)&state.gprs[NVMM_X64_GPR_RAX];
	} else {
		rsi = state.gprs[NVMM_X64_GPR_RSI];

		switch (exit->u.io.address_size) {
		case 8:
			gva = rsi;
			break;
		case 4:
			gva = (rsi & 0x00000000FFFFFFFF);
			break;
		case 2:
		default: /* impossible */
			gva = (rsi & 0x000000000000FFFF);
			break;
		}

		if (!is_long_mode(&state)) {
			ret = segment_apply(&state.segs[exit->u.io.seg], &gva,
			    io.size);
			if (ret == -1)
				return -1;
		}

		off = (gva & PAGE_MASK);
		gva &= ~PAGE_MASK;

		ret = x86_gva_to_gpa(mach, &state, gva, &gpa, &prot);
		if (ret == -1)
			return -1;
		if (__predict_false(is_illegal(&io, prot))) {
			errno = EFAULT;
			return -1;
		}
		ret = nvmm_gpa_to_hva(mach, gpa, &hva);
		if (ret == -1)
			return -1;

		ptr = (uint8_t *)hva + off;

		/*
		 * Special case. If the buffer is in between two pages, we
		 * need to retrieve data from the next page.
		 */
		if (__predict_false(off + io.size > PAGE_SIZE)) {
			cross = true;
			remain = off + io.size - PAGE_SIZE;
			done = PAGE_SIZE - off;

			memcpy(tmp, ptr, done);

			ret = x86_gva_to_gpa(mach, &state, gva + PAGE_SIZE,
			    &gpa, &prot);
			if (ret == -1)
				return -1;
			if (__predict_false(is_illegal(&io, prot))) {
				errno = EFAULT;
				return -1;
			}
			ret = nvmm_gpa_to_hva(mach, gpa, &hva);
			if (ret == -1)
				return -1;

			memcpy(&tmp[done], (uint8_t *)hva, remain);
			ptr2 = &tmp[done];
		}
	}

	if (io.in) {
		/* nothing to do */
	} else {
		memcpy(io.data, ptr, io.size);
	}

	(*cb)(&io);

	if (io.in) {
		if (!exit->u.io.str)
			state.gprs[NVMM_X64_GPR_RAX] = 0;
		if (__predict_false(cross)) {
			memcpy(ptr, io.data, done);
			memcpy(ptr2, &io.data[done], remain);
		} else {
			memcpy(ptr, io.data, io.size);
		}
	} else {
		/* nothing to do */
	}

	if (exit->u.io.rep) {
		state.gprs[NVMM_X64_GPR_RCX] -= 1;
		if (state.gprs[NVMM_X64_GPR_RCX] == 0) {
			state.gprs[NVMM_X64_GPR_RIP] = exit->u.io.npc;
		}
		if (exit->u.io.str) {
			if (state.gprs[NVMM_X64_GPR_RFLAGS] & PSL_D) {
				state.gprs[NVMM_X64_GPR_RSI] -= io.size;
			} else {
				state.gprs[NVMM_X64_GPR_RSI] += io.size;
			}
		}
	} else {
		state.gprs[NVMM_X64_GPR_RIP] = exit->u.io.npc;
	}

	ret = nvmm_vcpu_setstate(mach, cpuid, &state, NVMM_X64_STATE_GPRS);
	if (ret == -1)
		return -1;

	return 0;
}

/* -------------------------------------------------------------------------- */

int
nvmm_assist_mem(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_exit *exit, void (*cb)(struct nvmm_mem *))
{
	if (__predict_false(exit->reason != NVMM_EXIT_MEMORY)) {
		errno = EINVAL;
		return -1;
	}

	// TODO
	errno = ENOSYS;
	return -1;
}
