/*	$NetBSD: fbt_isa.c,v 1.1.2.2 2018/06/25 07:25:14 pgoyette Exp $	*/

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
 *
 * $FreeBSD: head/sys/cddl/dev/fbt/arm/fbt_isa.c 312378 2017-01-18 13:27:24Z andrew $
 *
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/kmem.h>

#include <sys/dtrace.h>

#include <machine/trap.h>
#include <arm/cpufunc.h>
#include <arm/armreg.h>
#include <arm/frame.h>
#include <uvm/uvm_extern.h>

#include "fbt.h"

#define	FBT_PUSHM		0xe92d0000
#define	FBT_POPM		0xe8bd0000
#define	FBT_JUMP		0xea000000
#define	FBT_SUBSP		0xe24dd000

#define	FBT_ENTRY	"entry"
#define	FBT_RETURN	"return"

int
fbt_invop(uintptr_t addr, struct trapframe *frame, uintptr_t rval)
{
	solaris_cpu_t *cpu = &solaris_cpu[cpu_number()];
	fbt_probe_t *fbt = fbt_probetab[FBT_ADDR2NDX(addr)];
	register_t fifthparam;

	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
			if (fbt->fbtp_roffset == 0) {
				/* Get 5th parameter from stack */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				fifthparam = *(register_t *)frame->tf_svc_sp;
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);

				cpu->cpu_dtrace_caller = frame->tf_svc_lr;
				dtrace_probe(fbt->fbtp_id, frame->tf_r0,
					     frame->tf_r1, frame->tf_r2,
					     frame->tf_r3, fifthparam);
			} else {
				/* XXX set caller */
				cpu->cpu_dtrace_caller = 0;
				dtrace_probe(fbt->fbtp_id, fbt->fbtp_roffset,
				    rval, 0, 0, 0);
			}

			cpu->cpu_dtrace_caller = 0;
			return (fbt->fbtp_rval);
		}
	}

	return (0);
}


void
fbt_patch_tracepoint(fbt_probe_t *fbt, fbt_patchval_t val)
{
	dtrace_icookie_t c;

	c = dtrace_interrupt_disable();

	ktext_write(fbt->fbtp_patchpoint, &val, sizeof (val));

	dtrace_interrupt_enable(c);
}

#ifdef __FreeBSD__

int
fbt_provide_module_function(linker_file_t lf, int symindx,
    linker_symval_t *symval, void *opaque)
{
	char *modname = opaque;
	const char *name = symval->name;
	fbt_probe_t *fbt, *retfbt;
	uint32_t *instr, *limit;
	int popm;

	if (fbt_excluded(name))
		return (0);

	instr = (uint32_t *)symval->value;
	limit = (uint32_t *)(symval->value + symval->size);

	/*
	 * va_arg functions has first instruction of
	 * sub sp, sp, #?
	 */
	if ((*instr & 0xfffff000) == FBT_SUBSP)
		instr++;

	/*
	 * check if insn is a pushm with LR
	 */
	if ((*instr & 0xffff0000) != FBT_PUSHM ||
	    (*instr & (1 << LR)) == 0)
		return (0);

	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 5, fbt);
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_BREAKPOINT;
	fbt->fbtp_rval = DTRACE_INVOP_PUSHM;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	lf->fbt_nentries++;

	popm = FBT_POPM | ((*instr) & 0x3FFF) | 0x8000;

	retfbt = NULL;
again:
	for (; instr < limit; instr++) {
		if (*instr == popm)
			break;
		else if ((*instr & 0xff000000) == FBT_JUMP) {
			uint32_t *target, *start;
			int offset;

			offset = (*instr & 0xffffff);
			offset <<= 8;
			offset /= 64;
			target = instr + (2 + offset);
			start = (uint32_t *)symval->value;
			if (target >= limit || target < start)
				break;
		}
	}

	if (instr >= limit)
		return (0);

	/*
	 * We have a winner!
	 */
	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
	fbt->fbtp_name = name;
	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    name, FBT_RETURN, 5, fbt);
	} else {
		retfbt->fbtp_next = fbt;
		fbt->fbtp_id = retfbt->fbtp_id;
	}
	retfbt = fbt;

	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = lf;
	fbt->fbtp_loadcnt = lf->loadcnt;
	fbt->fbtp_symindx = symindx;
	if ((*instr & 0xff000000) == FBT_JUMP)
		fbt->fbtp_rval = DTRACE_INVOP_B;
	else
		fbt->fbtp_rval = DTRACE_INVOP_POPM;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_BREAKPOINT;
	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	lf->fbt_nentries++;

	instr++;
	goto again;
}

#endif /* __FreeBSD_ */

#ifdef __NetBSD__

#define	FBT_PATCHVAL		DTRACE_BREAKPOINT

/* entry and return */
#define	FBT_BX_LR_P(insn)	(((insn) & ~INSN_COND_MASK) == 0x012fff1e)
#define	FBT_B_LABEL_P(insn)	(((insn) & 0xff000000) == 0xea000000)
/* entry */
#define	FBT_MOV_IP_SP_P(insn)	((insn) == 0xe1a0c00d)
/* index=1, add=1, wback=0 */
#define	FBT_LDR_IMM_P(insn)	(((insn) & 0xfff00000) == 0xe5900000)
#define	FBT_MOVW_P(insn)	(((insn) & 0xfff00000) == 0xe3000000)
#define	FBT_MOV_IMM_P(insn)	(((insn) & 0xffff0000) == 0xe3a00000)
#define	FBT_CMP_IMM_P(insn)	(((insn) & 0xfff00000) == 0xe3500000)
#define	FBT_PUSH_P(insn)	(((insn) & 0xffff0000) == 0xe92d0000)
/* return */
/* cond=always, writeback=no, rn=sp and register_list includes pc */
#define	FBT_LDM_P(insn)	(((insn) & 0x0fff8000) == 0x089d8000)
#define	FBT_LDMIB_P(insn)	(((insn) & 0x0fff8000) == 0x099d8000)
#define	FBT_MOV_PC_LR_P(insn)	(((insn) & ~INSN_COND_MASK) == 0x01a0f00e)
/* cond=always, writeback=no, rn=sp and register_list includes lr, but not pc */
#define	FBT_LDM_LR_P(insn)	(((insn) & 0xffffc000) == 0xe89d4000)
#define	FBT_LDMIB_LR_P(insn)	(((insn) & 0xffffc000) == 0xe99d4000)

/* rval = insn | invop_id (overwriting cond with invop ID) */
#define	BUILD_RVAL(insn, id)	(((insn) & ~INSN_COND_MASK) | __SHIFTIN((id), INSN_COND_MASK))
/* encode cond in the first byte */
#define	PATCHVAL_ENCODE_COND(insn)	(FBT_PATCHVAL | __SHIFTOUT((insn), INSN_COND_MASK))

int
fbt_provide_module_cb(const char *name, int symindx, void *value,
	uint32_t symsize, int type, void *opaque)
{
	fbt_probe_t *fbt, *retfbt;
	uint32_t *instr, *limit;
	bool was_ldm_lr = false;
	int size;

	struct fbt_ksyms_arg *fka = opaque;
	modctl_t *mod = fka->fka_mod;
	const char *modname = module_name(mod);


	/* got a function? */
	if (ELF_ST_TYPE(type) != STT_FUNC)
		return 0;

	if (fbt_excluded(name))
		return (0);

	/*
	 * Exclude some more symbols which can be called from probe context.
	 */
	if (strncmp(name, "_spl", 4) == 0 ||
	    strcmp(name, "binuptime") == 0 ||
	    strcmp(name, "nanouptime") == 0 ||
	    strcmp(name, "dosoftints") == 0 ||
	    strcmp(name, "fbt_emulate") == 0 ||
	    strcmp(name, "undefinedinstruction") == 0 ||
	    strncmp(name, "dmt_", 4) == 0 /* omap */ ||
	    strncmp(name, "mvsoctmr_", 9) == 0 /* marvell */ ) {
		return 0;
	}

	instr = (uint32_t *) value;
	limit = (uint32_t *)((uintptr_t)value + symsize);

	if (!FBT_MOV_IP_SP_P(*instr)
	    && !FBT_BX_LR_P(*instr)
	    && !FBT_MOVW_P(*instr)
	    && !FBT_MOV_IMM_P(*instr)
	    && !FBT_B_LABEL_P(*instr)
	    && !FBT_LDR_IMM_P(*instr)
	    && !FBT_CMP_IMM_P(*instr)
	    && !FBT_PUSH_P(*instr)
	    ) {
		return 0;
	}

	fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
	fbt->fbtp_name = name;
	fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
	    name, FBT_ENTRY, 5, fbt);
	fbt->fbtp_patchpoint = instr;
	fbt->fbtp_ctl = mod;
	/* fbt->fbtp_loadcnt = lf->loadcnt; */
	if (FBT_MOV_IP_SP_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOV_IP_SP);
	else if (FBT_LDR_IMM_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_LDR_IMM);
	else if (FBT_MOVW_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOVW);
	else if (FBT_MOV_IMM_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOV_IMM);
	else if (FBT_CMP_IMM_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_CMP_IMM);
	else if (FBT_BX_LR_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_BX_LR);
	else if (FBT_PUSH_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_PUSHM);
	else if (FBT_B_LABEL_P(*instr))
		fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_B);
	else
		KASSERT(0);

	KASSERTMSG((fbt->fbtp_rval >> 28) != 0,
		   "fbt %p insn 0x%x name %s rval 0x%08x",
		   fbt, *instr, name, fbt->fbtp_rval);

	fbt->fbtp_patchval = PATCHVAL_ENCODE_COND(*instr);
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

	retfbt = NULL;

	while (instr < limit) {
		if (instr >= limit)
			return (0);

		size = 1;

		if (!FBT_BX_LR_P(*instr)
		    && !FBT_MOV_PC_LR_P(*instr)
		    && !FBT_LDM_P(*instr)
		    && !FBT_LDMIB_P(*instr)
		    && !(was_ldm_lr && FBT_B_LABEL_P(*instr))
		    ) {
			if (FBT_LDM_LR_P(*instr) || FBT_LDMIB_LR_P(*instr))
				was_ldm_lr = true;
			else
				was_ldm_lr = false;
			instr += size;
			continue;
		}

		/*
		 * We have a winner!
		 */
		fbt = kmem_zalloc(sizeof (fbt_probe_t), KM_SLEEP);
		fbt->fbtp_name = name;

		if (retfbt == NULL) {
			fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
			    name, FBT_RETURN, 5, fbt);
		} else {
			retfbt->fbtp_next = fbt;
			fbt->fbtp_id = retfbt->fbtp_id;
		}

		retfbt = fbt;
		fbt->fbtp_patchpoint = instr;
		fbt->fbtp_ctl = mod;
		/* fbt->fbtp_loadcnt = lf->loadcnt; */
		fbt->fbtp_symindx = symindx;

		if (FBT_BX_LR_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_BX_LR);
		else if (FBT_MOV_PC_LR_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_MOV_PC_LR);
		else if (FBT_LDM_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_LDM);
		else if (FBT_LDMIB_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_POPM);
		else if (FBT_B_LABEL_P(*instr))
			fbt->fbtp_rval = BUILD_RVAL(*instr, DTRACE_INVOP_B);
		else
			KASSERT(0);

		KASSERTMSG((fbt->fbtp_rval >> 28) != 0, "fbt %p name %s rval 0x%08x",
			   fbt, name, fbt->fbtp_rval);

		fbt->fbtp_roffset = (uintptr_t)(instr - (uint32_t *) value);
		fbt->fbtp_patchval = PATCHVAL_ENCODE_COND(*instr);

		fbt->fbtp_savedval = *instr;
		fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
		fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

		instr += size;
		was_ldm_lr = false;
	}

	return 0;
}

#endif /* __NetBSD__ */
