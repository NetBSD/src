/*	$NetBSD: pthread_stack.c,v 1.5 2003/01/19 20:58:01 thorpej Exp $	*/

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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/mman.h>

#include <sched.h>
#include "pthread.h"
#include "pthread_int.h"

static pthread_t
pthread__stackid_setup(void *base, int size);


/* Allocate a stack for a thread, and set it up. It needs to be aligned, so 
 * that a thread can find itself by its stack pointer. 
 */
int
pthread__stackalloc(pthread_t *newt)
{
	void *addr;
	int ret;
	vaddr_t a, b, c, d;

	/* The mmap() interface doesn't let us specify alignment,
	 * so we work around it by mmap()'ing twice the needed space,
	 * then unmapping the unaligned stuff on the edges.
	 */

	addr = mmap(NULL, 2 * PT_STACKSIZE, PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, (off_t)0);
	if (addr == MAP_FAILED)
		return ENOMEM;

	a = (vaddr_t) addr;

	if ((a & PT_STACKMASK) != 0) {
		b = (a & ~PT_STACKMASK) + PT_STACKSIZE;
		ret = munmap((void *)a, (size_t)(b-a));
		if (ret == -1)
			return ENOMEM;
	} else {
		b = a;
	}

	c = b + PT_STACKSIZE;
	d = a + 2*PT_STACKSIZE;

	ret = munmap((void *)c, (size_t)(d-c));
	if (ret == -1)
		return ENOMEM;

	*newt = pthread__stackid_setup((void *)b, PT_STACKSIZE); 
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

	base = (void *) (pthread__sp() & ~PT_STACKMASK);

	*newt = pthread__stackid_setup(base, PT_STACKSIZE);
}

static pthread_t
/*ARGSUSED*/
pthread__stackid_setup(void *base, int size)
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
