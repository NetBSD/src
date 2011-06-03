/* $NetBSD: thread.c,v 1.1.2.1 2011/06/03 13:27:44 cherry Exp $ */

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

__RCSID("$NetBSD: thread.c,v 1.1.2.1 2011/06/03 13:27:44 cherry Exp $");

#include <assert.h>
#include <pthread.h>

#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/param.h>
#include <sys/wait.h>

#include "barriers.h"
#include "thread.h"


struct thread_arg { /* Payload for setjmp_tramp() */
	void (*func)(void *);
	void (*abortf)(void *);
	void *arg;
};

struct thread_ctx {
	cpuid_t cid; /* The cpu number we are running on */
	enum threadtype type; /* process, pthread, etc. */
	pid_t pid;
	pthread_t pth;
	cpuset_t *cset;
	struct barrier *init_bar;
	struct thread_arg ctx;
};

/* Thread wrappers */
/* Quick note on thread "id". Since we assume one thread per cpu, the
 * cpuid is used in place of the thread "id" for all practical
 * purposes.
 */

static sigjmp_buf sequel[MAXCPUS];

/* Fault handler, to test for legitimate page faults. */
static void
thread_pagefault(int sig)
{
	if (sig != SIGSEGV) return;

	siglongjmp(sequel[thread_cid(thread_self())], 1);
	fprintf(stderr, "pagefault handler did not longjmp() !");

	abort();
}

/*
 * Set the fault handler of the current process to fault_routine()
 * To restore the default handler, use prep_fault(NULL);
 * Returns the fault handler that has been set.
 */

void
prep_fault(bool prep)
{
	if (prep) {
		signal(SIGSEGV, thread_pagefault);
	}
	else {
		signal(SIGSEGV, SIG_DFL); /* reset signal */
	}
}

static void
_t_destroy_exit(struct thread_ctx *t)
{
	assert(t != NULL);

	switch(t->type) {
	case TYPE_PTHREAD:
		free(t);
		pthread_exit(NULL);
		/* NOTREACHED */
		break;
	case TYPE_FORK:
		free(t);
		exit(0);
		/* NOTREACHED */
		break;
	default:
		fprintf(stderr, "%s: invalid thread type\n", __func__);
		abort();
	}
}


/* Can only be called from a managed, own thread */

static void
thread_exit(struct thread_ctx *t)
{
	assert(t != NULL);
	assert(pthread_equal(t->pth, pthread_self()));

	if (t->type == TYPE_FORK) {
		assert(t->pid == getpid());
	}

	vm_barrier_destroy(t->init_bar);
	cpuset_destroy(t->cset);
	_t_destroy_exit(t);
}

inline bool
thread_equal(struct thread_ctx *t1, struct thread_ctx *t2)
{
	return (t1 == t2);
}

/* 
 * Same as thread_exit, but calls abort callback, if registered,
 * before exiting
 */
static void
thread_abort(struct thread_ctx *t)
{
	assert(t != NULL);
	assert(pthread_equal(t->pth, pthread_self()));

	if (t->type == TYPE_FORK) {
		assert(t->pid == getpid());
	}

	if (t->ctx.abortf != NULL) {
		t->ctx.abortf(t->ctx.arg);
	}

	vm_barrier_destroy(t->init_bar);
	cpuset_destroy(t->cset);
	_t_destroy_exit(t);
}

static pthread_key_t tls;

void
thread_init(void)
{
	if (pthread_key_create(&tls, NULL)) {
		perror("pthread_key_create()");
		abort();
	}

	if (pthread_setspecific(tls, NULL)) {
		perror("pthread_setspecific()");
		abort();
	}

}

void
thread_fini(void)
{
	if (pthread_key_delete(tls)) {
		perror("pthread_key_delete()");
		abort();
	}
}

static void *
setjmp_tramp(void *arg)
{
	assert(arg != NULL);

	pthread_t pth;
	struct thread_ctx *t = arg;

	pth = pthread_self();

	vm_barrier_hold(t->init_bar); /* Sync with thread_spawn */

	assert(pthread_equal(pth, t->pth));

	if (pthread_setspecific(tls, t)) {
		perror("pthread_setspecific()");
		thread_fini();
		thread_abort(t);
	}
		
	if (sigsetjmp(sequel[t->cid], 1)) {
		/*
		 * got here via longjmp() from fault
		 * routine.
		 */

		printf("caught exception\n");

		thread_abort(t);
	}

	t->ctx.func(t->ctx.arg);

	thread_exit(t);
	return NULL;
}


struct thread_ctx *
thread_self(void)
{
	struct thread_ctx *t;
	t = pthread_getspecific(tls);
	return t;
}

cpuid_t
thread_cid(struct thread_ctx *t)
{
	assert(t != NULL);
	return t->cid;
}

struct thread_ctx *
thread_spawn(cpuid_t cid,  /* cpu number */
	     enum threadtype threadtype,
	     void (*func)(void *),
	     void *arg,
	     void (*abortf)(void *))
{
	struct thread_ctx *t;
	cpuset_t *cpuset;

	assert(func != NULL);
	assert(cid <= MAXCPUS);

	t = (struct thread_ctx *) malloc(sizeof *t);
	if (t == NULL) {
		return NULL;
	}

	cpuset = cpuset_create();

	if (cpuset == NULL) {
		printf("Could not create cpuset\n");
		free(t);
		return NULL;
	}

	if (cpuset_set(cid, cpuset) == -1) {
		printf("Could not set cpuset affinity to cpu%lu \n", cid);
		cpuset_destroy(cpuset);
		free(t);
		return NULL;
	}

	t->cset = cpuset;
	t->cid = cid;
	t->ctx.func = func;
	t->ctx.arg = arg;
	if (abortf != NULL) {
		t->ctx.abortf = abortf;
	}

	t->init_bar = vm_barrier_create(2); /* Sync point between us and child */

	if (t->init_bar == NULL) {
		fprintf(stderr, "error initialising barrier\n");
		free(t);
 		return NULL;
	}

	switch (threadtype) {
	case TYPE_PTHREAD:
		t->type = TYPE_PTHREAD;
		if (pthread_create(&t->pth, NULL,
				   setjmp_tramp, t)) {
			fprintf(stderr, "error creating thread \n");
			free(t);
			return NULL;
		}
		break;
	case TYPE_FORK:
		t->type = TYPE_FORK;;
		t->pth = pthread_self();
		t->pid = fork();

		switch (t->pid) {
		case -1:
			perror("fork()");
			abort();
		case 0: /* child */
			t->pid = getpid();
			setjmp_tramp(t);
			/* NOTREACHED */
			fprintf(stderr, "setjmp_tramp() returned!\n");
			abort();
			break;
		default: /* parent */
			break;
		}
		break;
	default:
		fprintf(stderr, "incorrect type specified\n");
		free(t);
 		return NULL;

	}

	vm_barrier_hold(t->init_bar); /* Sync with setjmp_tramp() */

	/* Set affinity */

	if (pthread_setaffinity_np(t->pth, cpuset_size(t->cset), t->cset)) {
		printf("error binding thread to CPU %lu\n", 
		       t->cid);
		perror("pthread_setaffinity_np");
		/* XXX: "destroy" the thread */

		abort();
	}

	return t;
}

/* 
 * This function reaps the context memory, not thread_wait(); 
 * This makes it compulsory to use this function from the controlling
 * thread, to make sure memory is not leaked.
 *
 * This is slightly lame, but we're a testing framework, not a
 * threading library.
 */

void
thread_wait(struct thread_ctx *t)
{

	int error;
	pthread_t pth;

	assert(t != NULL);
	assert(t != thread_self());

	switch (t->type) {
		int status;

	case TYPE_PTHREAD:
		pth = t->pth;
		error = pthread_join(pth, NULL);
		if (error) {
			fprintf(stderr, "pthread_join() %s\n", strerror(error));
			abort();
		}
		break;
	case TYPE_FORK:
		if (waitpid(t->pid, &status, WNOHANG | WALLSIG) == -1) {
			perror("waitpid()");
			abort();
		}
		break;
	default:
		fprintf(stderr, "unknown thread type\n");
		abort();
	}

	/* t is free()d by the thread on thread_exit() */
}
