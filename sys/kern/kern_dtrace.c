/*	$NetBSD: kern_dtrace.c,v 1.1 2010/02/21 07:28:51 darran Exp $	*/

/*-
 * Copyright (c) 2007-2008 John Birrell <jb@FreeBSD.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_dtrace.c,v 1.1 2010/02/21 07:28:51 darran Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/proc.h>
#include <sys/dtrace_bsd.h>

#define KDTRACE_PROC_SIZE	64
#define KDTRACE_PROC_ZERO	8
#define	KDTRACE_THREAD_SIZE	256
#define	KDTRACE_THREAD_ZERO	64

/* Return the DTrace process data size compiled in the kernel hooks. */
size_t
kdtrace_proc_size()
{

	return(KDTRACE_PROC_SIZE);
}

void
kdtrace_proc_ctor(void *arg, struct proc *p)
{

	p->p_dtrace = kmem_alloc(KDTRACE_PROC_SIZE, KM_SLEEP);
	memset(p->p_dtrace, 0, KDTRACE_PROC_ZERO);
}

void
kdtrace_proc_dtor(void *arg, struct proc *p)
{

	if (p->p_dtrace != NULL) {
		kmem_free(p->p_dtrace, KDTRACE_PROC_SIZE);
		p->p_dtrace = NULL;
	}
}

/* Return the DTrace thread data size compiled in the kernel hooks. */
size_t
kdtrace_thread_size()
{

	return(KDTRACE_THREAD_SIZE);
}

void
kdtrace_thread_ctor(void *arg, struct lwp *l)
{

	l->l_dtrace = kmem_alloc(KDTRACE_THREAD_SIZE, KM_SLEEP);
	memset(l->l_dtrace, 0, KDTRACE_THREAD_ZERO);
}

void
kdtrace_thread_dtor(void *arg, struct lwp *l)
{

	if (l->l_dtrace != NULL) {
		kmem_free(l->l_dtrace, KDTRACE_THREAD_SIZE);
		l->l_dtrace = NULL;
	}
}
