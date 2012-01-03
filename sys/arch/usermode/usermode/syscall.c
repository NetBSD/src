/* $NetBSD: syscall.c,v 1.20 2012/01/03 12:05:00 reinoud Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: syscall.c,v 1.20 2012/01/03 12:05:00 reinoud Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/sched.h>
#include <sys/ktrace.h>
#include <sys/syscall.h>
#include <sys/syscallvar.h>
#include <sys/syscallargs.h>

#include <sys/userret.h>
#include <machine/pcb.h>
#include <machine/thunk.h>
#include <machine/machdep.h>


void userret(struct lwp *l);

void
userret(struct lwp *l)
{
	/* invoke MI userret code */
	mi_userret(l);
}

void
child_return(void *arg)
{
	lwp_t *l = arg;
	register_t rval[2];
	struct pcb *pcb = lwp_getpcb(l);

	/* return value zero */
	rval[0] = 0;
	rval[1] = 0;
	md_syscall_set_returnargs(l, &pcb->pcb_userret_ucp, 0, rval);

	aprint_debug("child return! lwp %p\n", l);
	userret(l);
	ktrsysret(SYS_fork, 0, 0);
}

extern const char *const syscallnames[];

static void syscall_args_print(lwp_t *l, int code, int nargs, int argsize,
	register_t *args);
static void syscall_retvals_print(lwp_t *l, lwp_t *clwp,
	int code, int nargs, register_t *args, int error, register_t *rval);


void
syscall(void)
{	
	lwp_t *l = curlwp;
	const struct proc * const p = l->l_proc;
	const struct sysent *callp;
	struct pcb *pcb = lwp_getpcb(l);
	ucontext_t *ucp = &pcb->pcb_userret_ucp;
	register_t copyargs[2+SYS_MAXSYSARGS];
	register_t *args;
	register_t rval[2];
	uint32_t code, opcode;
	uint nargs, argsize;
	int error;

	/* system call accounting */
	curcpu()->ci_data.cpu_nsyscall++;
	LWP_CACHE_CREDS(l, l->l_proc);

	/* TODO are we allowed to execute system calls in this memory space? */

	/* XXX do we want do do emulation? */
	md_syscall_get_opcode(ucp, &opcode);
	md_syscall_get_syscallnumber(ucp, &code);
	code &= (SYS_NSYSENT -1);

	callp   = p->p_emul->e_sysent + code;
	nargs   = callp->sy_narg;
	argsize = callp->sy_argsize;

	args  = copyargs;
	rval[0] = rval[1] = 0;
	error = md_syscall_getargs(l, ucp, nargs, argsize, args);

#if 0
	aprint_debug("syscall no. %d, ", code);
	aprint_debug("nargs %d, argsize %d =>  ", nargs, argsize);
	thunk_printf_debug("syscall no. %d, ", code);
	thunk_printf_debug("nargs %d, argsize %d =>  ", nargs, argsize);
#endif

	/*
	 * TODO change the pre and post printing into functions so they can be
	 * easily adjusted and dont clobber up this space
	 */

	if (!error)
		syscall_args_print(l, code, nargs, argsize, args);

	md_syscall_inc_pc(ucp, opcode);

	if (!error) {
		if (!__predict_false(p->p_trace_enabled)
		    || __predict_false(callp->sy_flags & SYCALL_INDIRECT)
		    || (error = trace_enter(code, args, callp->sy_narg)) == 0) {
			error = (*callp->sy_call)(l, args, rval);
		}

		if (__predict_false(p->p_trace_enabled)
		    && !__predict_false(callp->sy_flags & SYCALL_INDIRECT)) {
			trace_exit(code, rval, error);
		}
	}

	syscall_retvals_print(l, curlwp, code, nargs, args, error, rval);

//out:
	switch (error) {
	default:
		/* fall trough */
	case 0:
		md_syscall_set_returnargs(l, ucp, error, rval);
		/* fall trough */
	case EJUSTRETURN:
		break;
	case ERESTART:
		md_syscall_dec_pc(ucp, opcode);
		/* nothing to do */
		break;
	}
	//thunk_printf_debug("end of syscall : return to userland\n");
//if (code != 4) printf("userret() code %d\n", code);
	userret(l);
}


static void
syscall_args_print(lwp_t *l, int code, int nargs, int argsize, register_t *args)
{
	char **argv, **envp;

return;
	if (code != 4) {
		printf("lwp %p, code %3d, nargs %d, argsize %3d\t%s(", 
			l, code, nargs, argsize, syscallnames[code]);
		switch (code) {
		case 5:
			printf("\"%s\", %"PRIx32", %"PRIx32"", (char *) (args[0]), (uint) args[1], (uint) args[2]);
			break;
		case 33:
			printf("\"%s\", %"PRIx32"", (char *) (args[0]), (uint) args[1]);
			break;
		case 50:
			printf("\"%s\"", (char *) (args[0]));
			break;
		case 58:
			printf("\"%s\", %"PRIx32", %"PRIx32"", (char *) (args[0]), (uint) (args[1]), (uint) args[2]);
			break;
		case 59:
			printf("\"%s\", [", (char *) (args[0]));
			argv = (char **) (args[1]);
			if (*argv) {
				while (*argv) {
					printf("\"%s\", ", *argv);
					argv++;
				}
				printf("\b\b");
			}
			printf("], [");
			envp = (char **) (args[2]);
			if (*envp) {
				while (*envp) {
					printf("\"%s\", ", *envp);
					envp++;
				}
				printf("\b\b");
			}
			printf("]");
			break;
		default:
			for (int i = 0; i < nargs; i++)
				printf("%"PRIx32", ", (uint) args[i]);
			if (nargs)
				printf("\b\b");
		}
		printf(") ");
	}
#if 0
	if ((code == 4)) {
//		thunk_printf_debug("[us] %s", (char *) args[1]);
		printf("[us] %s", (char *) args[1]);
	}
#endif
}


static void
syscall_retvals_print(lwp_t *l, lwp_t *clwp, int code, int nargs, register_t *args, int error, register_t *rval)
{
	char const *errstr;

return;
	switch (error) {
	case EJUSTRETURN:
		errstr = "EJUSTRETURN";
		break;
	case EAGAIN:
		errstr = "EGAIN";
		break;
	default:
		errstr = "OK";
	}
	if (code != 4)
		printf("=> %s: %d, (%"PRIx32", %"PRIx32")\n",
			errstr, error, (uint) (rval[0]), (uint) (rval[1]));
}

