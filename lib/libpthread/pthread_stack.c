/*	$NetBSD: pthread_stack.c,v 1.10 2004/01/02 19:14:00 cl Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: pthread_stack.c,v 1.10 2004/01/02 19:14:00 cl Exp $");

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

static pthread_t
pthread__stackid_setup(void *base, size_t size);

#ifndef PT_FIXEDSTACKSIZE_LG
/* 
 * We have to initialize the pt_stack* variables here because mutexes
 * are used before pthread_init() and thus pthread__initmain() are
 * called.  Since mutexes only save the stack pointer and not a
 * pointer to the thread data, it is safe to change the mapping from
 * stack pointer to thread data afterwards.
 */
#define	_STACKSIZE_LG 18
int	pt_stacksize_lg = _STACKSIZE_LG;
size_t	pt_stacksize = 1 << _STACKSIZE_LG;
vaddr_t	pt_stackmask = (1 << _STACKSIZE_LG) - 1;
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

	*newt = pthread__stackid_setup(addr, PT_STACKSIZE); 
	return 0;
}


/*
 * Set up the slightly special stack for the "initial" thread, which
 * runs on the normal system stack, and thus gets slightly different
 * treatment.
 */
void
pthread__initmain(pthread_t *newt)
{
	void *base;

#ifndef PT_FIXEDSTACKSIZE_LG
	struct rlimit slimit;
	size_t pagesize;
	char *value;
	int ret;

	pagesize = (size_t)sysconf(_SC_PAGESIZE);
	pt_stacksize = 0;
	ret = getrlimit(RLIMIT_STACK, &slimit);
	if (ret == -1)
		err(1, "Couldn't get stack resource consumption limits");
	value = getenv("PTHREAD_STACKSIZE");
	if (value) {
		pt_stacksize = atoi(value) * 1024;
		if (pt_stacksize > slimit.rlim_cur)
			pt_stacksize = (size_t)slimit.rlim_cur;
	}
	if (pt_stacksize == 0)
		pt_stacksize = (size_t)slimit.rlim_cur;
	if (pt_stacksize < 4 * pagesize)
		errx(1, "Stacksize limit is too low, minimum %zd kbyte.",
		    4 * pagesize / 1024);

	pt_stacksize_lg = -1;
	while (pt_stacksize) {
		pt_stacksize >>= 1;
		pt_stacksize_lg++;
	}

	pt_stacksize = (1 << pt_stacksize_lg);
	pt_stackmask = pt_stacksize - 1;

	/*
	 * XXX The "initial" thread stack can be smaller than
	 * requested because we don't control the end of the stack.
	 * On i386 the stack usually ends at 0xbfc00000 and for
	 * requested sizes >=8MB, we get a 4MB smaller stack.
	 */
#endif /* PT_FIXEDSTACKSIZE_LG */

	base = (void *) (pthread__sp() & ~PT_STACKMASK);

	*newt = pthread__stackid_setup(base, PT_STACKSIZE);
}

static pthread_t
/*ARGSUSED*/
pthread__stackid_setup(void *base, size_t size)
{
	pthread_t t;
	size_t pagesize;
	int ret;

	/* Protect the next-to-bottom stack page as a red zone. */
	/* XXX assumes that the stack grows down. */
	pagesize = (size_t)sysconf(_SC_PAGESIZE);
#ifdef __hppa__
	#error "stack does not grow down"
#endif
	ret = mprotect((char *)base + pagesize, pagesize, PROT_NONE);
	if (ret == -1)
		err(2, "Couldn't mprotect() stack redzone at %p\n",
		    (char *)base + pagesize);
	/*
	 * Put a pointer to the pthread in the bottom (but
         * redzone-protected section) of the stack. 
	 */
	t = base;
	
	t->pt_stack.ss_sp = (char *)base + 2 * pagesize;
	t->pt_stack.ss_size = PT_STACKSIZE - 2 * pagesize;

	/* Set up an initial ucontext pointer to a "safe" area */
	t->pt_uc =(ucontext_t *)(void *)((char *)t->pt_stack.ss_sp + 
	    t->pt_stack.ss_size - (pagesize/2));
#ifdef _UC_UCONTEXT_ALIGN
	t->pt_uc = (ucontext_t *)((uintptr_t)t->pt_uc & _UC_UCONTEXT_ALIGN);
#endif

	return t;
}


ssize_t
pthread__stackinfo_offset()
{
	size_t pagesize;

	pagesize = (size_t)sysconf(_SC_PAGESIZE);
	return (-(2 * pagesize) +
	    offsetof(struct __pthread_st, pt_stackinfo));
}
