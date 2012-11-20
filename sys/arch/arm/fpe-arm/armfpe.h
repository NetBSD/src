/*	$NetBSD: armfpe.h,v 1.6.18.1 2012/11/20 03:01:05 tls Exp $	*/

/*
 * Copyright (c) 1995 Neil A Carson.
 * All rights reserved.
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
 * This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * armfpe.h
 *
 * Details of functions and structures needed for ARM FP cor
 * support in RiscBSD
 *
 * Created      : 04/01/96
 */

#ifndef _ARMFPE_H_
#define _ARMFPE_H_

#include <machine/fp.h>
#include <machine/ieeefp.h>
#include <machine/reg.h>

#ifdef _KERNEL

/*
 * Type for a saved FP context, if we want to translate the context to a
 * user-readable form
 */
 
typedef struct fp_context_frame {
	uint32_t fpsr;
	fp_extended_precision_t regs[8];
} fp_context_frame_t;

/* Define a structure that overlays the start of the core */

typedef struct {
	/*
	 * Addresses of procedures/functions
	 */

	uint32_t core_abort_addr;
	uint32_t core_initws_addr;
	uint32_t core_initcontext_addr;
	uint32_t core_changecontext_addr;
	uint32_t core_shutdown_addr;
	uint32_t core_activatecontext_addr;
	uint32_t core_deactivatecontext_addr;
	uint32_t core_savecontext_addr;
	uint32_t core_loadcontext_addr;
	uint32_t core_disable_addr;
	uint32_t core_enable_addr;

	/*
	 * Addresses of things that need to be filled in by the kernel on startup
	 */

	uint32_t *main_ws_ptr_addr;
	uint32_t *local_handler_ptr_addr;
	uint32_t *old_handler_ptr_addr;
	uint32_t *exc_handler_ptr_addr;
	uint32_t *fp_post_proc_addr;

	/*
	 * Constants that the kernel needs
	 */

	uint32_t workspacelength;
	uint32_t contextlength;

	/*
	 * Data pointers for extra information
	 */
	u_char *core_identity_addr;

} arm_fpe_mod_hdr_t;

/* macro to return the FP context for a process */

#define FP_CONTEXT(p) ((u_int)(((u_char *)(p)->p_addr) + sizeof(struct pcb)))

/* Prototypes */

int arm_fpe_boot(void);
int initialise_arm_fpe(void);
void arm_fpe_postproc(u_int fpframe, struct trapframe *frame);
void arm_fpe_exception(int exception, u_int fpframe, struct trapframe *frame);

void arm_fpe_core_disable(void);
void arm_fpe_core_enable(void);
u_int arm_fpe_core_initws(u_int workspace, int handler1, int handler2);
u_int arm_fpe_core_abort(u_int context, int r12, int pc);
void arm_fpe_core_initcontext(u_int context);
u_int arm_fpe_core_changecontext(u_int context);
void arm_fpe_core_shutdown(void);
void arm_fpe_core_activatecontext(u_int context);
u_int arm_fpe_core_deactivatecontext(void);
u_int arm_fpe_core_savecontext(u_int context, fp_context_frame_t *savearea, int pc);
void arm_fpe_core_loadcontext(u_int context, const fp_context_frame_t *loadarea);
void arm_fpe_copycontext(u_int c1, u_int c2);
void arm_fpe_getcontext(struct proc *p, struct fpreg *fpregs);
void arm_fpe_setcontext(struct proc *p, const struct fpreg *fpregs);

void arm_fpe_exception_glue(int exception);
void arm_fpe_panic(void);
void undefined_entry(void);
void arm_fpe_post_proc_glue(void);
void arm_fpe_set_exception_mask(fp_except);

#endif	/* _KERNEL */
#endif /* _ARMFPE_H_ */
