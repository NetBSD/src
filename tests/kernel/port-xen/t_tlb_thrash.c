/* $NetBSD: t_tlb_thrash.c,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cherry G. Mathew <cherry@NetBSD.org>
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

__RCSID("$NetBSD: t_tlb_thrash.c,v 1.1.2.1 2011/06/03 13:27:43 cherry Exp $");

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#include <sys/bitops.h>

#include <atf-c.h>

#include "arrayalloc.h"
#include "barriers.h"
#include "thread.h"
#include "system.h"

/* 
 * The goal of these tests is to stress the kernel pmap
 * implementation, from userspace. 
 */

/* 
 * Thread thrash: This test fires off one thread per CPU. 
 * Each thread makes synchronised, interleaved data accesses to a
 * shared, locked page of memory. Each thread has a unique mapping to
 * this page, obtained via mmap(). The mappings to the shared page are
 * torn down and created afresh every time, in order to exercise the
 * pmap routines. (XXX: break down this test into smaller tests that
 * exercise identifiable areas of pmap.)
 *
 * This test only operates on a single pmap.
 */


/* Pattern helpers. */
static void
pattern_write(vaddr_t addr, size_t len)
{
	vaddr_t p;
	for (p = addr; p < (addr + len); p++) {
		memset((void *)p, p % 255, 1);
	}
}

static bool
pattern_verify(vaddr_t addr, size_t len)
{
	vaddr_t p;
	unsigned int c;
	for (p = addr; p < (addr + len); p++) {
		c = 0;
		memcpy(&c, (void *)p, 1);
		if (c != (p % 255)) {
			return false;
		}
	}

	return true;
}

struct mem_verify {
	vaddr_t addr;
	vaddr_t len;

	struct barrier *ready_bar; /* tests are all ready */
	struct barrier *go_bar; /* Fire! */
	struct barrier *done_bar;

	bool result[MAXCPUS];
};


/*
 * Kickoff spawns off multiple threads, one with "master" status, and
 * all the other with "slave" status. All threads fireup, and
 * synchronise at three barriers, namely, "ready", "go", and "done".
 * Each thread runs on a unique CPU, and is bound to it for the life
 * of the test.
 */

static bool
test_kickoff(struct mem_verify *mvp, threadcb_t setup, threadcb_t master, threadcb_t slave, threadcb_t cleanup, enum threadtype type)
{
	cpuid_t ncpus, i, j;
	struct thread_ctx *tw[MAXCPUS], *tr[MAXCPUS];
	bool result = true;

	assert(mvp != NULL);
	assert(master != NULL);
	assert(slave != NULL);

	/* 1) Detect no. of cpus via cpuset(3) */
	ncpus = getcpus();

	printf("Detected %lu cpus\n", ncpus);
//	ATF_REQUIRE(ncpus > 0);

	/* 2) Prepare and fire off threads */
	thread_init();

	printf("new threads\n");

	for (i = 0; i < ncpus; i++) {
		printf("iteration %lu\n", i);
		/* reset payloads */
		mvp->result[i] = false;

		/* reset barriers */
		mvp->ready_bar = vm_barrier_create(ncpus);
		if (mvp->ready_bar == NULL) goto ready_bar_bad;
		mvp->go_bar = vm_barrier_create(ncpus);
		if (mvp->go_bar == NULL) goto go_bar_bad;
		mvp->done_bar = vm_barrier_create(ncpus);
		if (mvp->go_bar == NULL) goto done_bar_bad;
		goto barriers_good;

	done_bar_bad:
		vm_barrier_destroy(mvp->go_bar);
	go_bar_bad:
		vm_barrier_destroy(mvp->ready_bar);
	ready_bar_bad:
		abort();

	barriers_good:

		if (setup) {
			setup(mvp);
		}

		tw[i] = thread_spawn(i, type, master, mvp, cleanup);
		if (tw[i] == NULL) {
			printf("writer thread spawn failed for cpu%lu\n", i);
			abort();
		}

		//ATF_REQUIRE(tw[i] != NULL);


		for (j = 0; j < ncpus; j++) {
			if (j == i) {
				continue;
			}
			tr[j] = thread_spawn(j, type, slave, mvp, cleanup);
			if (tr[j] == NULL) {
				printf("writer thread spawn failed for cpu%lu\n", j);
				abort();
			}
		}

		/* Wait for threads */
		printf("waiting for threads\n");
		printf("waiting for writer %lu\n", i);

		thread_wait(tw[i]);

		/* Wait for threads to join */
		for (j = 0; j < ncpus; j++) {
			if (j == i) {
				continue;
			}

			printf("waiting for reader %lu\n", j);

			thread_wait(tr[j]);
			if (mvp->result[j] == false) {
				printf("Thread #%lu failed\n", j);
				result = false;
			}
			else {
				printf("Thread #%lu succeeded!\n", j);
			}
		}

		if (mvp->result[i] == false) {
			printf("Thread #%lu failed\n", i);
			result = false;
		}
		else {
			printf("Thread #%lu succeeded!\n", i);
		}

		if (cleanup) {
			cleanup(mvp);
		}

		/* Destroy barriers */
		vm_barrier_destroy(mvp->done_bar);
		vm_barrier_destroy(mvp->go_bar);
		vm_barrier_destroy(mvp->ready_bar);
	}


	thread_fini();

	return result;
}

/* 
 * Test #1: Write some data and verify it from separate threads on
 * their own cpus.
 */
static void
write_to_page(void *arg)
{
	assert(arg != NULL);

	struct mem_verify *mvp = arg;
	cpuid_t cpuid = thread_cid(thread_self());

	vm_barrier_hold(mvp->ready_bar);
	assert(mvp->addr != 0 && mvp->len == PAGE_SIZE);

	mvp->result[cpuid] = true;
	pattern_write(mvp->addr, PAGE_SIZE);

	vm_barrier_hold(mvp->go_bar); /* Tell the readers to go */

	vm_barrier_hold(mvp->done_bar);
}

static void
verify_page(void *arg)
{
	assert(arg != NULL);

	struct mem_verify *mvp = arg;

	cpuid_t cpuid = thread_cid(thread_self());

	vm_barrier_hold(mvp->ready_bar);
	assert(mvp->addr != 0 && mvp->len == PAGE_SIZE);

	vm_barrier_hold(mvp->go_bar);

	mvp->result[cpuid] = pattern_verify(mvp->addr, mvp->len);
	vm_barrier_hold(mvp->done_bar);
}

/*
 * The setup function is only called once per test, and from the
 * control thread in test_kickoff().
 * Its purpose is to setup test specific context (eg: memory shared
 * between threads)
 */

static void
readwrite_setup(void *arg)
{

	struct mem_verify *mvp = arg;

	assert(mvp != NULL);

	mvp->addr = (vaddr_t) arrayalloc(PAGE_SIZE);

	if (mvp->addr == 0) {
		abort();
	}
	mvp->len = PAGE_SIZE;
}

/* 
 * The cleanup function gets called per thread, via thread_abort(),
 * or after the test is complete, for test specific cleanup. It is
 * guaranteed to be called at least once, per test, if provided via
 * test_kickoff() 
 * Unlike master/slave functions, cleanup may be called from a non
 * thread_spawn()ed "thread" (ie; from the main loop for all practical
 * purposes. Watchout for this when using thread_cid() for eg:
 */

static void
readwrite_cleanup(void *arg)
{
	struct mem_verify *mvp = arg;

	assert(mvp != NULL);

	struct thread_ctx *t = thread_self();

	/* Only need to cleanup once */
	if (mvp->addr == 0 || mvp->len != PAGE_SIZE) {
		fprintf(stderr, "skipping prior cleanup.\n");
		return;
	}

	/* Free up test specific resources */
	munmap((void *)mvp->addr, mvp->len);

	mvp->addr = mvp->len = 0;

	if (t != NULL) { /* we're on a master/slave thread */
		mvp->result[thread_cid(t)] = false;
	}

	/* reset barriers */
	vm_barrier_reset(mvp->done_bar);
	vm_barrier_reset(mvp->go_bar);
	vm_barrier_reset(mvp->ready_bar);

}

static bool
test_threadwriteread(void)
{
	struct mem_verify mv;
	memset(&mv, 0, sizeof mv);

	printf("Test #1: ---------------------------------------\n");
	if (!test_kickoff(&mv, readwrite_setup, write_to_page, verify_page, readwrite_cleanup, TYPE_PTHREAD)) {
		printf("Test #1 failed\n");
		return false;
	}
	printf("Test #1 ends: -----------------------------------\n");

	return true;
}

static bool
test_forkwriteread(void)
{
	struct mem_verify *mvp;

	mvp = arrayalloc(sizeof *mvp);
	if (mvp == NULL) {
		fprintf(stderr, "arrayalloc()\n");
		abort();
	}
	memset(mvp, 0, sizeof *mvp);

	printf("Test #1.5: ---------------------------------------\n");
	if (!test_kickoff(mvp, readwrite_setup, write_to_page, verify_page, readwrite_cleanup, TYPE_FORK)) {
		printf("Test #1.5 failed\n");
		return false;
	}
	printf("Test #1.5 ends: -----------------------------------\n");

	return true;
}

/* TLB shootdown. remove mapping from a different CPU */
static void
tlb_master(void *arg)
{
	assert(arg != NULL);

	struct mem_verify *mvp = arg;
	cpuid_t cpuid = thread_cid(thread_self());
	printf("%s: cpuid == %lu\n", __func__, cpuid);
	write_to_page(mvp);
	if (mvp->result[cpuid] == false) {
		return;
	}

	assert(mvp->addr != 0 && mvp->len != 0);
	vm_barrier_hold(mvp->ready_bar);

	pattern_write(mvp->addr, PAGE_SIZE);
	/* unmap the user mapping. */
	munmap((void *)mvp->addr, mvp->len);

	vm_barrier_hold(mvp->go_bar); /* Tell the readers to go */

	vm_barrier_hold(mvp->done_bar);

	mvp->result[cpuid] = true;
}

static void
tlb_slave(void *arg)
{
	assert(arg != NULL);

	struct mem_verify *mvp = arg;
	cpuid_t cpuid = thread_cid(thread_self());
	printf("%s: cpuid == %lu\n", __func__, cpuid);
	mvp->result[cpuid] = false;
	verify_page(mvp);
	if (mvp->result[cpuid] == false) {
		return;
	}

	mvp->result[cpuid] = false;

	vm_barrier_hold(mvp->ready_bar);
	assert(mvp->addr != 0 && mvp->len == PAGE_SIZE);

	prep_fault(true); /* Intercept faults from this point */

	vm_barrier_hold(mvp->go_bar);

	/*
	 * A correctly updated TLB will fault when pattern_verify()
	 * attempts to access memory.  The correct exit path for this
	 * thread is via the fault handler :-)
	 */

	/* 
	 * A stale TLB should be able to verify properly. 
	 * This shouldn't happen. Therefore, invert the result 
	 */

	mvp->result[cpuid] = !pattern_verify(mvp->addr, mvp->len);

	vm_barrier_hold(mvp->done_bar);

	/* 
	 * reset SIGSEGV handler - shouldn't get here if the test
	 * succeeds 
	 */
	prep_fault(false); 
}

/* 
 * The cleanup function gets called per thread, via thread_abort(),
 * Or after the test is complete, for test specific cleanup. It is
 * guaranteed to be called at least once, per test, if provided via
 * test_kickoff() 
 * Unlike master/slave functions, cleanup may be called from a non
 * thread_spawn()ed "thread" (ie; from the main loop for all practical
 * purposes. Watchout for this when using thread_cid() for eg:
 */

static void
tlbreadwrite_cleanup(void *arg)
{
	struct mem_verify *mvp = arg;

	assert(mvp != NULL);
	struct thread_ctx *t = thread_self();

	prep_fault(false); /* reset SIGSEGV handler */

	/* Only need to cleanup once */
	if (mvp->addr == 0 || mvp->len != PAGE_SIZE) {
		printf("skipping prior cleanup.\n");
		return;
	}

	/* Free up test specific resources. Note that we do not unmap
	 * the block of memory we've mapped. We need to leave that 
	 * to tlb_master() as part of the test.
	 */

	mvp->addr = mvp->len = 0;

	if (t != NULL) { /* We're in a master/slave thread */
		
		printf("thread_cid(t) == %lu\n", thread_cid(t));
		mvp->result[thread_cid(t)] = true; /* XXX: need a few
						    * more checks: 
						    * eg: are we on
						    * the right thread ?
						    */
	}

	/* reset barriers */
	vm_barrier_reset(mvp->done_bar);
	vm_barrier_reset(mvp->go_bar);
	vm_barrier_reset(mvp->ready_bar);
}

/* Test #2 */
/* 
 * This test repeats test #1 (see Test#1 for details). This warms up
 * the TLB on all cpus.  Followed by this, the master thread removes
 * the underlying mapping of the shared page in question, and signals
 * the slaves to access them. If the page is accessible without error,
 * this means that the TLB entry on the slave CPU is stale.
 */

static bool
test_tlbthreadstale(void)
{
	bool result;
	struct mem_verify mv;
	memset(&mv, 0, sizeof mv);

	printf("Test #2: ---------------------------------------\n");
	result = test_kickoff(&mv, readwrite_setup, tlb_master, tlb_slave, tlbreadwrite_cleanup, TYPE_PTHREAD);

	printf("Test #2 ends: -----------------------------------\n");

	return result;
}

static bool
test_tlbforkstale(void)
{
	bool result;
	struct mem_verify *mvp;

	mvp = arrayalloc(sizeof *mvp);
	if (mvp == NULL) {
		fprintf(stderr, "arrayalloc()\n");
		abort();
	}
	memset(mvp, 0, sizeof *mvp);

	printf("Test #3: ---------------------------------------\n");
	result = test_kickoff(mvp, readwrite_setup, tlb_master, tlb_slave, tlbreadwrite_cleanup, TYPE_FORK);

	printf("Test #3 ends: -----------------------------------\n");

	return result;
}


ATF_TC(threadwriteread);
ATF_TC_HEAD(threadwriteread, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "Simple write to shared memory and read back and verify");
}

ATF_TC_BODY(threadwriteread, tc)
{
	/* Init the mmap wrapper stuff */
	arrayalloc_init(NULL);

	if (!test_threadwriteread()) {
		fprintf(stderr, "test_writeread() failed\n");
		abort(); /* XXX REQUIRE_... */
	}

	arrayalloc_fini();
}

ATF_TC(forkwriteread);
ATF_TC_HEAD(forkwriteread, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "Simple write to shared memory and read back and verify");
}

ATF_TC_BODY(forkwriteread, tc)
{
	/* Init the mmap wrapper stuff */
	arrayalloc_init(NULL);

	if (!test_forkwriteread()) {
		fprintf(stderr, "test_writeread() failed\n");
		abort(); /* XXX REQUIRE_... */
	}

	arrayalloc_fini();
}

ATF_TC(tlbthreadstale);
ATF_TC_HEAD(tlbthreadstale, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "access a page invalidated from another cpu in the same pmap.");
}

ATF_TC_BODY(tlbthreadstale, tc)
{
	/* Init the mmap wrapper stuff */
	arrayalloc_init(NULL);

	if (!test_tlbthreadstale()) {
		fprintf(stderr, "test_tlbstale() failed\n");
		abort(); /* XXX REQUIRE_... */
	}

	arrayalloc_fini();
}

ATF_TC(tlbforkstale);
ATF_TC_HEAD(tlbforkstale, tc)
{
	atf_tc_set_md_var(tc, "descr",
			  "access a page invalidated from another cpu.");
}

ATF_TC_BODY(tlbforkstale, tc)
{
	/* Init the mmap wrapper stuff */
	arrayalloc_init(NULL);

	if (!test_tlbforkstale()) {
		fprintf(stderr, "test_tlbstale() failed\n");
		abort(); /* XXX REQUIRE_... */
	}

	arrayalloc_fini();
}


ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, threadwriteread);
	ATF_TP_ADD_TC(tp, forkwriteread);
	ATF_TP_ADD_TC(tp, tlbthreadstale);
	ATF_TP_ADD_TC(tp, tlbforkstale);
	return atf_no_error();
}
