/*	$NetBSD: kern_kthread.c,v 1.16.6.2 2007/04/10 12:07:13 ad Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Andrew Doran.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_kthread.c,v 1.16.6.2 2007/04/10 12:07:13 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/wait.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

/*
 * note that stdarg.h and the ansi style va_start macro is used for both
 * ansi and traditional c complers.
 * XXX: this requires that stdarg.h define: va_alist and va_dcl
 */
#include <machine/stdarg.h>

/*
 * Fork a kernel thread.  Any process can request this to be done.
 */
int
kthread_create(pri_t pri, bool mpsafe, void (*func)(void *), void *arg,
	       struct lwp **lp, const char *fmt, ...)
{
	struct lwp *l;
	vaddr_t uaddr;
	bool inmem;
	int error;
	va_list ap;

	inmem = uvm_uarea_alloc(&uaddr);
	if (uaddr == 0)
		return ENOMEM;
	error = newlwp(&lwp0, &proc0, uaddr, inmem, LWP_DETACHED, NULL, 0,
	    func, arg, &l);
	if (error) {
		/* XXX uvm_uarea_free(uaddr); */
		return error;
	}

	/* Set parameters. */
	if (pri == PRI_NONE) {
		/* Minimum kernel priority level. */
		pri = PUSER - 1;
	}
	if (mpsafe)
		l->l_pflag |= LP_MPSAFE;
	if (fmt != NULL) {
		va_start(ap, fmt);
		vsnprintf(l->l_name, MAXCOMLEN, fmt, ap);
		va_end(ap);
	}

	/* Set the new LWP running. */
	mutex_enter(&proc0.p_smutex);
	lwp_lock(l);
	l->l_usrpri = pri;
	l->l_priority = pri;
	l->l_stat = LSRUN;
	proc0.p_nrlwps++;
	setrunqueue(l);
	lwp_unlock(l);
	mutex_exit(&proc0.p_smutex);

	/* All done! */
	if (lp != NULL)
		*lp = l;

	return (0);
}

/*
 * Cause a kernel thread to exit.  Assumes the exiting thread is the
 * current context.
 */
void
kthread_exit(int ecode)
{
	struct lwp *l = curlwp;

	/* We can't do much with the exit code, so just report it. */
	if (ecode != 0)
		printf("WARNING: kthread `%s' (%d) exits with status %d\n",
		    l->l_name, l->l_lid, ecode);

	/* And exit.. */
	lwp_exit(l);

	/*
	 * XXX Fool the compiler.  Making exit1() __noreturn__ is a can
	 * XXX of worms right now.
	 */
	for (;;)
		;
}
