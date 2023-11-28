/*	$NetBSD: t_stack.c,v 1.6.2.2 2023/11/28 13:17:11 martin Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

#define	_KMEMUSER		/* __MACHINE_STACK_GROWS_UP */

#include <sys/cdefs.h>
__RCSID("$NetBSD: t_stack.c,v 1.6.2.2 2023/11/28 13:17:11 martin Exp $");

#include <sys/mman.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <uvm/uvm_param.h>	/* VM_THREAD_GUARD_SIZE */

#include <atf-c.h>
#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "h_macros.h"

struct jmp_ctx {
	jmp_buf buf;
};

/*
 * State used by various tests.
 */
struct ctx {
	size_t size;		/* default stack size */
	size_t guardsize;	/* default guard size */
	void *addr;		/* user-allocated stack */
	pthread_key_t jmp_key;	/* jmp_ctx to return from SIGSEGV handler */
} ctx, *C = &ctx;

/*
 * getdefaultstacksize()
 *
 *	Return the default stack size for threads created with
 *	pthread_create.
 */
static size_t
getdefaultstacksize(void)
{
	pthread_attr_t attr;
	size_t stacksize;

	/*
	 * When called from the main thread, this returns the default
	 * stack size (pthread__stacksize) used for pthreads.
	 */
	RZ(pthread_getattr_np(pthread_self(), &attr));
	RZ(pthread_attr_getstacksize(&attr, &stacksize));
	RZ(pthread_attr_destroy(&attr));

	/*
	 * Verify that the assumption above holds.
	 */
	extern size_t pthread__stacksize; /* pthread_int.h */
	ATF_CHECK_EQ_MSG(stacksize, pthread__stacksize,
	    "stacksize=%zu pthread__stacksize=%zu",
	    stacksize, pthread__stacksize);

	return stacksize;
}

/*
 * getnondefaultstacksize()
 *
 *	Return a stack size that is not the default stack size for
 *	threads created with pthread_create.
 */
static size_t
getnondefaultstacksize(void)
{

	return getdefaultstacksize() + sysconf(_SC_PAGESIZE);
}

/*
 * getdefaultguardsize()
 *
 *	Return the default guard size for threads created with
 *	pthread_create.
 */
static size_t
getdefaultguardsize(void)
{
	const int mib[2] = { CTL_VM, VM_THREAD_GUARD_SIZE };
	unsigned guardsize;
	size_t len = sizeof(guardsize);

	RL(sysctl(mib, __arraycount(mib), &guardsize, &len, NULL, 0));
	ATF_REQUIRE_EQ_MSG(len, sizeof(guardsize),
	    "len=%zu sizeof(guardsize)=%zu", len, sizeof(guardsize));

	/*
	 * Verify this matches what libpthread determined.
	 */
	extern size_t pthread__guardsize; /* pthread_int.h */
	ATF_CHECK_EQ_MSG(guardsize, pthread__guardsize,
	    "guardsize=%u pthread__guardsize=%zu",
	    guardsize, pthread__guardsize);

	return guardsize;
}

/*
 * alloc(nbytes)
 *
 *	Allocate an nbytes-long page-aligned read/write region and
 *	return a pointer to it.  Abort the test if allocation fails, so
 *	if this function returns it succeeds.
 */
static void *
alloc(size_t nbytes)
{
	void *ptr;

	REQUIRE_LIBC((ptr = mmap(/*hint*/NULL, nbytes,
		    PROT_READ|PROT_WRITE, MAP_ANON, /*fd*/-1, /*offset*/0)),
	    MAP_FAILED);

	return ptr;
}

/*
 * init(stacksize)
 *
 *	Initialize state used by various tests with the specified
 *	stacksize.
 *
 *	Make sure to allocate enough space that even if there shouldn't
 *	be a stack guard (i.e., it should be empty), adjusting the
 *	requested bounds by the default stack guard size will leave us
 *	inside allocated memory.
 */
static void
init(size_t stacksize)
{

	C->size = stacksize;
	C->guardsize = getdefaultguardsize();
	C->addr = alloc(C->size + C->guardsize);
	RZ(pthread_key_create(&C->jmp_key, NULL));
}

/*
 * stack_pointer()
 *
 *	Return the stack pointer.  This is used to verify whether the
 *	stack pointer lie within a certain address range.
 */
static __noinline void *
stack_pointer(void)
{
	return __builtin_frame_address(0);
}

/*
 * sigsegv_ok(signo)
 *
 *	Signal handler for SIGSEGV to return to the jmp ctx, to verify
 *	that SIGSEGV happened without crashing.
 */
static void
sigsegv_ok(int signo)
{
	struct jmp_ctx *j = pthread_getspecific(C->jmp_key);

	longjmp(j->buf, 1);
}

/*
 * checksigsegv(p)
 *
 *	Verify that reading *p triggers SIGSEGV.  Fails test nonfatally
 *	if SIGSEGV doesn't happen.
 */
static void
checksigsegv(const char *p)
{
	struct jmp_ctx j;
	struct sigaction act, oact;
	volatile struct sigaction oactsave;
	volatile char v;

	memset(&act, 0, sizeof(act));
	act.sa_handler = &sigsegv_ok;

	if (setjmp(j.buf) == 0) {
		pthread_setspecific(C->jmp_key, &j);
		RL(sigaction(SIGSEGV, &act, &oact));
		oactsave = oact;
		v = *p;		/* trigger SIGSEGV */
		atf_tc_fail_nonfatal("failed to trigger SIGSEGV at %p", p);
	} else {
		/* return from SIGSEGV handler */
		oact = oactsave;
	}
	RL(sigaction(SIGSEGV, &oact, NULL));
	pthread_setspecific(C->jmp_key, NULL);

	(void)v;		/* suppress unused variable warnings */
}

/*
 * checknosigsegv(p)
 *
 *	Verify that reading *p does not trigger SIGSEGV.  Fails test
 *	nonfatally if SIGSEGV happens.
 */
static void
checknosigsegv(const char *p)
{
	struct jmp_ctx j;
	struct sigaction act, oact;
	volatile struct sigaction oactsave;
	volatile char v;

	memset(&act, 0, sizeof(act));
	act.sa_handler = &sigsegv_ok;

	if (setjmp(j.buf) == 0) {
		pthread_setspecific(C->jmp_key, &j);
		RL(sigaction(SIGSEGV, &act, &oact));
		oactsave = oact;
		v = *p;		/* better not trigger SIGSEGV */
	} else {
		/* return from SIGSEGV handler */
		atf_tc_fail_nonfatal("spuriously triggered SIGSEGV at %p", p);
		oact = oactsave;
	}
	RL(sigaction(SIGSEGV, &oact, NULL));
	pthread_setspecific(C->jmp_key, NULL);

	(void)v;		/* suppress unused variable warnings */
}

/*
 * checkguardaccessthread(cookie)
 *
 *	Thread start routine that verifies it has access to the start
 *	and end of its stack, according to pthread_attr_getstack, and
 *	_does not_ have access to the start or end of its stack guard,
 *	above the stack (in stack growth direction) by
 *	pthread_attr_getguardsize bytes.
 */
static void *
checkguardaccessthread(void *cookie)
{
	pthread_t t = pthread_self();
	pthread_attr_t attr;
	void *addr, *guard;
	size_t size, guardsize;

	/*
	 * Get the the stack and stack guard parameters.
	 */
	RZ(pthread_getattr_np(t, &attr));
	RZ(pthread_attr_getstack(&attr, &addr, &size));
	RZ(pthread_attr_getguardsize(&attr, &guardsize));

	/*
	 * Determine where the guard starts in virtual address space
	 * (not in stack growth direction).
	 */
#ifdef __MACHINE_STACK_GROWS_UP
	guard = (char *)addr + size;
#else
	guard = (char *)addr - guardsize;
#endif

	/*
	 * Verify access to the start and end of the stack itself.
	 */
	checknosigsegv(addr);
	checknosigsegv((char *)addr + size - 1);

	/*
	 * Verify no access to the start or end of the stack guard.
	 */
	checksigsegv(guard);
	checksigsegv((char *)guard + guardsize - 1);

	return NULL;
}

/*
 * checkaddraccessthread(cookie)
 *
 *	Thread start routine that verifies its stack is [C->addr,
 *	C->addr + C->size), according to pthread_attr_getstack and
 *	pthread_addr_getstacksize, and verifies it has access to that
 *	whole range.
 */
static void *
checkaddraccessthread(void *cookie)
{
	pthread_t t = pthread_self();
	pthread_attr_t attr;
	void *sp;
	void *addr;
	size_t size, size0;

	/*
	 * Verify the stack pointer lies somewhere in the allocated
	 * range.
	 */
	sp = stack_pointer();
	ATF_CHECK_MSG(C->addr <= sp, "sp=%p not in [%p,%p + 0x%zu) = [%p,%p)",
	    sp, C->addr, C->addr, C->size, C->addr, (char *)C->addr + C->size);
	ATF_CHECK_MSG(sp <= (void *)((char *)C->addr + C->size),
	    "sp=%p not in [%p,%p + 0x%zu) = [%p,%p)",
	    sp, C->addr, C->addr, C->size, C->addr, (char *)C->addr + C->size);

	/*
	 * Verify, if not that, then the stack pointer at least lies
	 * within the extra buffer we allocated for slop to address a
	 * bug NetBSD libpthread used to have of spuriously adding the
	 * guard size to a user-allocated stack address.  This is
	 * ATF_REQUIRE, not ATF_CHECK, because if this doesn't hold, we
	 * might be clobbering some other memory like malloc pages,
	 * causing the whole test to crash with useless diagnostics.
	 */
	ATF_REQUIRE_MSG(sp <= (void *)((char *)C->addr + C->size +
		C->guardsize),
	    "sp=%p not even in buffer [%p,%p + 0x%zu + 0x%zu) = [%p,%p)",
	    sp, C->addr, C->addr, C->size, C->guardsize,
	    C->addr, (char *)C->addr + C->size + C->guardsize);

	/*
	 * Get the stack parameters -- both via pthread_attr_getstack
	 * and via pthread_attr_getstacksize, to make sure they agree
	 * -- and verify that they are what we expect from the caller.
	 */
	RZ(pthread_getattr_np(t, &attr));
	RZ(pthread_attr_getstack(&attr, &addr, &size));
	RZ(pthread_attr_getstacksize(&attr, &size0));
	ATF_CHECK_EQ_MSG(C->addr, addr, "expected %p actual %p",
	    C->addr, addr);
	ATF_CHECK_EQ_MSG(C->size, size, "expected %zu actual %zu",
	    C->size, size);
	ATF_CHECK_EQ_MSG(C->size, size0, "expected %zu actual %zu",
	    C->size, size0);

	/*
	 * Verify that we have access to what we expect the stack to
	 * be.
	 */
	checknosigsegv(C->addr);
	checknosigsegv((char *)C->addr + C->size - 1);

	return NULL;
}

ATF_TC(stack1);
ATF_TC_HEAD(stack1, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test allocating and reallocating a thread with a user stack");
}
ATF_TC_BODY(stack1, tc)
{
	pthread_attr_t attr;
	pthread_t t, t2;

	/*
	 * Allocate a stack with a non-default size to verify
	 * libpthread didn't choose the stack size for us.
	 */
	init(getnondefaultstacksize());

	/*
	 * Create a thread with user-allocated stack of a non-default
	 * size to verify the stack size and access.
	 */
	RZ(pthread_attr_init(&attr));
	RZ(pthread_attr_setstack(&attr, C->addr, C->size));
	RZ(pthread_create(&t, &attr, &checkaddraccessthread, C));
	RZ(pthread_join(t, NULL));

	/*
	 * Create another thread with the same parameters, and verify
	 * that (a) it was recycled, and (b) it works the same way.
	 */
	RZ(pthread_create(&t2, &attr, &checkaddraccessthread, C));
	ATF_CHECK_EQ_MSG(t, t2, "t=%p t2=%p", t, t2); /* NetBSD recycles */
	RZ(pthread_join(t2, NULL));
}

ATF_TC(stack2);
ATF_TC_HEAD(stack2, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "Test reallocating a thread with a newly self-allocated stack");
}
ATF_TC_BODY(stack2, tc)
{
	pthread_attr_t attr, attr2;
	size_t size, size2;
	pthread_t t, t2;

	/*
	 * Allocate a stack with the default size so that we verify
	 * when libpthread reuses the thread, it doesn't inadvertently
	 * reuse the libpthread-allocated stack too and instead
	 * correctly uses our user-allocated stack.
	 */
	init(getdefaultstacksize());

	/*
	 * Create a thread with a libpthread-allocated stack that
	 * verifies
	 * (a) access to its own stack, and
	 * (b) no access to its own guard pages;
	 * then get its attributes and wait for it to complete.
	 */
	RZ(pthread_create(&t, NULL, &checkguardaccessthread, C));
	RZ(pthread_getattr_np(t, &attr));
	RZ(pthread_join(t, NULL));

	/*
	 * Create a thread with a user-allocated stack that verifies
	 * (a) stack addr/size match request, and
	 * (b) access to the requested stack,
	 * and confirm that the first thread was recycled -- not part
	 * of POSIX semantics, but part of NetBSD's implementation;
	 * this way, we verify that, even though the thread is
	 * recycled, the thread's stack is set to the user-allocated
	 * stack and access to it works as expected.  Then wait for it
	 * to complete.
	 */
	RZ(pthread_attr_init(&attr2));
	RZ(pthread_attr_setstack(&attr2, C->addr, C->size));
	RZ(pthread_create(&t2, &attr2, &checkaddraccessthread, C));
	ATF_CHECK_EQ_MSG(t, t2, "t=%p t2=%p", t, t2); /* NetBSD recycles */
	RZ(pthread_join(t2, NULL));

	/*
	 * Verify that the libpthread-allocated stack and
	 * user-allocated stack had the same size, since we chose the
	 * default size.
	 *
	 * Note: We can't say anything about the guard size, because
	 * with pthread_attr_setstack, the guard size is ignored, and
	 * it's not clear from POSIX whether any meaningful guard size
	 * is stored for retrieval with pthread_attr_getguardsize in
	 * attributes with pthread_attr_setstack.
	 */
	RZ(pthread_attr_getstacksize(&attr, &size));
	RZ(pthread_attr_getstacksize(&attr2, &size2));
	ATF_CHECK_EQ_MSG(size, size2, "size=%zu size2=%zu", size, size2);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, stack1);
	ATF_TP_ADD_TC(tp, stack2);

	return atf_no_error();
}
