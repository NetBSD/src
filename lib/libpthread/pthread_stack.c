/* $Id: pthread_stack.c,v 1.1.2.2 2001/07/13 02:20:27 nathanw Exp $ */

/* Copyright */


#include <assert.h>
#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/queue.h>
#include <sys/mman.h>

#include "sched.h"
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

	addr = mmap(0, 2 * PT_STACKSIZE, PROT_READ|PROT_WRITE,
	    MAP_ANON|MAP_PRIVATE, -1, 0);
	if (addr == MAP_FAILED)
		return ENOMEM;

	a = (vaddr_t) addr;

	if ((a & PT_STACKMASK) != 0) {
		b = (a & ~PT_STACKMASK) + PT_STACKSIZE;
		ret = munmap((void *)a,  b-a);
		if (ret == -1)
			return ENOMEM;
	} else {
		b = a;
	}

	c = b + PT_STACKSIZE;
	d = a + 2*PT_STACKSIZE;

	ret = munmap((void *)c, d-c);
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
pthread__stackid_setup(void *base, int size)
{
	pthread_t t;
	int pagesize, ret;
	void *addr;

	/* Deallocate the bottom page but one as a red zone. */
	/* XXX assumes that the stack grows down. */
	pagesize = sysconf(_SC_PAGESIZE);
	ret = munmap( ((char *)base + pagesize), pagesize);
	if (ret == -1)
		err(2, "Couldn't munmap()-protect stack redzone at %p\n",
		    (char *)base + pagesize);
	/* Put a pointer to the pthread in the bottom (but
         *  redzone-protected section) of the stack. 
	 */
	t = base;
	
	t->pt_stack.ss_sp = (char *)base + 2 * pagesize;
	t->pt_stack.ss_size = PT_STACKSIZE - 2 * pagesize;

	/* Set up an initial ucontext pointer to a "safe" area */
	t->pt_uc =(ucontext_t *) ((char *)t->pt_stack.ss_sp + 
	    t->pt_stack.ss_size - (pagesize/2));

	return t;
}


