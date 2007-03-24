/* $NetBSD: kern_kcont.c,v 1.13.26.1 2007/03/24 14:56:01 yamt Exp $ */

/*
 * Copyright 2003 Jonathan Stone.
 * All rights reserved.
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

/*
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_kcont.c,v 1.13.26.1 2007/03/24 14:56:01 yamt Exp $ ");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/queue.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/kthread.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <lib/libkern/libkern.h>

#include <machine/intr.h>	/* IPL_*, and schedsoftnet() */
				/* XXX: schedsofnet() should die. */

#include <sys/kcont.h>

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
/*
 * Software-interrupt priority continuation queues.
 */
static kcq_t kcq_softnet;
static kcq_t kcq_softclock;
static kcq_t kcq_softserial;

static void *kc_si_softnet;
static void *kc_si_softclock;
static void *kc_si_softserial;
#endif /* __HAVE_GENERIC_SOFT_INTERRUPTS */

/*
 * Pool allocator structure.
 */
POOL_INIT(kc_pool, sizeof(struct kc), 0, 0, 0, "kcpl", NULL, IPL_VM);

/*
 * Process-context continuation queue.
 */
static kcq_t kcq_process_ctxt;

/*
 * Insert/Remove a fully-formed struct kc * into the kc_queue *
 * of some kernel object (e.g., a struct buf *).
 * For fine-grained SMP, both enqueueing and dequeueing will
 * need a locking mechanism.
 */
static inline void
kcont_enqueue_atomic(kcq_t *kcq, struct kc *kc)
{
	int s;

	s = splvm();
	SIMPLEQ_INSERT_TAIL(kcq, kc, kc_next);
	splx(s);
}

static inline struct kc *
kcont_dequeue_atomic(kcq_t *kcq)
{
	struct kc *kc;
	int s;

	s = splvm();
	kc = SIMPLEQ_FIRST(kcq);
	if (kc != NULL) {
		SIMPLEQ_REMOVE_HEAD(kcq, kc_next);
		SIMPLEQ_NEXT(kc, kc_next) = NULL;
	}
	splx(s);
	return kc;
}

/*
 * Construct a continuation object from pre-allocated memory.
 * Used by functions that are about call an asynchronous operation,
 * to build a continuation to be called once the operation completes.
 */
static inline struct kc *
kc_set(struct kc *kc, void (*func)(void *, void *, int),
    void *env_arg, int ipl)
{

	kc->kc_fn = func;
	kc->kc_env_arg = env_arg;
	kc->kc_ipl = ipl;
	kc->kc_flags = 0;
#ifdef DEBUG
	kc->kc_obj = NULL;
	kc->kc_status = -1;
	SIMPLEQ_NEXT(kc, kc_next) = NULL;
#endif
	return kc;
}

/*
 * Request a continuation.  Caller provides space for the struct kc *.
 */
struct kc *
kcont(struct kc *kc, void (*func)(void *, void *, int),
    void *env_arg, int continue_ipl)
{

	/* Just save the arguments in the kcont *. */
	return kc_set(kc, func, env_arg, continue_ipl);
}

/*
 * Request a malloc'ed/auto-freed continuation. The kcont framework
 * mallocs the struct kc, and initializes it with the caller-supplied args.
 * Once the asynchronous operation completes and the continuation function
 * has been called, the kcont framework will free the struct kc *
 * immediately after the continuation function returns.
 */
struct kc *
kcont_malloc(int malloc_flags,
    void (*func)(void *, void *, int),
    void *env_arg, int continue_ipl)
{
	struct kc *kc;
	int pool_flags;

	pool_flags = (malloc_flags & M_NOWAIT) ? 0 : PR_WAITOK;
	pool_flags |= (malloc_flags & M_CANFAIL) ? PR_LIMITFAIL : 0;

	kc = pool_get(&kc_pool, pool_flags);
	if (kc == NULL)
		return kc;
	return kc_set(kc, func, env_arg, continue_ipl);
}

/*
 * Dispatch a dequeued continuation which requested deferral
 * into the appropriate lower-priority queue.
 * Public API entry to defer execution of a pre-built kcont.
 */
void
kcont_defer(struct kc *kc, void *obj, int status)
{
	/*
	 * IPL at which to synchronize access to object is
	 * above the continuer's requested callback IPL,
	 * (e.g., continuer wants IPL_SOFTNET but the object
	 * holding this continuation incurred the wakeup()able
	 * event whilst at IPL_BIO).
	 * Defer this kc to a lower-priority kc queue,
	 * to be serviced slightly later at a lower IPL.
	 */

	/*
	 * If we already deferred this kcont, don't clobber
	 * the previously-saved kc_object and kc_status.
	 * (The obj/status arguments passed in by ck_run() should
	 * be the same as kc_object/kc_status, but don't rely on that.)
	 */
	if ((kc->kc_flags & KC_DEFERRED) == 0) {
		kc->kc_flags |= KC_DEFERRED;
		kc->kc_obj = obj;
		kc->kc_status = status;
	}

	switch (kc->kc_ipl) {
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	case KC_IPL_DEFER_SOFTCLOCK:
		kcont_enqueue_atomic(&kcq_softclock, kc);
		softintr_schedule(kc_si_softclock);
		break;
	case KC_IPL_DEFER_SOFTNET:
		kcont_enqueue_atomic(&kcq_softnet, kc);
		softintr_schedule(kc_si_softnet);
		break;
	case KC_IPL_DEFER_SOFTSERIAL:
		kcont_enqueue_atomic(&kcq_softserial, kc);
		softintr_schedule(kc_si_softserial);
		break;

#else /* !__HAVE_GENERIC_SOFT_INTERRUPTS */
	/* What to do? For now, punt to process context */
	case KC_IPL_DEFER_SOFTCLOCK:
	case KC_IPL_DEFER_SOFTSERIAL:
	case KC_IPL_DEFER_SOFTNET:
		/*FALLTHROUGH*/
#endif /* __HAVE_GENERIC_SOFT_INTERRUPTS */

	case KC_IPL_DEFER_PROCESS:
		kcont_enqueue_atomic(&kcq_process_ctxt, kc);
		wakeup(&kcq_process_ctxt);
		break;
	default:
		KASSERT(0);
	}
}

void
kcont_defer_malloc(int mallocflags,
    void (*func)(void *, void *, int),
    void *obj, void *env_arg, int status, int ipl)
{
	struct kc *kc;

	kc = kcont_malloc(mallocflags, func, env_arg, ipl);
	if (kc != NULL)
		kcont_defer(kc, obj, status);
}

/*
 * Enqueue a pre-existing kcont onto a struct kcq completion queue
 * of some pre-existing kernel object.
 */
void
kcont_enqueue(kcq_t *kcq, struct kc *kc)
{

	kcont_enqueue_atomic(kcq, kc);
}


/*
 * Run through a list of continuations, calling (or handing off)
 * continuation functions.
 * If the caller-provided IPL is the same as the requested IPL,
 * deliver the callback.
 * If the caller-provided IPL is higher than the requested
 * callback IPL, re-enqueue the continuation to a lower-priority queue.
 */
void
kcont_run(kcq_t *kcq, void *obj, int status, int curipl)
{
	struct kc *kc;

	while ((kc = kcont_dequeue_atomic(kcq)) != NULL) {

		/* If execution of kc was already deferred, restore context. */
		if (kc->kc_flags & KC_DEFERRED) {
			KASSERT(obj == NULL);
			obj = kc->kc_obj;
			status = kc->kc_status;
		}

		/* Check whether to execute now or to defer. */
		if (kc->kc_ipl == KC_IPL_IMMED || curipl <= kc->kc_ipl) {
			int saved_flags = kc->kc_flags; /* XXX see below */

			/* Satisfy our raison d'e^tre */
			(*kc->kc_fn)(obj, kc->kc_env_arg, status);

			/*
			 * We must not touch (*kc) after calling
			 * (*kc->kc_fn), unless we were specifically
			 * asked to free it.  The memory for (*kc) may
			 * be a sub-field of some other object (for example,
			 * of kc->kc_env_arg) and (*kc_fn)() may already
			 * have freed it by the time we get here.  So save
			 * kc->kc_flags (above) and use that saved copy
			 * to test for auto-free.
			 */
			if (saved_flags & KC_AUTOFREE)
				pool_put(&kc_pool, kc);
		} else {
			kcont_defer(kc, obj, status);
		}
	}
}

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
/*
 * Trampolines for processing software-interrupt kcont queues.
 */
static void
kcont_run_softclock(void *arg)
{

	kcont_run((struct kcqueue *)arg, NULL, 0, KC_IPL_DEFER_SOFTCLOCK);
}

static void
kcont_run_softnet(void *arg)
{

	kcont_run((struct kcqueue *)arg, NULL, 0, KC_IPL_DEFER_SOFTNET);
}

static void
kcont_run_softserial(void *arg)
{

	kcont_run((struct kcqueue *)arg, NULL, 0, KC_IPL_DEFER_SOFTSERIAL);
}
#endif /* __HAVE_GENERIC_SOFT_INTERRUPTS */

/*
 * Main entrypoint for kcont worker kthreads to execute
 * a continuation which requested deferral to process context.
 */
static void
kcont_worker(void *arg)
{
	int status;

	(void)arg;	/* kill GCC warning */

	while (1) {
		status = ltsleep(&kcq_process_ctxt, PCATCH, "kcont", hz, NULL);
		if (status != 0 && status != EWOULDBLOCK)
			break;
		kcont_run(&kcq_process_ctxt, NULL, 0, KC_IPL_DEFER_PROCESS);
	}
	kthread_exit(0);
}

static void
kcont_create_worker(void *arg)
{
	if (kthread_create1(kcont_worker, NULL, NULL, "kcont"))
		panic("fork kcont");
}

/*
 * Initialize kcont subsystem.
 */
void
kcont_init(void)
{

#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
	/*
	 * Initialize kc_queue and callout for soft-int deferred
	 * continuations. (If not available, deferrals fall back
	 * to deferring all the way to process context).
	 */
	SIMPLEQ_INIT(&kcq_softclock);
	kc_si_softclock = softintr_establish(IPL_SOFTCLOCK,
	    kcont_run_softclock, &kcq_softnet);

	SIMPLEQ_INIT(&kcq_softnet);
	kc_si_softnet = softintr_establish(IPL_SOFTNET,
	    kcont_run_softnet, &kcq_softnet);

	SIMPLEQ_INIT(&kcq_softserial);
	kc_si_softserial = softintr_establish(IPL_SOFTSERIAL,
	    kcont_run_softserial, &kcq_softserial);
#endif	/* __HAVE_GENERIC_SOFT_INTERRUPTS */

	/*
	 * Create kc_queue for process-context continuations, and
	 * a worker kthread to process the queue. (Fine-grained SMP
	 * locking should have at least one worker kthread per CPU).
	 */
	SIMPLEQ_INIT(&kcq_process_ctxt);
	kthread_create(kcont_create_worker, NULL);
}
