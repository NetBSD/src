/*	$NetBSD: allocfree.c,v 1.1 2008/12/20 22:29:05 ad Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: allocfree.c,v 1.1 2008/12/20 22:29:05 ad Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/kthread.h>
#include <sys/condvar.h>
#include <sys/cpu.h>
#include <sys/atomic.h>

#include <machine/cpu_counter.h>

MODULE(MODULE_CLASS_MISC, allocfree, NULL);

static size_t		sz = 128;
static int		nthreads;
static int		count = 100000;
static uint64_t		total;
static kmutex_t		lock;
static kcondvar_t	cv;
static int		nrun;
static void		(*method)(void);
static int		barrier;
static volatile u_int	barrier2;
static int		timing;
static struct pool	pool;
static pool_cache_t	cache;

static void
handle_props(prop_dictionary_t props)
{
	prop_number_t num;

	num = prop_dictionary_get(props, "size");
	if (num != NULL && prop_object_type(num) == PROP_TYPE_NUMBER) {
		sz = (size_t)prop_number_integer_value(num);
		sz = max(sz, 1);
		sz = min(sz, 1024*1024);
	}
	num = prop_dictionary_get(props, "count");
	if (num != NULL && prop_object_type(num) == PROP_TYPE_NUMBER) {
		count = (int)prop_number_integer_value(num);
		count = min(count, 1);
	}
	num = prop_dictionary_get(props, "timing");
	if (num != NULL && prop_object_type(num) == PROP_TYPE_NUMBER) {
		timing = (int)prop_number_integer_value(num);
	}
}

static void
kmem_method(void)
{
	int *p;

	p = kmem_alloc(sz, KM_SLEEP); 
	if (p != NULL) {
		*p = 1;
		kmem_free(p, sz);
	}
}

static void
malloc_method(void)
{
	int *p;

	p = malloc(sz, M_DEVBUF, M_WAITOK);
	if (p != NULL) {
		*p = 1;
		free(p, M_DEVBUF);
	}
}

static void
pool_method(void)
{
	int *p;

	p = pool_get(&pool, PR_WAITOK);
	if (p != NULL) {
		*p = 1;
		pool_put(&pool, p);
	}
}

static void
cache_method(void)
{
	int *p;

	p = pool_cache_get(cache, PR_WAITOK);
	if (p != NULL) {
		*p = 1;
		pool_cache_put(cache, p);
	}
}

static void
test_thread(void *cookie)
{
	struct timespec s, e, t;
	int lcv;
	uint64_t x;

	kpreempt_disable();

	memset(&t, 0, sizeof(t));
	x = 0;

	mutex_enter(&lock);
	barrier++;
	while (barrier < nthreads) {
		cv_wait(&cv, &lock);
	}
	cv_broadcast(&cv);
	mutex_exit(&lock);

	atomic_inc_uint(&barrier2);
	while (barrier2 < nthreads) {
		nullop(NULL);
	}

	if (timing) {
		for (lcv = count; lcv != 0; lcv--) {
			x -= cpu_counter();
			(*method)();
			x += cpu_counter();
		}	
	} else {
		for (lcv = count; lcv != 0; lcv--) {
			nanotime(&s);
			(*method)();
			nanotime(&e);
			timespecsub(&e, &s, &e);
			timespecadd(&e, &t, &t);
		}
	}

	mutex_enter(&lock);
	barrier = 0;
	barrier2 = 0;
	if (timing) {
		total += x * 1000000000LL / cpu_frequency(curcpu());
	} else {
		total += timespec2ns(&t);
	}
	if (--nrun == 0) {
		cv_broadcast(&cv);
	}
	mutex_exit(&lock);

	kpreempt_enable();
	kthread_exit(0);
}

static void
run2(int nt, void (*func)(void))
{
	struct cpu_info *ci;
	CPU_INFO_ITERATOR cii;
	int error;

	nthreads = nt;
	total = 0;
	method = func;
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (nt-- == 0) {
			break;
		}
		error = kthread_create(PRI_NONE, KTHREAD_MPSAFE,
		    ci, test_thread, NULL, NULL, "test");
		if (error == 0) {
			nrun++;
		} else {
			nthreads--;
		}
	}
	mutex_enter(&lock);
	cv_broadcast(&cv);
	while (nrun > 0) {
		cv_wait(&cv, &lock);
	}
	mutex_exit(&lock);
	if (nthreads == 0) {
		printf("FAILED\n");
	} else {
		printf("\t%d", (int)(total / nthreads / count));
	}
}

static void
run1(int nt)
{

	run2(nt, malloc_method);
	run2(nt, kmem_method);
	run2(nt, pool_method);
	run2(nt, cache_method);
	printf("\n");

}

static void
run0(void)
{
	int i;

	for (i = 1; i <= ncpu; i++) {
		printf("%d\t%d", sz, i);
		run1(i);
	}
}

static int
allocfree_modcmd(modcmd_t cmd, void *arg)
{
	const char *timer;

	switch (cmd) {
	case MODULE_CMD_INIT:
		handle_props(arg);
		timer = (timing ? "cpu_counter" : "nanotime");
		printf("=> using %s() for timings\n", timer);
		printf("SIZE\tNCPU\tMALLOC\tKMEM\tPOOL\tCACHE\n");
		mutex_init(&lock, MUTEX_DEFAULT, IPL_NONE);
		cv_init(&cv, "testcv");
		pool_init(&pool, sz, 0, 0, 0, "tpool",
		    &pool_allocator_nointr, IPL_NONE);
		cache = pool_cache_init(sz, 0, 0, 0, "tcache",
		    NULL, IPL_NONE, NULL, NULL, NULL);
		run0();
		pool_destroy(&pool);
		pool_cache_destroy(cache);
		mutex_destroy(&lock);
		cv_destroy(&cv);
		return 0;

	case MODULE_CMD_FINI:
		/* XXX in theory, threads could still be running. */
		return 0;

	default:
		return ENOTTY;
	}
}
