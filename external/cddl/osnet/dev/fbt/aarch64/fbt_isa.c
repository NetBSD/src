/*	$NetBSD: fbt_isa.c,v 1.8 2023/05/09 21:29:07 riastradh Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 * Portions Copyright 2013 Justin Hibbits jhibbits@freebsd.org
 * Portions Copyright 2013 Howard Su howardsu@freebsd.org
 * Portions Copyright 2015 Ruslan Bukin <br@bsdpad.com>
 *
 * $FreeBSD$
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <dev/mm.h>

#include <machine/cpufunc.h>

#include <sys/dtrace.h>

#include "fbt.h"

#define	AARCH64_BRK		0xd4200000
#define	AARCH64_BRK_IMM16_SHIFT	5
#define	AARCH64_BRK_IMM16_VAL	(0x40d << AARCH64_BRK_IMM16_SHIFT)
#define	FBT_PATCHVAL		(AARCH64_BRK | AARCH64_BRK_IMM16_VAL)
#define	FBT_ENTRY	"entry"
#define	FBT_RETURN	"return"

/*
 * How many artificial frames appear between dtrace_probe and the
 * interrupted function call?
 *
 *	fbt_invop
 *	dtrace_invop
 *	dtrace_invop_start
 *	el1_trap_exit
 */
#define	FBT_AFRAMES	4

int
fbt_invop(uintptr_t addr, struct trapframe *frame, uintptr_t r0)
{
	solaris_cpu_t *cpu;
	fbt_probe_t *fbt;

	cpu = &solaris_cpu[cpu_index(curcpu())];
	fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
			cpu->cpu_dtrace_caller = addr;

			dtrace_probe(fbt->fbtp_id, frame->tf_regs.r_reg[0],
			    frame->tf_regs.r_reg[1], frame->tf_regs.r_reg[2],
			    frame->tf_regs.r_reg[3], frame->tf_regs.r_reg[4]);

			cpu->cpu_dtrace_caller = 0;
			KASSERT(fbt->fbtp_savedval != 0);
			return (fbt->fbtp_savedval);
		}
	}

	return (0);
}

void
fbt_patch_tracepoint(fbt_probe_t *fbt, fbt_patchval_t val)
{
	paddr_t pa;
	vaddr_t va;

	if (!pmap_extract(pmap_kernel(), (vaddr_t)fbt->fbtp_patchpoint, &pa))
		return;
	if (!mm_md_direct_mapped_phys(pa, &va))
		return;
	*(fbt_patchval_t *)va = htole32(val);
	cpu_icache_sync_range((vm_offset_t)fbt->fbtp_patchpoint, sizeof(val));
}

#if defined(__FreeBSD__)
int
fbt_provide_module_function(linker_file_t lf, int symindx,
    linker_symval_t *symval, void *opaque)
#elif defined(__NetBSD__)
int
fbt_provide_module_cb(const char *name, int symindx, void *value,
    uint32_t symsize, int type, void *opaque)
#else
#error unsupported platform
#endif
{
	fbt_probe_t *fbt, *retfbt;
	uint32_t *target, *start;
	uint32_t *instr, *limit;
	int offs;

#ifdef __FreeBSD__
	char *modname = opaque;
	const char *name = symval->name;

	instr = (uint32_t *)(symval->value);
	limit = (uint32_t *)(symval->value + symval->size);
#endif
#ifdef __NetBSD__
	struct fbt_ksyms_arg *fka = opaque;
	modctl_t *mod = fka->fka_mod;
	const char *modname = module_name(mod);

	/* got a function? */
	if (ELF_ST_TYPE(type) != STT_FUNC)
		return 0;

	instr = (uint32_t *)(value);
	limit = (uint32_t *)((uintptr_t)value + symsize);
#endif

	/* Check if function is excluded from instrumentation */
	if (fbt_excluded(name))
		return (0);

	if (strncmp(name, "_spl", 4) == 0 ||
	    strcmp(name, "dosoftints") == 0 ||
	    strcmp(name, "nanouptime") == 0 ||
	    strncmp(name, "gtmr_", 5) == 0) {
		return 0;
	}

	/* Look for stp (pre-indexed) operation */
	for (; instr < limit; instr++) {
		if ((le32toh(*instr) & LDP_STP_MASK) == STP_64)
			break;
	}

	if (instr >= limit)
		return (0);
	KASSERT(*instr != 0);

#ifdef __FreeBSD__
	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
#else
	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
#endif
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, FBT_AFRAMES, fbt);
	fbt->fbtp_patchpoint = instr;
#ifdef __FreeBSD__
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
#endif
#ifdef __NetBSD__
	fbt->fbtp_ctl = mod;
#endif
	fbt->fbtp_savedval = le32toh(*instr);
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

#ifdef __FreeBSD__
	lf->fbt_nentries++;
#endif

	retfbt = NULL;
again:
	for (; instr < limit; instr++) {
		if (le32toh(*instr) == RET_INSTR)
			break;
		else if ((le32toh(*instr) & B_MASK) == B_INSTR) {
			offs = (le32toh(*instr) & B_DATA_MASK);
			offs *= 4;
			target = (instr + offs);
#ifdef __FreeBSD__
			start = (uint32_t *)symval->value;
#else
			start = (uint32_t *)value;
#endif
			if (target >= limit || target < start)
				break;
		}
	}

	if (instr >= limit)
		return (0);
	KASSERT(*instr != 0);

	/*
	 * We have a winner!
	 */
#ifdef __FreeBSD__
	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
#else
	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
#endif
	fbt->fbtp_name = name;
	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    name, FBT_RETURN, FBT_AFRAMES, fbt);
	} else {
		retfbt->fbtp_next = fbt;
		fbt->fbtp_id = retfbt->fbtp_id;
	}
	retfbt = fbt;

	fbt->fbtp_patchpoint = instr;
#ifdef __FreeBSD__
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
#endif
#ifdef __NetBSD__
	fbt->fbtp_ctl = mod;
#endif
	fbt->fbtp_savedval = le32toh(*instr);
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

#ifdef __FreeBSD__
	lf->fbt_nentries++;
#endif

	instr++;
	goto again;
}
