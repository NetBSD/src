/*	$NetBSD: db_trace.c,v 1.1.26.1 2007/02/27 16:51:55 yamt Exp $	*/

/* Inspired by reading alpha/db_trace.c */

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Cherry G. Mathew
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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


#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include <machine/cpufunc.h>
#include <machine/md_var.h>
#include <machine/db_machdep.h>

#include <ddb/db_sym.h>
#include <ddb/db_access.h>
#include <ddb/db_output.h>
#include <ddb/db_variables.h>
#include <ddb/db_interface.h>

#include <ia64/unwind/decode.h>
#include <ia64/unwind/stackframe.h>

#if 0
#define UNWIND_DIAGNOSTIC
#endif

#define debug_frame_dump_XXX(uwf) \
	printf("Frame Dump: \n bsp = 0x%lx \n pfs = 0x%lx, SOL(pfs) = %lu \n rp = 0x%lx \n",  \
	       uwf->bsp, uwf->pfs, IA64_CFM_SOL(uwf->pfs), uwf->rp);	\

void
initunwindframe(struct unwind_frame *uwf, struct trapframe *tf);
void
rewindframe(struct unwind_frame *uwf, db_addr_t ip);

void
db_stack_trace_print(db_expr_t addr, bool have_addr, db_expr_t count,
		     const char *modif, void (*pr)(const char *, ...))
{
	char c;
	const char *cp = modif;
	bool trace_thread = FALSE;
	bool trace_user = FALSE;
	struct trapframe *tf;
	struct unwind_frame current_frame;
	db_addr_t ip;
	const char *name;
	db_sym_t sym;
	db_expr_t offset;

	while ((c = *cp++) != 0) {
		trace_thread |= c == 't';
		trace_user |= c == 'u';
	}

	if (trace_user) {
		(*pr)("User-space stack tracing not implemented yet. \n");
		return;
	}
	if (!have_addr) {
		(*pr)("--Kernel Call Trace-- \n");

		tf = DDB_REGS;
		ip = tf->tf_special.iip + ((tf->tf_special.psr >> 41) & 3);

		initunwindframe(&current_frame, tf);

#ifdef UNWIND_DIAGNOSTIC
		struct unwind_frame *uwf = &current_frame;
		debug_frame_dump_XXX(uwf);
#endif
		patchunwindframe(&current_frame, ip - kernstart, kernstart);
#ifdef UNWIND_DIAGNOSTIC
		debug_frame_dump_XXX(uwf);
#endif
		/* Get into unwind loop. */

		while(ip) {
			sym = db_search_symbol(ip, DB_STGY_ANY, &offset);
			db_symbol_values(sym, &name, NULL);
			(*pr)("%s(...)\n", name);

			ip = current_frame.rp;

			if(!ip) break;

			rewindframe(&current_frame, ip);
		}

		return;


	} else (*pr) ("Unwind from arbitrary addresses unimplemented. \n");
		

		if (trace_thread) {
			(*pr)("trace by pid unimplemented. \n");
			return;
		}
		else {
			(*pr)("trace from arbitrary trap frame address unimplemented. \n");
		}

}

extern db_addr_t ia64_unwindtab;
extern vsize_t ia64_unwindtablen;


/* Generates initial unwind frame context based on the contents
 * of the trap frame, by consulting the Unwind library 
 * staterecord. If a register is of type enum UNSAVED, we fetch
 * the live value of the register from the trapframe.
 */

void
initunwindframe(struct unwind_frame *uwf, struct trapframe *tf)
		 
{

	uwf->rp = tf->tf_special.rp;

	/* ndirty = bsp - bspstore: , not the same as the definition in the spec.
	 * Gave me hell for a day!
	 * see: ia64/exception.S: exception_save_restore: */

	uwf->bsp = tf->tf_special.bspstore + tf->tf_special.ndirty;
	uwf->bsp = ia64_bsp_adjust_ret(uwf->bsp, IA64_CFM_SOF(tf->tf_special.cfm));
#ifdef UNWIND_DIAGNOSTIC
	printf("inituwframe(): SOF(cfm) = %lu \n", IA64_CFM_SOF(tf->tf_special.cfm));
#endif
	uwf->pfs = tf->tf_special.pfs;
	uwf->sp = uwf->psp = tf->tf_special.sp;


}
		


/* Single step the frame backward. 
 * Assumes unwind_frame is setup already.
 */

void
rewindframe(struct unwind_frame *uwf, db_addr_t ip)
{
/* XXX: Check for a stack switch */

	uwf->bsp = ia64_bsp_adjust_ret(uwf->bsp, IA64_CFM_SOL(uwf->pfs));
	uwf->sp = uwf->psp;

	/* Pre-stomp frame dump */
#ifdef UNWIND_DIAGNOSTIC
	debug_frame_dump_XXX(uwf);
#endif

	/* Stomp on rp and pfs 
	 */
	patchunwindframe(uwf, ip - kernstart, kernstart);

#ifdef UNWIND_DIAGNOSTIC
	debug_frame_dump_XXX(uwf);
#endif

}
