/*	$NetBSD: fbt_isa.c,v 1.1 2021/03/29 05:17:09 simonb Exp $	*/

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
 * Portions Copyright 2015-2016 Ruslan Bukin <br@bsdpad.com>
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
#include <sys/kmem.h>
#include <sys/module.h>

#include <sys/dtrace.h>

#include <machine/regnum.h>
#include <machine/locore.h>

#include <mips/cache.h>

#include "fbt.h"

#ifdef __FreeBSD__
#define	CURRENT_CPU		curcpu
#endif
#ifdef __NetBSD__
#define	CURRENT_CPU		cpu_index(curcpu())
#endif

#define	FBT_PATCHVAL		(MIPS_BREAK_INSTR)
#define	FBT_ENTRY		"entry"
#define	FBT_RETURN		"return"

int
fbt_invop(uintptr_t addr, struct trapframe *frame, uintptr_t rval)
{
	solaris_cpu_t *cpu;
	fbt_probe_t *fbt;

	cpu = &solaris_cpu[CURRENT_CPU];
	fbt = fbt_probetab[FBT_ADDR2NDX(addr)];

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
			cpu->cpu_dtrace_caller = addr;

			dtrace_probe(fbt->fbtp_id, frame->tf_regs[_R_A0],
			    frame->tf_regs[_R_A1], frame->tf_regs[_R_A2],
			    frame->tf_regs[_R_A3], frame->tf_regs[_R_A4]);

			cpu->cpu_dtrace_caller = 0;
			return (fbt->fbtp_savedval);
		}
	}

	return (0);
}

void
fbt_patch_tracepoint(fbt_probe_t *fbt, fbt_patchval_t val)
{

	*fbt->fbtp_patchpoint = val;
	mips_icache_sync_range((vm_offset_t)fbt->fbtp_patchpoint, 4);
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
	uint32_t *instr, *limit;

#ifdef __FreeBSD__
	const char *name;
	char *modname;

	modname = opaque;
	name = symval->name;

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

	/* Look for store double to ra register */
	for (; instr < limit; instr++) {
		if ((*instr & LDSD_RA_SP_MASK) == SD_RA_SP)
			break;
	}

	if (instr >= limit)
		return (0);

#ifdef __FreeBSD__
	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
#endif
#ifdef __NetBSD__
	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
#endif
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 3, fbt);
	fbt->fbtp_patchpoint = instr;
#ifdef __FreeBSD__
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
#endif
#ifdef __NetBSD__
	fbt->fbtp_ctl = mod;
#endif
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_rval = DTRACE_INVOP_SD;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

#ifdef __FreeBSD__
	lf->fbt_nentries++;
#endif

	retfbt = NULL;
again:
	for (; instr < limit; instr++) {
		if ((*instr & LDSD_RA_SP_MASK) == LD_RA_SP) {
			break;
		}
	}

	if (instr >= limit)
		return (0);

	/*
	 * We have a winner!
	 */
#ifdef __FreeBSD__
	fbt = malloc(sizeof (fbt_probe_t), M_FBT, M_WAITOK | M_ZERO);
#endif
#ifdef __NetBSD__
	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
#endif
	fbt->fbtp_name = name;
	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    name, FBT_RETURN, 3, fbt);
	} else {
#ifdef __FreeBSD__
		retfbt->fbtp_probenext = fbt;
#endif
#ifdef __NetBSD__
		fbt->fbtp_ctl = mod;
#endif
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
	fbt->fbtp_symindx = symindx;
	fbt->fbtp_rval = DTRACE_INVOP_LD;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

#ifdef __FreeBSD__
	lf->fbt_nentries++;
#endif

	instr++;
	goto again;
}
