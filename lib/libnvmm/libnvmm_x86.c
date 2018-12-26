/*	$NetBSD: libnvmm_x86.c,v 1.4.2.3 2018/12/26 14:01:27 pgoyette Exp $	*/

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
is_64bit(struct nvmm_x64_state *state)
{
	return (state->segs[NVMM_X64_SEG_CS].attrib.lng != 0);
}

static inline bool
is_32bit(struct nvmm_x64_state *state)
{
	return (state->segs[NVMM_X64_SEG_CS].attrib.lng == 0) &&
	    (state->segs[NVMM_X64_SEG_CS].attrib.def32 == 1);
}

static inline bool
is_16bit(struct nvmm_x64_state *state)
{
	return (state->segs[NVMM_X64_SEG_CS].attrib.lng == 0) &&
	    (state->segs[NVMM_X64_SEG_CS].attrib.def32 == 0);
}

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
	uint8_t tmp[8];
	uint8_t *ptr, *ptr2;
	int reg = 0; /* GCC */
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
		if (io.in) {
			reg = NVMM_X64_GPR_RDI;
		} else {
			reg = NVMM_X64_GPR_RSI;
		}

		switch (exit->u.io.address_size) {
		case 8:
			gva = state.gprs[reg];
			break;
		case 4:
			gva = (state.gprs[reg] & 0x00000000FFFFFFFF);
			break;
		case 2:
		default: /* impossible */
			gva = (state.gprs[reg] & 0x000000000000FFFF);
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

	if (exit->u.io.str) {
		if (state.gprs[NVMM_X64_GPR_RFLAGS] & PSL_D) {
			state.gprs[reg] -= io.size;
		} else {
			state.gprs[reg] += io.size;
		}
	}

	if (exit->u.io.rep) {
		state.gprs[NVMM_X64_GPR_RCX] -= 1;
		if (state.gprs[NVMM_X64_GPR_RCX] == 0) {
			state.gprs[NVMM_X64_GPR_RIP] = exit->u.io.npc;
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

static void x86_emul_or(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
static void x86_emul_and(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
static void x86_emul_xor(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
static void x86_emul_mov(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
static void x86_emul_stos(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
static void x86_emul_lods(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);

enum x86_legpref {
	/* Group 1 */
	LEG_LOCK = 0,
	LEG_REPN,	/* REPNE/REPNZ */
	LEG_REP,	/* REP/REPE/REPZ */
	/* Group 2 */
	LEG_OVR_CS,
	LEG_OVR_SS,
	LEG_OVR_DS,
	LEG_OVR_ES,
	LEG_OVR_FS,
	LEG_OVR_GS,
	LEG_BRN_TAKEN,
	LEG_BRN_NTAKEN,
	/* Group 3 */
	LEG_OPR_OVR,
	/* Group 4 */
	LEG_ADR_OVR,

	NLEG
};

struct x86_rexpref {
	bool present;
	bool w;
	bool r;
	bool x;
	bool b;
};

struct x86_reg {
	int num;	/* NVMM GPR state index */
	uint64_t mask;
};

enum x86_disp_type {
	DISP_NONE,
	DISP_0,
	DISP_1,
	DISP_4
};

struct x86_disp {
	enum x86_disp_type type;
	uint8_t data[4];
};

enum REGMODRM__Mod {
	MOD_DIS0, /* also, register indirect */
	MOD_DIS1,
	MOD_DIS4,
	MOD_REG
};

enum REGMODRM__Reg {
	REG_000, /* these fields are indexes to the register map */
	REG_001,
	REG_010,
	REG_011,
	REG_100,
	REG_101,
	REG_110,
	REG_111
};

enum REGMODRM__Rm {
	RM_000, /* reg */
	RM_001, /* reg */
	RM_010, /* reg */
	RM_011, /* reg */
	RM_RSP_SIB, /* reg or SIB, depending on the MOD */
	RM_RBP_DISP32, /* reg or displacement-only (= RIP-relative on amd64) */
	RM_110,
	RM_111
};

struct x86_regmodrm {
	bool present;
	enum REGMODRM__Mod mod;
	enum REGMODRM__Reg reg;
	enum REGMODRM__Rm rm;
};

struct x86_immediate {
	size_t size;	/* 1/2/4/8 */
	uint8_t data[8];
};

struct x86_sib {
	uint8_t scale;
	const struct x86_reg *idx;
	const struct x86_reg *bas;
};

enum x86_store_type {
	STORE_NONE,
	STORE_REG,
	STORE_IMM,
	STORE_SIB,
	STORE_DMO
};

struct x86_store {
	enum x86_store_type type;
	union {
		const struct x86_reg *reg;
		struct x86_immediate imm;
		struct x86_sib sib;
		uint64_t dmo;
	} u;
	struct x86_disp disp;
};

struct x86_instr {
	size_t len;
	bool legpref[NLEG];
	struct x86_rexpref rexpref;
	size_t operand_size;
	size_t address_size;

	struct x86_regmodrm regmodrm;

	const struct x86_opcode *opcode;

	struct x86_store src;
	struct x86_store dst;

	struct x86_store *strm;

	void (*emul)(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
};

struct x86_decode_fsm {
	/* vcpu */
	bool is64bit;
	bool is32bit;
	bool is16bit;

	/* fsm */
	int (*fn)(struct x86_decode_fsm *, struct x86_instr *);
	uint8_t *buf;
	uint8_t *end;
};

struct x86_opcode {
	uint8_t byte;
	bool regmodrm;
	bool regtorm;
	bool dmo;
	bool todmo;
	bool stos;
	bool lods;
	bool szoverride;
	int defsize;
	int allsize;
	bool group11;
	bool immediate;
	int immsize;
	int flags;
	void (*emul)(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
};

struct x86_group_entry {
	void (*emul)(struct nvmm_mem *, void (*)(struct nvmm_mem *), uint64_t *);
};

#define OPSIZE_BYTE 0x01
#define OPSIZE_WORD 0x02 /* 2 bytes */
#define OPSIZE_DOUB 0x04 /* 4 bytes */
#define OPSIZE_QUAD 0x08 /* 8 bytes */

#define FLAG_z	0x02

static const struct x86_group_entry group11[8] = {
	[0] = { .emul = x86_emul_mov }
};

static const struct x86_opcode primary_opcode_table[] = {
	/*
	 * Group11
	 */
	{
		.byte = 0xC6,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.group11 = true,
		.immediate = true,
		.immsize = OPSIZE_BYTE,
		.emul = NULL /* group11 */
	},
	{
		.byte = 0xC7,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.group11 = true,
		.immediate = true,
		.immsize = -1, /* special, Z */
		.flags = FLAG_z,
		.emul = NULL /* group11 */
	},

	/*
	 * OR
	 */
	{
		/* Eb, Gb */
		.byte = 0x08,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_or
	},
	{
		/* Ev, Gv */
		.byte = 0x09,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_or
	},
	{
		/* Gb, Eb */
		.byte = 0x0A,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_or
	},
	{
		/* Gv, Ev */
		.byte = 0x0B,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_or
	},

	/*
	 * AND
	 */
	{
		/* Eb, Gb */
		.byte = 0x20,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_and
	},
	{
		/* Ev, Gv */
		.byte = 0x21,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_and
	},
	{
		/* Gb, Eb */
		.byte = 0x22,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_and
	},
	{
		/* Gv, Ev */
		.byte = 0x23,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_and
	},

	/*
	 * XOR
	 */
	{
		/* Eb, Gb */
		.byte = 0x30,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_xor
	},
	{
		/* Ev, Gv */
		.byte = 0x31,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_xor
	},
	{
		/* Gb, Eb */
		.byte = 0x32,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_xor
	},
	{
		/* Gv, Ev */
		.byte = 0x33,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_xor
	},

	/*
	 * MOV
	 */
	{
		/* Eb, Gb */
		.byte = 0x88,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_mov
	},
	{
		/* Ev, Gv */
		.byte = 0x89,
		.regmodrm = true,
		.regtorm = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_mov
	},
	{
		/* Gb, Eb */
		.byte = 0x8A,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_mov
	},
	{
		/* Gv, Ev */
		.byte = 0x8B,
		.regmodrm = true,
		.regtorm = false,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_mov
	},
	{
		/* AL, Ob */
		.byte = 0xA0,
		.dmo = true,
		.todmo = false,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_mov
	},
	{
		/* rAX, Ov */
		.byte = 0xA1,
		.dmo = true,
		.todmo = false,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_mov
	},
	{
		/* Ob, AL */
		.byte = 0xA2,
		.dmo = true,
		.todmo = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_mov
	},
	{
		/* Ov, rAX */
		.byte = 0xA3,
		.dmo = true,
		.todmo = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_mov
	},

	/*
	 * STOS
	 */
	{
		/* Yb, AL */
		.byte = 0xAA,
		.stos = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_stos
	},
	{
		/* Yv, rAX */
		.byte = 0xAB,
		.stos = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_stos
	},

	/*
	 * LODS
	 */
	{
		/* AL, Xb */
		.byte = 0xAC,
		.lods = true,
		.szoverride = false,
		.defsize = OPSIZE_BYTE,
		.allsize = -1,
		.emul = x86_emul_lods
	},
	{
		/* rAX, Xv */
		.byte = 0xAD,
		.lods = true,
		.szoverride = true,
		.defsize = -1,
		.allsize = OPSIZE_WORD|OPSIZE_DOUB|OPSIZE_QUAD,
		.emul = x86_emul_lods
	},
};

static const struct x86_reg gpr_map__rip = { NVMM_X64_GPR_RIP, 0xFFFFFFFFFFFFFFFF };

/* [REX-present][enc][opsize] */
static const struct x86_reg gpr_map__special[2][4][8] = {
	[false] = {
		/* No REX prefix. */
		[0b00] = {
			[0] = { NVMM_X64_GPR_RAX, 0x000000000000FF00 }, /* AH */
			[1] = { NVMM_X64_GPR_RSP, 0x000000000000FFFF }, /* SP */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RSP, 0x00000000FFFFFFFF }, /* ESP */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 },
		},
		[0b01] = {
			[0] = { NVMM_X64_GPR_RCX, 0x000000000000FF00 }, /* CH */
			[1] = { NVMM_X64_GPR_RBP, 0x000000000000FFFF }, /* BP */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RBP, 0x00000000FFFFFFFF },	/* EBP */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 },
		},
		[0b10] = {
			[0] = { NVMM_X64_GPR_RDX, 0x000000000000FF00 }, /* DH */
			[1] = { NVMM_X64_GPR_RSI, 0x000000000000FFFF }, /* SI */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RSI, 0x00000000FFFFFFFF }, /* ESI */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 },
		},
		[0b11] = {
			[0] = { NVMM_X64_GPR_RBX, 0x000000000000FF00 }, /* BH */
			[1] = { NVMM_X64_GPR_RDI, 0x000000000000FFFF }, /* DI */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RDI, 0x00000000FFFFFFFF }, /* EDI */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 },
		}
	},
	[true] = {
		/* Has REX prefix. */
		[0b00] = {
			[0] = { NVMM_X64_GPR_RSP, 0x00000000000000FF }, /* SPL */
			[1] = { NVMM_X64_GPR_RSP, 0x000000000000FFFF }, /* SP */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RSP, 0x00000000FFFFFFFF }, /* ESP */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RSP, 0xFFFFFFFFFFFFFFFF }, /* RSP */
		},
		[0b01] = {
			[0] = { NVMM_X64_GPR_RBP, 0x00000000000000FF }, /* BPL */
			[1] = { NVMM_X64_GPR_RBP, 0x000000000000FFFF }, /* BP */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RBP, 0x00000000FFFFFFFF }, /* EBP */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RBP, 0xFFFFFFFFFFFFFFFF }, /* RBP */
		},
		[0b10] = {
			[0] = { NVMM_X64_GPR_RSI, 0x00000000000000FF }, /* SIL */
			[1] = { NVMM_X64_GPR_RSI, 0x000000000000FFFF }, /* SI */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RSI, 0x00000000FFFFFFFF }, /* ESI */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RSI, 0xFFFFFFFFFFFFFFFF }, /* RSI */
		},
		[0b11] = {
			[0] = { NVMM_X64_GPR_RDI, 0x00000000000000FF }, /* DIL */
			[1] = { NVMM_X64_GPR_RDI, 0x000000000000FFFF }, /* DI */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RDI, 0x00000000FFFFFFFF }, /* EDI */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RDI, 0xFFFFFFFFFFFFFFFF }, /* RDI */
		}
	}
};

/* [depends][enc][size] */
static const struct x86_reg gpr_map[2][8][8] = {
	[false] = {
		/* Not extended. */
		[0b000] = {
			[0] = { NVMM_X64_GPR_RAX, 0x00000000000000FF }, /* AL */
			[1] = { NVMM_X64_GPR_RAX, 0x000000000000FFFF }, /* AX */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RAX, 0x00000000FFFFFFFF }, /* EAX */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RAX, 0x00000000FFFFFFFF }, /* RAX */
		},
		[0b001] = {
			[0] = { NVMM_X64_GPR_RCX, 0x00000000000000FF }, /* CL */
			[1] = { NVMM_X64_GPR_RCX, 0x000000000000FFFF }, /* CX */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RCX, 0x00000000FFFFFFFF }, /* ECX */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RCX, 0x00000000FFFFFFFF }, /* RCX */
		},
		[0b010] = {
			[0] = { NVMM_X64_GPR_RDX, 0x00000000000000FF }, /* DL */
			[1] = { NVMM_X64_GPR_RDX, 0x000000000000FFFF }, /* DX */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RDX, 0x00000000FFFFFFFF }, /* EDX */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RDX, 0x00000000FFFFFFFF }, /* RDX */
		},
		[0b011] = {
			[0] = { NVMM_X64_GPR_RBX, 0x00000000000000FF }, /* BL */
			[1] = { NVMM_X64_GPR_RBX, 0x000000000000FFFF }, /* BX */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_RBX, 0x00000000FFFFFFFF }, /* EBX */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_RBX, 0x00000000FFFFFFFF }, /* RBX */
		},
		[0b100] = {
			[0] = { -1, 0 }, /* SPECIAL */
			[1] = { -1, 0 }, /* SPECIAL */
			[2] = { -1, 0 },
			[3] = { -1, 0 }, /* SPECIAL */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 }, /* SPECIAL */
		},
		[0b101] = {
			[0] = { -1, 0 }, /* SPECIAL */
			[1] = { -1, 0 }, /* SPECIAL */
			[2] = { -1, 0 },
			[3] = { -1, 0 }, /* SPECIAL */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 }, /* SPECIAL */
		},
		[0b110] = {
			[0] = { -1, 0 }, /* SPECIAL */
			[1] = { -1, 0 }, /* SPECIAL */
			[2] = { -1, 0 },
			[3] = { -1, 0 }, /* SPECIAL */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 }, /* SPECIAL */
		},
		[0b111] = {
			[0] = { -1, 0 }, /* SPECIAL */
			[1] = { -1, 0 }, /* SPECIAL */
			[2] = { -1, 0 },
			[3] = { -1, 0 }, /* SPECIAL */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { -1, 0 }, /* SPECIAL */
		},
	},
	[true] = {
		/* Extended. */
		[0b000] = {
			[0] = { NVMM_X64_GPR_R8, 0x00000000000000FF }, /* R8B */
			[1] = { NVMM_X64_GPR_R8, 0x000000000000FFFF }, /* R8W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R8, 0x00000000FFFFFFFF }, /* R8D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R8, 0x00000000FFFFFFFF }, /* R8 */
		},
		[0b001] = {
			[0] = { NVMM_X64_GPR_R9, 0x00000000000000FF }, /* R9B */
			[1] = { NVMM_X64_GPR_R9, 0x000000000000FFFF }, /* R9W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R9, 0x00000000FFFFFFFF }, /* R9D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R9, 0x00000000FFFFFFFF }, /* R9 */
		},
		[0b010] = {
			[0] = { NVMM_X64_GPR_R10, 0x00000000000000FF }, /* R10B */
			[1] = { NVMM_X64_GPR_R10, 0x000000000000FFFF }, /* R10W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R10, 0x00000000FFFFFFFF }, /* R10D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R10, 0x00000000FFFFFFFF }, /* R10 */
		},
		[0b011] = {
			[0] = { NVMM_X64_GPR_R11, 0x00000000000000FF }, /* R11B */
			[1] = { NVMM_X64_GPR_R11, 0x000000000000FFFF }, /* R11W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R11, 0x00000000FFFFFFFF }, /* R11D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R11, 0x00000000FFFFFFFF }, /* R11 */
		},
		[0b100] = {
			[0] = { NVMM_X64_GPR_R12, 0x00000000000000FF }, /* R12B */
			[1] = { NVMM_X64_GPR_R12, 0x000000000000FFFF }, /* R12W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R12, 0x00000000FFFFFFFF }, /* R12D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R12, 0x00000000FFFFFFFF }, /* R12 */
		},
		[0b101] = {
			[0] = { NVMM_X64_GPR_R13, 0x00000000000000FF }, /* R13B */
			[1] = { NVMM_X64_GPR_R13, 0x000000000000FFFF }, /* R13W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R13, 0x00000000FFFFFFFF }, /* R13D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R13, 0x00000000FFFFFFFF }, /* R13 */
		},
		[0b110] = {
			[0] = { NVMM_X64_GPR_R14, 0x00000000000000FF }, /* R14B */
			[1] = { NVMM_X64_GPR_R14, 0x000000000000FFFF }, /* R14W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R14, 0x00000000FFFFFFFF }, /* R14D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R14, 0x00000000FFFFFFFF }, /* R14 */
		},
		[0b111] = {
			[0] = { NVMM_X64_GPR_R15, 0x00000000000000FF }, /* R15B */
			[1] = { NVMM_X64_GPR_R15, 0x000000000000FFFF }, /* R15W */
			[2] = { -1, 0 },
			[3] = { NVMM_X64_GPR_R15, 0x00000000FFFFFFFF }, /* R15D */
			[4] = { -1, 0 },
			[5] = { -1, 0 },
			[6] = { -1, 0 },
			[7] = { NVMM_X64_GPR_R15, 0x00000000FFFFFFFF }, /* R15 */
		},
	}
};

static int
node_overflow(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	fsm->fn = NULL;
	return -1;
}

static int
fsm_read(struct x86_decode_fsm *fsm, uint8_t *bytes, size_t n)
{
	if (fsm->buf + n > fsm->end) {
		return -1;
	}
	memcpy(bytes, fsm->buf, n);
	return 0;
}

static void
fsm_advance(struct x86_decode_fsm *fsm, size_t n,
    int (*fn)(struct x86_decode_fsm *, struct x86_instr *))
{
	fsm->buf += n;
	if (fsm->buf > fsm->end) {
		fsm->fn = node_overflow;
	} else {
		fsm->fn = fn;
	}
}

static const struct x86_reg *
resolve_special_register(struct x86_instr *instr, uint8_t enc, size_t regsize)
{
	enc &= 0b11;
	if (regsize == 8) {
		/* May be 64bit without REX */
		return &gpr_map__special[1][enc][regsize-1];
	}
	return &gpr_map__special[instr->rexpref.present][enc][regsize-1];
}

/*
 * Special node, for STOS and LODS. Fake a displacement of zero on the
 * destination register.
 */
static int
node_stlo(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode = instr->opcode;
	struct x86_store *stlo, *streg;
	size_t adrsize, regsize;

	adrsize = instr->address_size;
	regsize = instr->operand_size;

	if (opcode->stos) {
		streg = &instr->src;
		stlo = &instr->dst;
	} else {
		streg = &instr->dst;
		stlo = &instr->src;
	}

	streg->type = STORE_REG;
	streg->u.reg = &gpr_map[0][0][regsize-1]; /* ?AX */

	stlo->type = STORE_REG;
	if (opcode->stos) {
		/* ES:RDI, force ES */
		stlo->u.reg = &gpr_map__special[1][3][adrsize-1];
		instr->legpref[LEG_OVR_ES] = true;
	} else {
		/* DS:RSI */
		stlo->u.reg = &gpr_map__special[1][2][adrsize-1];
	}
	stlo->disp.type = DISP_0;

	fsm_advance(fsm, 0, NULL);

	return 0;
}

static int
node_dmo(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode = instr->opcode;
	struct x86_store *stdmo, *streg;
	size_t adrsize, regsize;

	adrsize = instr->address_size;
	regsize = instr->operand_size;

	if (opcode->todmo) {
		streg = &instr->src;
		stdmo = &instr->dst;
	} else {
		streg = &instr->dst;
		stdmo = &instr->src;
	}

	streg->type = STORE_REG;
	streg->u.reg = &gpr_map[0][0][regsize-1]; /* ?AX */

	stdmo->type = STORE_DMO;
	if (fsm_read(fsm, (uint8_t *)&stdmo->u.dmo, adrsize) == -1) {
		return -1;
	}
	fsm_advance(fsm, adrsize, NULL);

	return 0;
}

static int
node_immediate(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode = instr->opcode;
	struct x86_store *store;
	uint8_t flags;
	uint8_t immsize;

	/* The immediate is the source */
	store = &instr->src;
	immsize = instr->operand_size;

	/* Get the correct flags */
	flags = opcode->flags;
	if ((flags & FLAG_z) && (immsize == 8)) {
		/* 'z' operates here */
		immsize = 4;
	}

	store->type = STORE_IMM;
	store->u.imm.size = immsize;

	if (fsm_read(fsm, store->u.imm.data, store->u.imm.size) == -1) {
		return -1;
	}

	fsm_advance(fsm, store->u.imm.size, NULL);

	return 0;
}

static int
node_disp(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode = instr->opcode;
	size_t n;

	if (instr->strm->disp.type == DISP_1) {
		n = 1;
	} else { /* DISP4 */
		n = 4;
	}

	if (fsm_read(fsm, instr->strm->disp.data, n) == -1) {
		return -1;
	}

	if (opcode->immediate) {
		fsm_advance(fsm, n, node_immediate);
	} else {
		fsm_advance(fsm, n, NULL);
	}

	return 0;
}

static const struct x86_reg *
get_register_idx(struct x86_instr *instr, uint8_t index)
{
	uint8_t enc = index;
	const struct x86_reg *reg;
	size_t regsize;

	regsize = instr->address_size;
	reg = &gpr_map[instr->rexpref.x][enc][regsize-1];

	if (reg->num == -1) {
		reg = resolve_special_register(instr, enc, regsize);
	}

	return reg;
}

static const struct x86_reg *
get_register_bas(struct x86_instr *instr, uint8_t base)
{
	uint8_t enc = base;
	const struct x86_reg *reg;
	size_t regsize;

	regsize = instr->address_size;
	reg = &gpr_map[instr->rexpref.b][enc][regsize-1];
	if (reg->num == -1) {
		reg = resolve_special_register(instr, enc, regsize);
	}

	return reg;
}

static int
node_sib(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode;
	uint8_t scale, index, base;
	bool noindex, nobase;
	uint8_t byte;

	if (fsm_read(fsm, &byte, sizeof(byte)) == -1) {
		return -1;
	}

	scale = ((byte & 0b11000000) >> 6);
	index = ((byte & 0b00111000) >> 3);
	base  = ((byte & 0b00000111) >> 0);

	opcode = instr->opcode;

	noindex = false;
	nobase = false;

	if (index == 0b100 && !instr->rexpref.x) {
		/* Special case: the index is null */
		noindex = true;
	}

	if (instr->regmodrm.mod == 0b00 && base == 0b101) {
		/* Special case: the base is null + disp32 */
		instr->strm->disp.type = DISP_4;
		nobase = true;
	}

	instr->strm->type = STORE_SIB;
	instr->strm->u.sib.scale = (1 << scale);
	if (!noindex)
		instr->strm->u.sib.idx = get_register_idx(instr, index);
	if (!nobase)
		instr->strm->u.sib.bas = get_register_bas(instr, base);

	/* May have a displacement, or an immediate */
	if (instr->strm->disp.type == DISP_1 || instr->strm->disp.type == DISP_4) {
		fsm_advance(fsm, 1, node_disp);
	} else if (opcode->immediate) {
		fsm_advance(fsm, 1, node_immediate);
	} else {
		fsm_advance(fsm, 1, NULL);
	}

	return 0;
}

static const struct x86_reg *
get_register_reg(struct x86_instr *instr, const struct x86_opcode *opcode)
{
	uint8_t enc = instr->regmodrm.reg;
	const struct x86_reg *reg;
	size_t regsize;

	if ((opcode->flags & FLAG_z) && (instr->operand_size == 8)) {
		/* 'z' operates here */
		regsize = 4;
	} else {
		regsize = instr->operand_size;
	}

	reg = &gpr_map[instr->rexpref.r][enc][regsize-1];
	if (reg->num == -1) {
		reg = resolve_special_register(instr, enc, regsize);
	}

	return reg;
}

static const struct x86_reg *
get_register_rm(struct x86_instr *instr, const struct x86_opcode *opcode)
{
	uint8_t enc = instr->regmodrm.rm;
	const struct x86_reg *reg;
	size_t regsize;

	if (instr->strm->disp.type == DISP_NONE) {
		if ((opcode->flags & FLAG_z) && (instr->operand_size == 8)) {
			/* 'z' operates here */
			regsize = 4;
		} else {
			regsize = instr->operand_size;
		}
	} else {
		/* Indirect access, the size is that of the address. */
		regsize = instr->address_size;
	}

	reg = &gpr_map[instr->rexpref.b][enc][regsize-1];
	if (reg->num == -1) {
		reg = resolve_special_register(instr, enc, regsize);
	}

	return reg;
}

static inline bool
has_sib(struct x86_instr *instr)
{
	return (instr->regmodrm.mod != 3 && instr->regmodrm.rm == 4);
}

static inline bool
is_rip_relative(struct x86_instr *instr)
{
	return (instr->strm->disp.type == DISP_0 &&
	    instr->regmodrm.rm == RM_RBP_DISP32);
}

static enum x86_disp_type
get_disp_type(struct x86_instr *instr)
{
	switch (instr->regmodrm.mod) {
	case MOD_DIS0:	/* indirect */
		return DISP_0;
	case MOD_DIS1:	/* indirect+1 */
		return DISP_1;
	case MOD_DIS4:	/* indirect+4 */
		return DISP_4;
	case MOD_REG:	/* direct */
	default:	/* gcc */
		return DISP_NONE;
	}
}

static int
node_regmodrm(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	struct x86_store *strg, *strm;
	const struct x86_opcode *opcode;
	const struct x86_reg *reg;
	uint8_t byte;

	if (fsm_read(fsm, &byte, sizeof(byte)) == -1) {
		return -1;
	}

	opcode = instr->opcode;

	instr->regmodrm.present = true;
	instr->regmodrm.mod = ((byte & 0b11000000) >> 6);
	instr->regmodrm.reg = ((byte & 0b00111000) >> 3);
	instr->regmodrm.rm  = ((byte & 0b00000111) >> 0);

	if (opcode->regtorm) {
		strg = &instr->src;
		strm = &instr->dst;
	} else { /* RM to REG */
		strm = &instr->src;
		strg = &instr->dst;
	}

	/* Save for later use. */
	instr->strm = strm;

	/*
	 * Special cases: Groups. The REG field of REGMODRM is the index in
	 * the group. op1 gets overwritten in the Immediate node, if any.
	 */
	if (opcode->group11) {
		if (group11[instr->regmodrm.reg].emul == NULL) {
			return -1;
		}
		instr->emul = group11[instr->regmodrm.reg].emul;
	}

	reg = get_register_reg(instr, opcode);
	if (reg == NULL) {
		return -1;
	}
	strg->type = STORE_REG;
	strg->u.reg = reg;

	if (has_sib(instr)) {
		/* Overwrites RM */
		fsm_advance(fsm, 1, node_sib);
		return 0;
	}

	/* The displacement applies to RM. */
	strm->disp.type = get_disp_type(instr);

	if (is_rip_relative(instr)) {
		/* Overwrites RM */
		strm->type = STORE_REG;
		strm->u.reg = &gpr_map__rip;
		strm->disp.type = DISP_4;
		fsm_advance(fsm, 1, node_disp);
		return 0;
	}

	reg = get_register_rm(instr, opcode);
	if (reg == NULL) {
		return -1;
	}
	strm->type = STORE_REG;
	strm->u.reg = reg;

	if (strm->disp.type == DISP_NONE) {
		/* Direct register addressing mode */
		if (opcode->immediate) {
			fsm_advance(fsm, 1, node_immediate);
		} else {
			fsm_advance(fsm, 1, NULL);
		}
	} else if (strm->disp.type == DISP_0) {
		/* Indirect register addressing mode */
		if (opcode->immediate) {
			fsm_advance(fsm, 1, node_immediate);
		} else {
			fsm_advance(fsm, 1, NULL);
		}
	} else {
		fsm_advance(fsm, 1, node_disp);
	}

	return 0;
}

static size_t
get_operand_size(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode = instr->opcode;
	int opsize;

	/* Get the opsize */
	if (!opcode->szoverride) {
		opsize = opcode->defsize;
	} else if (instr->rexpref.present && instr->rexpref.w) {
		opsize = 8;
	} else {
		if (!fsm->is16bit) {
			if (instr->legpref[LEG_OPR_OVR]) {
				opsize = 2;
			} else {
				opsize = 4;
			}
		} else { /* 16bit */
			if (instr->legpref[LEG_OPR_OVR]) {
				opsize = 4;
			} else {
				opsize = 2;
			}
		}
	}

	/* See if available */
	if ((opcode->allsize & opsize) == 0) {
		// XXX do we care?
	}

	return opsize;
}

static size_t
get_address_size(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	if (fsm->is64bit) {
		if (__predict_false(instr->legpref[LEG_ADR_OVR])) {
			return 4;
		}
		return 8;
	}

	if (fsm->is32bit) {
		if (__predict_false(instr->legpref[LEG_ADR_OVR])) {
			return 2;
		}
		return 4;
	}

	/* 16bit. */
	if (__predict_false(instr->legpref[LEG_ADR_OVR])) {
		return 4;
	}
	return 2;
}

static int
node_primary_opcode(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	const struct x86_opcode *opcode;
	uint8_t byte;
	size_t i, n;

	if (fsm_read(fsm, &byte, sizeof(byte)) == -1) {
		return -1;
	}

	n = sizeof(primary_opcode_table) / sizeof(primary_opcode_table[0]);
	for (i = 0; i < n; i++) {
		if (primary_opcode_table[i].byte == byte)
			break;
	}
	if (i == n) {
		return -1;
	}
	opcode = &primary_opcode_table[i];

	instr->opcode = opcode;
	instr->emul = opcode->emul;
	instr->operand_size = get_operand_size(fsm, instr);
	instr->address_size = get_address_size(fsm, instr);

	if (opcode->regmodrm) {
		fsm_advance(fsm, 1, node_regmodrm);
	} else if (opcode->dmo) {
		/* Direct-Memory Offsets */
		fsm_advance(fsm, 1, node_dmo);
	} else if (opcode->stos || opcode->lods) {
		fsm_advance(fsm, 1, node_stlo);
	} else {
		return -1;
	}

	return 0;
}

static int
node_main(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	uint8_t byte;

#define ESCAPE	0x0F
#define VEX_1	0xC5
#define VEX_2	0xC4
#define XOP	0x8F

	if (fsm_read(fsm, &byte, sizeof(byte)) == -1) {
		return -1;
	}

	/*
	 * We don't take XOP. It is AMD-specific, and it was removed shortly
	 * after being introduced.
	 */
	if (byte == ESCAPE) {
		return -1;
	} else if (!instr->rexpref.present) {
		if (byte == VEX_1) {
			return -1;
		} else if (byte == VEX_2) {
			return -1;
		} else {
			fsm->fn = node_primary_opcode;
		}
	} else {
		fsm->fn = node_primary_opcode;
	}

	return 0;
}

static int
node_rex_prefix(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	struct x86_rexpref *rexpref = &instr->rexpref;
	uint8_t byte;
	size_t n = 0;

	if (fsm_read(fsm, &byte, sizeof(byte)) == -1) {
		return -1;
	}

	if (byte >= 0x40 && byte <= 0x4F) {
		if (__predict_false(!fsm->is64bit)) {
			return -1;
		}
		rexpref->present = true;
		rexpref->w = ((byte & 0x8) != 0);
		rexpref->r = ((byte & 0x4) != 0);
		rexpref->x = ((byte & 0x2) != 0);
		rexpref->b = ((byte & 0x1) != 0);
		n = 1;
	}

	fsm_advance(fsm, n, node_main);
	return 0;
}

static const uint8_t legpref_table[NLEG] = {
	/* Group 1 */
	[LEG_LOCK] = 0xF0,
	[LEG_REPN] = 0xF2,
	[LEG_REP]  = 0xF3,
	/* Group 2 */
	[LEG_OVR_CS] = 0x2E,
	[LEG_OVR_SS] = 0x36,
	[LEG_OVR_DS] = 0x3E,
	[LEG_OVR_ES] = 0x26,
	[LEG_OVR_FS] = 0x64,
	[LEG_OVR_GS] = 0x65,
	[LEG_BRN_TAKEN]  = 0x2E,
	[LEG_BRN_NTAKEN] =  0x3E,
	/* Group 3 */
	[LEG_OPR_OVR] = 0x66,
	/* Group 4 */
	[LEG_ADR_OVR] = 0x67
};

static int
node_legacy_prefix(struct x86_decode_fsm *fsm, struct x86_instr *instr)
{
	uint8_t byte;
	size_t i;

	if (fsm_read(fsm, &byte, sizeof(byte)) == -1) {
		return -1;
	}

	for (i = 0; i < NLEG; i++) {
		if (byte == legpref_table[i])
			break;
	}

	if (i == NLEG) {
		fsm->fn = node_rex_prefix;
	} else {
		instr->legpref[i] = true;
		fsm_advance(fsm, 1, node_legacy_prefix);
	}

	return 0;
}

static int
x86_decode(uint8_t *inst_bytes, size_t inst_len, struct x86_instr *instr,
    struct nvmm_x64_state *state)
{
	struct x86_decode_fsm fsm;
	int ret;

	memset(instr, 0, sizeof(*instr));

	fsm.is64bit = is_64bit(state);
	fsm.is32bit = is_32bit(state);
	fsm.is16bit = is_16bit(state);

	fsm.fn = node_legacy_prefix;
	fsm.buf = inst_bytes;
	fsm.end = inst_bytes + inst_len;

	while (fsm.fn != NULL) {
		ret = (*fsm.fn)(&fsm, instr);
		if (ret == -1)
			return -1;
	}

	instr->len = fsm.buf - inst_bytes;

	return 0;
}

/* -------------------------------------------------------------------------- */

static inline uint8_t
compute_parity(uint8_t *data)
{
	uint64_t *ptr = (uint64_t *)data;
	uint64_t val = *ptr;

	val ^= val >> 32;
	val ^= val >> 16;
	val ^= val >> 8;
	val ^= val >> 4;
	val ^= val >> 2;
	val ^= val >> 1;
	return (~val) & 1;
}

static void
x86_emul_or(struct nvmm_mem *mem, void (*cb)(struct nvmm_mem *),
    uint64_t *gprs)
{
	const bool write = mem->write;
	uint64_t fl = gprs[NVMM_X64_GPR_RFLAGS];
	uint8_t data[8];
	size_t i;

	fl &= ~(PSL_V|PSL_C|PSL_Z|PSL_N|PSL_PF);

	memcpy(data, mem->data, sizeof(data));

	/* Fetch the value to be OR'ed. */
	mem->write = false;
	(*cb)(mem);

	/* Perform the OR. */
	for (i = 0; i < mem->size; i++) {
		mem->data[i] |= data[i];
		if (mem->data[i] != 0)
			fl |= PSL_Z;
	}
	if (mem->data[mem->size-1] & __BIT(7))
		fl |= PSL_N;
	if (compute_parity(mem->data))
		fl |= PSL_PF;

	if (write) {
		/* Write back the result. */
		mem->write = true;
		(*cb)(mem);
	}

	gprs[NVMM_X64_GPR_RFLAGS] = fl;
}

static void
x86_emul_and(struct nvmm_mem *mem, void (*cb)(struct nvmm_mem *),
    uint64_t *gprs)
{
	const bool write = mem->write;
	uint64_t fl = gprs[NVMM_X64_GPR_RFLAGS];
	uint8_t data[8];
	size_t i;

	fl &= ~(PSL_V|PSL_C|PSL_Z|PSL_N|PSL_PF);

	memcpy(data, mem->data, sizeof(data));

	/* Fetch the value to be AND'ed. */
	mem->write = false;
	(*cb)(mem);

	/* Perform the AND. */
	for (i = 0; i < mem->size; i++) {
		mem->data[i] &= data[i];
		if (mem->data[i] != 0)
			fl |= PSL_Z;
	}
	if (mem->data[mem->size-1] & __BIT(7))
		fl |= PSL_N;
	if (compute_parity(mem->data))
		fl |= PSL_PF;

	if (write) {
		/* Write back the result. */
		mem->write = true;
		(*cb)(mem);
	}

	gprs[NVMM_X64_GPR_RFLAGS] = fl;
}

static void
x86_emul_xor(struct nvmm_mem *mem, void (*cb)(struct nvmm_mem *),
    uint64_t *gprs)
{
	const bool write = mem->write;
	uint64_t fl = gprs[NVMM_X64_GPR_RFLAGS];
	uint8_t data[8];
	size_t i;

	fl &= ~(PSL_V|PSL_C|PSL_Z|PSL_N|PSL_PF);

	memcpy(data, mem->data, sizeof(data));

	/* Fetch the value to be XOR'ed. */
	mem->write = false;
	(*cb)(mem);

	/* Perform the XOR. */
	for (i = 0; i < mem->size; i++) {
		mem->data[i] ^= data[i];
		if (mem->data[i] != 0)
			fl |= PSL_Z;
	}
	if (mem->data[mem->size-1] & __BIT(7))
		fl |= PSL_N;
	if (compute_parity(mem->data))
		fl |= PSL_PF;

	if (write) {
		/* Write back the result. */
		mem->write = true;
		(*cb)(mem);
	}

	gprs[NVMM_X64_GPR_RFLAGS] = fl;
}

static void
x86_emul_mov(struct nvmm_mem *mem, void (*cb)(struct nvmm_mem *),
    uint64_t *gprs)
{
	/*
	 * Nothing special, just move without emulation.
	 */
	(*cb)(mem);
}

static void
x86_emul_stos(struct nvmm_mem *mem, void (*cb)(struct nvmm_mem *),
    uint64_t *gprs)
{
	/*
	 * Just move, and update RDI.
	 */
	(*cb)(mem);

	if (gprs[NVMM_X64_GPR_RFLAGS] & PSL_D) {
		gprs[NVMM_X64_GPR_RDI] -= mem->size;
	} else {
		gprs[NVMM_X64_GPR_RDI] += mem->size;
	}
}

static void
x86_emul_lods(struct nvmm_mem *mem, void (*cb)(struct nvmm_mem *),
    uint64_t *gprs)
{
	/*
	 * Just move, and update RSI.
	 */
	(*cb)(mem);

	if (gprs[NVMM_X64_GPR_RFLAGS] & PSL_D) {
		gprs[NVMM_X64_GPR_RSI] -= mem->size;
	} else {
		gprs[NVMM_X64_GPR_RSI] += mem->size;
	}
}

/* -------------------------------------------------------------------------- */

static inline uint64_t
gpr_read_address(struct x86_instr *instr, struct nvmm_x64_state *state, int gpr)
{
	uint64_t val;

	val = state->gprs[gpr];
	if (__predict_false(instr->address_size == 4)) {
		val &= 0x00000000FFFFFFFF;
	} else if (__predict_false(instr->address_size == 2)) {
		val &= 0x000000000000FFFF;
	}

	return val;
}

static int
store_to_mem(struct nvmm_machine *mach, struct nvmm_x64_state *state,
    struct x86_instr *instr, struct x86_store *store, struct nvmm_mem *mem)
{
	struct x86_sib *sib;
	nvmm_prot_t prot;
	gvaddr_t gva, off;
	uint64_t reg;
	int ret, seg;
	uint32_t *p;

	gva = 0;

	if (store->type == STORE_SIB) {
		sib = &store->u.sib;
		if (sib->bas != NULL)
			gva += gpr_read_address(instr, state, sib->bas->num);
		if (sib->idx != NULL) {
			reg = gpr_read_address(instr, state, sib->idx->num);
			gva += sib->scale * reg;
		}
	} else if (store->type == STORE_REG) {
		gva = gpr_read_address(instr, state, store->u.reg->num);
	} else {
		gva = store->u.dmo;
	}

	if (store->disp.type != DISP_NONE) {
		p = (uint32_t *)&store->disp.data[0];
		gva += *p;
	}

	mem->gva = gva;

	if (!is_long_mode(state)) {
		if (instr->legpref[LEG_OVR_CS]) {
			seg = NVMM_X64_SEG_CS;
		} else if (instr->legpref[LEG_OVR_SS]) {
			seg = NVMM_X64_SEG_SS;
		} else if (instr->legpref[LEG_OVR_ES]) {
			seg = NVMM_X64_SEG_ES;
		} else if (instr->legpref[LEG_OVR_FS]) {
			seg = NVMM_X64_SEG_FS;
		} else if (instr->legpref[LEG_OVR_GS]) {
			seg = NVMM_X64_SEG_GS;
		} else {
			seg = NVMM_X64_SEG_DS;
		}

		ret = segment_apply(&state->segs[seg], &mem->gva, mem->size);
		if (ret == -1)
			return -1;
	}

	if ((mem->gva & PAGE_MASK) + mem->size > PAGE_SIZE) {
		/* Don't allow a cross-page MMIO. */
		errno = EINVAL;
		return -1;
	}

	off = (mem->gva & PAGE_MASK);
	mem->gva &= ~PAGE_MASK;

	ret = x86_gva_to_gpa(mach, state, mem->gva, &mem->gpa, &prot);
	if (ret == -1)
		return -1;

	mem->gva += off;
	mem->gpa += off;

	return 0;
}

static int
fetch_instruction(struct nvmm_machine *mach, struct nvmm_x64_state *state,
    struct nvmm_exit *exit)
{
	size_t fetchsize, remain, done;
	gvaddr_t gva, off;
	nvmm_prot_t prot;
	gpaddr_t gpa;
	uintptr_t hva;
	uint8_t *ptr;
	int ret;

	fetchsize = sizeof(exit->u.mem.inst_bytes);

	gva = state->gprs[NVMM_X64_GPR_RIP];
	if (!is_long_mode(state)) {
		ret = segment_apply(&state->segs[NVMM_X64_SEG_CS], &gva,
		    fetchsize);
		if (ret == -1)
			return -1;
	}

	off = (gva & PAGE_MASK);
	gva &= ~PAGE_MASK;

	ret = x86_gva_to_gpa(mach, state, gva, &gpa, &prot);
	if (ret == -1)
		return -1;
	if (__predict_false((prot & NVMM_PROT_EXEC) == 0)) {
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
	if (__predict_false(off + fetchsize > PAGE_SIZE)) {
		remain = off + fetchsize - PAGE_SIZE;
		done = PAGE_SIZE - off;

		memcpy(exit->u.mem.inst_bytes, ptr, done);

		ret = x86_gva_to_gpa(mach, state, gva + PAGE_SIZE,
		    &gpa, &prot);
		if (ret == -1)
			return -1;
		if (__predict_false((prot & NVMM_PROT_EXEC) == 0)) {
			errno = EFAULT;
			return -1;
		}
		ret = nvmm_gpa_to_hva(mach, gpa, &hva);
		if (ret == -1)
			return -1;

		memcpy(&exit->u.mem.inst_bytes[done], (uint8_t *)hva, remain);
	} else {
		memcpy(exit->u.mem.inst_bytes, ptr, fetchsize);
		exit->u.mem.inst_len = fetchsize;
	}

	return 0;
}

#define DISASSEMBLER_BUG()	\
	do {			\
		errno = EINVAL;	\
		return -1;	\
	} while (0);

int
nvmm_assist_mem(struct nvmm_machine *mach, nvmm_cpuid_t cpuid,
    struct nvmm_exit *exit, void (*cb)(struct nvmm_mem *))
{
	struct nvmm_x64_state state;
	struct x86_instr instr;
	struct nvmm_mem mem;
	uint64_t val;
	int ret;

	if (__predict_false(exit->reason != NVMM_EXIT_MEMORY)) {
		errno = EINVAL;
		return -1;
	}

	ret = nvmm_vcpu_getstate(mach, cpuid, &state,
	    NVMM_X64_STATE_GPRS | NVMM_X64_STATE_SEGS | NVMM_X64_STATE_CRS |
	    NVMM_X64_STATE_MSRS);
	if (ret == -1)
		return -1;

	if (exit->u.mem.inst_len == 0) {
		/*
		 * The instruction was not fetched from the kernel. Fetch
		 * it ourselves.
		 */
		ret = fetch_instruction(mach, &state, exit);
		if (ret == -1)
			return -1;
	}

	ret = x86_decode(exit->u.mem.inst_bytes, exit->u.mem.inst_len,
	    &instr, &state);
	if (ret == -1) {
		errno = ENODEV;
		return -1;
	}

	if (instr.legpref[LEG_REPN]) {
		errno = ENODEV;
		return -1;
	}

	memset(&mem, 0, sizeof(mem));

	switch (instr.src.type) {
	case STORE_REG:
		if (instr.src.disp.type != DISP_NONE) {
			/* Indirect access. */
			mem.write = false;
			mem.size = instr.operand_size;
			ret = store_to_mem(mach, &state, &instr, &instr.src,
			    &mem);
			if (ret == -1)
				return -1;
		} else {
			/* Direct access. */
			mem.write = true;
			mem.size = instr.operand_size;
			val = state.gprs[instr.src.u.reg->num];
			val = __SHIFTOUT(val, instr.src.u.reg->mask);
			memcpy(mem.data, &val, mem.size);
		}
		break;

	case STORE_IMM:
		mem.write = true;
		mem.size = instr.src.u.imm.size;
		memcpy(mem.data, instr.src.u.imm.data, mem.size);
		break;

	case STORE_SIB:
		mem.write = false;
		mem.size = instr.operand_size;
		ret = store_to_mem(mach, &state, &instr, &instr.src,
		    &mem);
		if (ret == -1)
			return -1;
		break;

	case STORE_DMO:
		mem.write = false;
		mem.size = instr.operand_size;
		ret = store_to_mem(mach, &state, &instr, &instr.src,
		    &mem);
		if (ret == -1)
			return -1;
		break;

	default:
		return -1;
	}

	switch (instr.dst.type) {
	case STORE_REG:
		if (instr.dst.disp.type != DISP_NONE) {
			if (__predict_false(!mem.write)) {
				DISASSEMBLER_BUG();
			}
			mem.size = instr.operand_size;
			ret = store_to_mem(mach, &state, &instr, &instr.dst,
			    &mem);
			if (ret == -1)
				return -1;
		} else {
			/* nothing */
		}
		break;

	case STORE_IMM:
		/* The dst can't be an immediate. */
		DISASSEMBLER_BUG();

	case STORE_SIB:
		if (__predict_false(!mem.write)) {
			DISASSEMBLER_BUG();
		}
		mem.size = instr.operand_size;
		ret = store_to_mem(mach, &state, &instr, &instr.dst,
		    &mem);
		if (ret == -1)
			return -1;
		break;

	case STORE_DMO:
		if (__predict_false(!mem.write)) {
			DISASSEMBLER_BUG();
		}
		mem.size = instr.operand_size;
		ret = store_to_mem(mach, &state, &instr, &instr.dst,
		    &mem);
		if (ret == -1)
			return -1;
		break;

	default:
		return -1;
	}

	(*instr.emul)(&mem, cb, state.gprs);

	if (!mem.write) {
		/* instr.dst.type == STORE_REG */
		memcpy(&val, mem.data, sizeof(uint64_t));
		val = __SHIFTIN(val, instr.dst.u.reg->mask);
		state.gprs[instr.dst.u.reg->num] &= ~instr.dst.u.reg->mask;
		state.gprs[instr.dst.u.reg->num] |= val;
	}

	if (instr.legpref[LEG_REP]) {
		state.gprs[NVMM_X64_GPR_RCX] -= 1;
		if (state.gprs[NVMM_X64_GPR_RCX] == 0) {
			state.gprs[NVMM_X64_GPR_RIP] += instr.len;
		}
	} else {
		state.gprs[NVMM_X64_GPR_RIP] += instr.len;
	}

	ret = nvmm_vcpu_setstate(mach, cpuid, &state, NVMM_X64_STATE_GPRS);
	if (ret == -1)
		return -1;

	return 0;
}
