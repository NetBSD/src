/*	$NetBSD: kern_xxx.c,v 1.74 2017/10/28 00:37:11 pgoyette Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)kern_xxx.c	8.3 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_xxx.c,v 1.74 2017/10/28 00:37:11 pgoyette Exp $");

#ifdef _KERNEL_OPT
#include "opt_syscall_debug.h"
#include "opt_kernhist.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/syscallargs.h>
#include <sys/kauth.h>
#include <sys/kernhist.h>

/* ARGSUSED */
int
sys_reboot(struct lwp *l, const struct sys_reboot_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) opt;
		syscallarg(char *) bootstr;
	} */
	int error;
	char *bootstr, bs[128];

	if ((error = kauth_authorize_system(l->l_cred, KAUTH_SYSTEM_REBOOT,
	    0, NULL, NULL, NULL)) != 0)
		return (error);

	/*
	 * Only use the boot string if RB_STRING is set.
	 */
	if ((SCARG(uap, opt) & RB_STRING) &&
	    (error = copyinstr(SCARG(uap, bootstr), bs, sizeof(bs), 0)) == 0)
		bootstr = bs;
	else
		bootstr = NULL;
	/*
	 * Not all ports use the bootstr currently.
	 */
	KERNEL_LOCK(1, NULL);
	cpu_reboot(SCARG(uap, opt), bootstr);
	KERNEL_UNLOCK_ONE(NULL);
	return (0);
}

/*
 * Pull in the indirect syscall functions here.
 * They are only actually used if the ports syscall entry code
 * doesn't special-case SYS_SYSCALL and SYS___SYSCALL
 *
 * In some cases the generated code for the two functions is identical,
 * but there isn't a MI way of determining that - so we don't try.
 */

#define SYS_SYSCALL sys_syscall
#include "sys_syscall.c"
#undef SYS_SYSCALL

#define SYS_SYSCALL sys___syscall
#include "sys_syscall.c"
#undef SYS_SYSCALL

#ifdef SYSCALL_DEBUG
#define	SCDEBUG_CALLS		0x0001	/* show calls */
#define	SCDEBUG_RETURNS		0x0002	/* show returns */
#define	SCDEBUG_ALL		0x0004	/* even syscalls that are not implemented */
#define	SCDEBUG_SHOWARGS	0x0008	/* show arguments to calls */
#define	SCDEBUG_KERNHIST	0x0010	/* use kernhist instead of printf */

#ifndef SCDEBUG_DEFAULT
#define SCDEBUG_DEFAULT (SCDEBUG_CALLS|SCDEBUG_RETURNS|SCDEBUG_SHOWARGS)
#endif

int	scdebug = SCDEBUG_DEFAULT;

#ifdef KERNHIST
KERNHIST_DEFINE(scdebughist);
#define SCDEBUG_KERNHIST_FUNC(a)		KERNHIST_FUNC(a)
#define SCDEBUG_KERNHIST_CALLED(a)		KERNHIST_CALLED(a)
#define SCDEBUG_KERNHIST_LOG(a,b,c,d,e,f)	KERNHIST_LOG(a,b,c,d,e,f)
#else
#define SCDEBUG_KERNHIST_FUNC(a)		/* nothing */
#define SCDEBUG_KERNHIST_CALLED(a)		/* nothing */
#define SCDEBUG_KERNHIST_LOG(a,b,c,d,e,f)	/* nothing */
/* The non-kernhist support version can elide all this code easily. */
#undef	SCDEBUG_KERNHIST
#define	SCDEBUG_KERNHIST 0
#endif

#ifdef __HAVE_MINIMAL_EMUL
#define CODE_NOT_OK(code, em)	((int)(code) < 0)
#else
#define CODE_NOT_OK(code, em)	(((int)(code) < 0) || \
				 ((int)(code) >= (em)->e_nsysent))
#endif

void
scdebug_call(register_t code, const register_t args[])
{
	SCDEBUG_KERNHIST_FUNC("scdebug_call");
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	const struct sysent *sy;
	const struct emul *em;
	int i;

	if ((scdebug & SCDEBUG_CALLS) == 0)
		return;

	if (scdebug & SCDEBUG_KERNHIST)
		SCDEBUG_KERNHIST_CALLED(scdebughist);

	em = p->p_emul;
	sy = &em->e_sysent[code];

	if ((scdebug & SCDEBUG_ALL) == 0 &&
	    (CODE_NOT_OK(code, em) || sy->sy_call == sys_nosys)) {
		if (scdebug & SCDEBUG_KERNHIST)
			SCDEBUG_KERNHIST_LOG(scdebughist, "", 0, 0, 0, 0);
		return;
	}

	/*
	 * The kernhist version of scdebug needs to restrict the usage
	 * compared to the normal version.  histories must avoid these
	 * sorts of usage:
	 *
	 *	- the format string *must* be literal, as it is used
	 *	  at display time in the kernel or userland
	 *	- strings in the format will cause vmstat -u to crash
	 *	  so avoid using %s formats
	 *
	 * to avoid these, we have a fairly long block to print args
	 * as the format needs to change for each, and we can't just
	 * call printf() on each argument until we're done.
	 */
	if (scdebug & SCDEBUG_KERNHIST) {
		if (CODE_NOT_OK(code, em)) {
			SCDEBUG_KERNHIST_LOG(scdebughist, 
			    "pid %jd:%jd: OUT OF RANGE (%jd)",
			    p->p_pid, l->l_lid, code, 0);
		} else {
			SCDEBUG_KERNHIST_LOG(scdebughist,
			    "pid %jd:%jd: num %jd call %#jx",
			    p->p_pid, l->l_lid, code, (uintptr_t)sy->sy_call);
			if ((scdebug & SCDEBUG_SHOWARGS) == 0)
				return;

			if (sy->sy_narg > 7) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[4-7]: (%jx, %jx, %jx, %jx, ...)",
				    (long)args[4], (long)args[5],
				    (long)args[6], (long)args[7]);
			} else if (sy->sy_narg > 6) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[4-6]: (%jx, %jx, %jx)",
				    (long)args[4], (long)args[5],
				    (long)args[6], 0);
			} else if (sy->sy_narg > 5) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[4-5]: (%jx, %jx)",
				    (long)args[4], (long)args[5], 0, 0);
			} else if (sy->sy_narg == 5) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[4]: (%jx)",
				    (long)args[4], 0, 0, 0);
			}

			if (sy->sy_narg > 3) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[0-3]: (%jx, %jx, %jx, %jx, ...)",
				    (long)args[0], (long)args[1],
				    (long)args[2], (long)args[3]);
			} else if (sy->sy_narg > 2) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[0-2]: (%jx, %jx, %jx)",
				    (long)args[0], (long)args[1],
				    (long)args[2], 0);
			} else if (sy->sy_narg > 1) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[0-1]: (%jx, %jx)",
				    (long)args[0], (long)args[1], 0, 0);
			} else if (sy->sy_narg == 1) {
				SCDEBUG_KERNHIST_LOG(scdebughist,
				    "args[0]: (%jx)",
				    (long)args[0], 0, 0, 0);
			}
		}
		return;
	}

	printf("proc %d (%s): %s num ", p->p_pid, p->p_comm, em->e_name);
	if (CODE_NOT_OK(code, em))
		printf("OUT OF RANGE (%ld)", (long)code);
	else {
		printf("%ld call: %s", (long)code, em->e_syscallnames[code]);
		if (scdebug & SCDEBUG_SHOWARGS) {
			printf("(");
			for (i = 0; i < sy->sy_argsize/sizeof(register_t); i++)
				printf("%s0x%lx", i == 0 ? "" : ", ",
				    (long)args[i]);
			printf(")");
		}
	}
	printf("\n");
}

void
scdebug_ret(register_t code, int error, const register_t retval[])
{
	SCDEBUG_KERNHIST_FUNC("scdebug_ret");
	struct lwp *l = curlwp;
	struct proc *p = l->l_proc;
	const struct sysent *sy;
	const struct emul *em;

	if ((scdebug & SCDEBUG_RETURNS) == 0)
		return;

	if (scdebug & SCDEBUG_KERNHIST)
		SCDEBUG_KERNHIST_CALLED(scdebughist);

	em = p->p_emul;
	sy = &em->e_sysent[code];
	if ((scdebug & SCDEBUG_ALL) == 0 &&
	    (CODE_NOT_OK(code, em) || sy->sy_call == sys_nosys)) {
		if (scdebug & SCDEBUG_KERNHIST)
			SCDEBUG_KERNHIST_LOG(scdebughist, "", 0, 0, 0, 0);
		return;
	}

	if (scdebug & SCDEBUG_KERNHIST) {
		if (CODE_NOT_OK(code, em)) {
			SCDEBUG_KERNHIST_LOG(scdebughist, 
			    "pid %jd:%jd: OUT OF RANGE (%jd)",
			    p->p_pid, l->l_lid, code, 0);
		} else {
			SCDEBUG_KERNHIST_LOG(scdebughist,
			    "pid %jd:%jd: num %jd",
			    p->p_pid, l->l_lid, code, 0);
			SCDEBUG_KERNHIST_LOG(scdebughist,
			    "ret: err = %jd, rv = 0x%jx,0x%jx",
			    error, (long)retval[0], (long)retval[1], 0);
		}
		return;
	}

	printf("proc %d (%s): %s num ", p->p_pid, p->p_comm, em->e_name);
	if (CODE_NOT_OK(code, em))
		printf("OUT OF RANGE (%ld)", (long)code);
	else
		printf("%ld ret %s: err = %d, rv = 0x%lx,0x%lx", (long)code,
		    em->e_syscallnames[code], error,
		    (long)retval[0], (long)retval[1]);
	printf("\n");
}
#endif /* SYSCALL_DEBUG */

#ifndef SCDEBUG_KERNHIST_SIZE
#define SCDEBUG_KERNHIST_SIZE 500
#endif

void
scdebug_init(void)
{
#if defined(SYSCALL_DEBUG) && defined(KERNHIST)
	/* Setup scdebughist kernel history */
	KERNHIST_INIT(scdebughist, SCDEBUG_KERNHIST_SIZE);
#endif
}
