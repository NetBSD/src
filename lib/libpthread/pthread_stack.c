/*	$NetBSD: pthread_stack.c,v 1.21 2007/08/16 00:41:24 ad Exp $	*/

/*-
 * Copyright (c) 2001, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nathan J. Williams.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__RCSID("$NetBSD: pthread_stack.c,v 1.21 2007/08/16 00:41:24 ad Exp $");

#define __EXPOSE_STACK 1
#include <sys/param.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/resource.h>

#include <sched.h>
#include "pthread.h"
#include "pthread_int.h"

static int pthread__stackid_setup(void *, size_t, pthread_t *);

#ifndef PT_FIXEDSTACKSIZE_LG
/* 
 * We have to initialize the pthread_stack* variables here because
 * mutexes are used before pthread_init() and thus pthread__initmain()
 * are called.  Since mutexes only save the stack pointer and not a
 * pointer to the thread data, it is safe to change the mapping from
 * stack pointer to thread data afterwards.
 */
#define	_STACKSIZE_LG 18
int	pthread_stacksize_lg = _STACKSIZE_LG;
size_t	pthread_stacksize = 1 << _STACKSIZE_LG;
vaddr_t	pthread_stackmask = (1 << _STACKSIZE_LG) - 1;
#undef	_STACKSIZE_LG
#endif /* !PT_FIXEDSTACKSIZE_LG */


/*
 * Allocate a stack for a thread, and set it up. It needs to be aligned, so 
 * that a thread can find itself by its stack pointer. 
 */
int
pthread__stackalloc(pthread_t *newt)
{
	void *addr;

	addr = mmap(NULL, PT_STACKSIZE, PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE | MAP_ALIGNED(PT_STACKSIZE_LG), -1, (off_t)0);

	if (addr == MAP_FAILED)
		return ENOMEM;

	pthread__assert(((intptr_t)addr & PT_STACKMASK) == 0);

	return pthread__stackid_setup(addr, PT_STACKSIZE, newt); 
}


/*
 * Set up the slightly special stack for the "initial" thread, which
 * runs on the normal system stack, and thus gets slightly different
 * treatment.
 */
void
pthread__initmain(pthread_t *newt)
{
	pthread_t t;
	void *base;
	size_t size;
	int error;

#ifndef PT_FIXEDSTACKSIZE_LG
	struct rlimit slimit;
	size_t pagesize;
	char *value;
	int ret;

	pagesize = (size_t)sysconf(_SC_PAGESIZE);
	pthread_stacksize = 0;
	ret = getrlimit(RLIMIT_STACK, &slimit);
	if (ret == -1)
		err(1, "Couldn't get stack resource consumption limits");
	value = getenv("PTHREAD_STACKSIZE");
	if (value) {
		pthread_stacksize = atoi(value) * 1024;
		if (pthread_stacksize > slimit.rlim_cur)
			pthread_stacksize = (size_t)slimit.rlim_cur;
	}
	if (pthread_stacksize == 0)
		pthread_stacksize = (size_t)slimit.rlim_cur;
	if (pthread_stacksize < 4 * pagesize)
		errx(1, "Stacksize limit is too low, minimum %zd kbyte.",
		    4 * pagesize / 1024);

	pthread_stacksize_lg = -1;
	while (pthread_stacksize) {
		pthread_stacksize >>= 1;
		pthread_stacksize_lg++;
	}

	pthread_stacksize = (1 << pthread_stacksize_lg);
	pthread_stackmask = pthread_stacksize - 1;
#endif /* PT_FIXEDSTACKSIZE_LG */

	base = (void *)(pthread__sp() & ~PT_STACKMASK);
	size = PT_STACKSIZE;

	error = pthread__stackid_setup(base, size, &t);
	if (error) {
		/* XXX */
		errx(2, "failed to setup main thread: error=%d", error);
	}

	*newt = t;
}

static int
/*ARGSUSED*/
pthread__stackid_setup(void *base, size_t size, pthread_t *tp)
{
	pthread_t t;
	void *redaddr;
	size_t pagesize;
	caddr_t sp;
	int ret;

	t = base;
	pagesize = (size_t)sysconf(_SC_PAGESIZE);

	/*
	 * Put a pointer to the pthread in the bottom (but
         * redzone-protected section) of the stack. 
	 */

	redaddr = STACK_SHRINK(STACK_MAX(base, size), pagesize);
	t->pt_stack.ss_size = size - 2 * pagesize;
#ifdef __MACHINE_STACK_GROWS_UP
	t->pt_stack.ss_sp = (char *)(void *)base + pagesize;
	sp = t->pt_stack.ss_sp;
#else
	t->pt_stack.ss_sp = (char *)(void *)base + 2 * pagesize;
	sp = (caddr_t)t->pt_stack.ss_sp + t->pt_stack.ss_size;
#endif

	/* Protect the next-to-bottom stack page as a red zone. */
	ret = mprotect(redaddr, pagesize, PROT_NONE);
	if (ret == -1) {
		return errno;
	}
	*tp = t;
	return 0;
}
