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
 *
 * $FreeBSD: head/sys/cddl/dev/fbt/x86/fbt_isa.c 309785 2016-12-10 03:11:05Z markj $
 *
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/proc.h>
#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/module.h>
#include <sys/kmem.h>

#include <sys/dtrace.h>

#if 1
#include <machine/cpufunc.h>
#include <machine/specialreg.h>
#if 0
#include <x86/cpuvar.h>
#endif
#include <x86/cputypes.h>
#endif

#include "fbt.h"

#define	FBT_PUSHL_EBP		0x55
#define	FBT_MOVL_ESP_EBP0_V0	0x8b
#define	FBT_MOVL_ESP_EBP1_V0	0xec
#define	FBT_MOVL_ESP_EBP0_V1	0x89
#define	FBT_MOVL_ESP_EBP1_V1	0xe5
#define	FBT_REX_RSP_RBP		0x48

#define	FBT_POPL_EBP		0x5d
#define	FBT_RET			0xc3
#define	FBT_RET_IMM16		0xc2
#define	FBT_LEAVE		0xc9

#ifdef __amd64__
#define	FBT_PATCHVAL		0xcc
#else
#define	FBT_PATCHVAL		0xf0
#endif

#define	FBT_ENTRY	"entry"
#define	FBT_RETURN	"return"

int
fbt_invop(uintptr_t addr, struct trapframe *frame, uintptr_t rval)
{
	solaris_cpu_t *cpu;
	uintptr_t *stack;
	uintptr_t arg0, arg1, arg2, arg3, arg4;
	fbt_probe_t *fbt;

#ifdef __amd64__
	stack = (uintptr_t *)frame->tf_rsp;
#else
	/* Skip hardware-saved registers. */
#ifdef __NetBSD__
	stack = (uintptr_t *)&frame->tf_esp;
#else
	stack = (uintptr_t *)frame->tf_isp + 3;
#endif
#endif

	cpu = &solaris_cpu[cpu_number()];
	fbt = fbt_probetab[FBT_ADDR2NDX(addr)];
	for (; fbt != NULL; fbt = fbt->fbtp_hashnext) {
		if ((uintptr_t)fbt->fbtp_patchpoint == addr) {
			if (fbt->fbtp_roffset == 0) {
#ifdef __amd64__
				/* fbt->fbtp_rval == DTRACE_INVOP_PUSHQ_RBP */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				cpu->cpu_dtrace_caller = stack[0];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);

				arg0 = frame->tf_rdi;
				arg1 = frame->tf_rsi;
				arg2 = frame->tf_rdx;
				arg3 = frame->tf_rcx;
				arg4 = frame->tf_r8;
#else
				int i = 0;

				/*
				 * When accessing the arguments on the stack,
				 * we must protect against accessing beyond
				 * the stack.  We can safely set NOFAULT here
				 * -- we know that interrupts are already
				 * disabled.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				cpu->cpu_dtrace_caller = stack[i++];
				arg0 = stack[i++];
				arg1 = stack[i++];
				arg2 = stack[i++];
				arg3 = stack[i++];
				arg4 = stack[i++];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);
#endif

				dtrace_probe(fbt->fbtp_id, arg0, arg1,
				    arg2, arg3, arg4);

				cpu->cpu_dtrace_caller = 0;
			} else {
#ifdef __amd64__
				/*
				 * On amd64, we instrument the ret, not the
				 * leave.  We therefore need to set the caller
				 * to ensure that the top frame of a stack()
				 * action is correct.
				 */
				DTRACE_CPUFLAG_SET(CPU_DTRACE_NOFAULT);
				cpu->cpu_dtrace_caller = stack[0];
				DTRACE_CPUFLAG_CLEAR(CPU_DTRACE_NOFAULT |
				    CPU_DTRACE_BADADDR);
#endif

				dtrace_probe(fbt->fbtp_id, fbt->fbtp_roffset,
				    rval, 0, 0, 0);
				cpu->cpu_dtrace_caller = 0;
			}

			return (fbt->fbtp_rval);
		}
	}

	return (0);
}




#ifdef __FreeBSD__
void
fbt_patch_tracepoint(fbt_probe_t *fbt, fbt_patchval_t val)
{

	*fbt->fbtp_patchpoint = val;
}
#endif

#ifdef __NetBSD__
void
fbt_patch_tracepoint(fbt_probe_t *fbt, fbt_patchval_t val)
{
	u_long psl;
	u_long cr0;

	/* Disable interrupts. */
	psl = x86_read_psl();
	x86_disable_intr();

	/* Disable write protection in supervisor mode. */
	cr0 = rcr0();
	lcr0(cr0 & ~CR0_WP);

	for (; fbt != NULL; fbt = fbt->fbtp_next) {
		*fbt->fbtp_patchpoint = val;
	}

	/* Write back and invalidate cache, flush pipelines. */
	wbinvd();
	x86_flush();
	x86_write_psl(psl);

	/* Re-enable write protection. */
	lcr0(cr0);
}
#endif


#ifdef __FreeBSD__
int
fbt_provide_module_function(linker_file_t lf, int symindx,
    linker_symval_t *symval, void *opaque)
#endif
#ifdef __NetBSD__
int
fbt_provide_module_cb(const char *name, int symindx, void *value,
    uint32_t symsize, int type, void *opaque)
#endif
{
	fbt_probe_t *fbt, *retfbt;
	u_int8_t *instr, *limit;
	int j;
	int size;

#ifdef __FreeBSD_
	char *modname = opaque;
	const char *name = symval->name;
	size_t symsize = symval->size;
	void *value = symval->value;

	/*
	 * trap_check() is a wrapper for DTrace's fault handler, so we don't
	 * want to be able to instrument it.
	 */
	if (strcmp(name, "trap_check") == 0)
		return (0);
#endif
#ifdef __NetBSD__
	struct fbt_ksyms_arg *fka = opaque;
	modctl_t *mod = fka->fka_mod;
	const char *modname = module_name(mod);

	/* got a function? */
	if (ELF_ST_TYPE(type) != STT_FUNC)
		return 0;

	/*
	 * Exclude some more symbols which can be called from probe context.
	 */
	if (strcmp(name, "x86_curcpu") == 0 ||
	    strcmp(name, "x86_curlwp") == 0) {
		return 0;
	}
#endif

	if (fbt_excluded(name))
		return (0);

	instr = (u_int8_t *) value;
	limit = (u_int8_t *) value + symsize;

#ifdef __amd64__
	while (instr < limit) {
		if (*instr == FBT_PUSHL_EBP)
			break;

		if ((size = dtrace_instr_size(instr)) <= 0)
			break;

		instr += size;
	}

	if (instr >= limit || *instr != FBT_PUSHL_EBP) {
		/*
		 * We either don't save the frame pointer in this
		 * function, or we ran into some disassembly
		 * screw-up.  Either way, we bail.
		 */
		return (0);
	}
#else
	if (instr[0] != FBT_PUSHL_EBP)
		return (0);

	if (!(instr[1] == FBT_MOVL_ESP_EBP0_V0 &&
	    instr[2] == FBT_MOVL_ESP_EBP1_V0) &&
	    !(instr[1] == FBT_MOVL_ESP_EBP0_V1 &&
	    instr[2] == FBT_MOVL_ESP_EBP1_V1))
		return (0);
#endif

	fbt = kmem_zalloc(sizeof (*fbt), KM_SLEEP);
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
	fbt->fbtp_rval = DTRACE_INVOP_PUSHL_EBP;
	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_symindx = symindx;

	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;
#ifdef __FreeBSD__
	lf->fbt_nentries++;
#endif

	retfbt = NULL;
again:
	if (instr >= limit)
		return (0);

	/*
	 * If this disassembly fails, then we've likely walked off into
	 * a jump table or some other unsuitable area.  Bail out of the
	 * disassembly now.
	 */
	if ((size = dtrace_instr_size(instr)) <= 0)
		return (0);

#ifdef __amd64__
	/*
	 * We only instrument "ret" on amd64 -- we don't yet instrument
	 * ret imm16, largely because the compiler doesn't seem to
	 * (yet) emit them in the kernel...
	 */
	if (*instr != FBT_RET) {
		instr += size;
		goto again;
	}
#else
	if (!(size == 1 &&
	    (*instr == FBT_POPL_EBP || *instr == FBT_LEAVE) &&
	    (*(instr + 1) == FBT_RET ||
	    *(instr + 1) == FBT_RET_IMM16))) {
		instr += size;
		goto again;
	}
#endif

	/*
	 * We (desperately) want to avoid erroneously instrumenting a
	 * jump table, especially given that our markers are pretty
	 * short:  two bytes on x86, and just one byte on amd64.  To
	 * determine if we're looking at a true instruction sequence
	 * or an inline jump table that happens to contain the same
	 * byte sequences, we resort to some heuristic sleeze:  we
	 * treat this instruction as being contained within a pointer,
	 * and see if that pointer points to within the body of the
	 * function.  If it does, we refuse to instrument it.
	 */
	for (j = 0; j < sizeof (uintptr_t); j++) {
		caddr_t check = (caddr_t) instr - j;
		uint8_t *ptr;

		if (check < (caddr_t)value)
			break;

		if (check + sizeof (caddr_t) > (caddr_t)limit)
			continue;

		ptr = *(uint8_t **)check;

		if (ptr >= (uint8_t *) value && ptr < limit) {
			instr += size;
			goto again;
		}
	}

	/*
	 * We have a winner!
	 */
	fbt = kmem_zalloc(sizeof (*fbt), KM_SLEEP);
	fbt->fbtp_name = name;

	if (retfbt == NULL) {
		fbt->fbtp_id = dtrace_probe_create(fbt_id, modname,
		    name, FBT_RETURN, 3, fbt);
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
	fbt->fbtp_symindx = symindx;

#ifndef __amd64__
	if (*instr == FBT_POPL_EBP) {
		fbt->fbtp_rval = DTRACE_INVOP_POPL_EBP;
	} else {
		ASSERT(*instr == FBT_LEAVE);
		fbt->fbtp_rval = DTRACE_INVOP_LEAVE;
	}
	fbt->fbtp_roffset =
	    (uintptr_t)(instr - (uint8_t *) value) + 1;

#else
	ASSERT(*instr == FBT_RET);
	fbt->fbtp_rval = DTRACE_INVOP_RET;
	fbt->fbtp_roffset =
		(uintptr_t)(instr - (uint8_t *) value);
#endif

	fbt->fbtp_savedval = *instr;
	fbt->fbtp_patchval = FBT_PATCHVAL;
	fbt->fbtp_hashnext = fbt_probetab[FBT_ADDR2NDX(instr)];
	fbt_probetab[FBT_ADDR2NDX(instr)] = fbt;

#ifdef __FreeBSD__
	lf->fbt_nentries++;
#endif

	instr += size;
	goto again;
}
