/* $NetBSD: */

/*
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jonathan Stone, currently employed by Decru, Inc.
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
 * Copyright 2003 Jonathan Stone.
 * All rights reserved.
 */

#include <sys/queue.h>

/*
 * kcont -- Continuation-passing for BSD kernels.
 * 
 * This module defines structures that implement a
 * continuation-passing model in C which can be used as a
 * ``callback''-like  alternative to using  a process context to
 * tsleep() on an object address, waiting for some event or status
 * change on that object.  Since ANSI C provides neither
 * lexically-nested functions nor heap allocation of activation
 * records, we implement a continuation as a struct containing:
 * 1. A function pointer, pointing to the continuation.
 * 2. A object pointer: the object being "slept" upon. 
 * 3. An argument pointer: a pointer to private storage containing
 *    other arguments to the continuation function, including any
 *    additional state beyond the "slept-upon" object.
 */

typedef struct kc {
	SIMPLEQ_ENTRY(kc) kc_next;	/* next kc in object's callback list */

	void (*kc_fn) (void * /*obj*/, void * /*env_arg*/, int /* status*/);

	void * kc_env_arg;	/* caller-supplied continuation argument */
	void * kc_obj;		/* saved object, used for deferred calls */
	int kc_status;		/* saved status, used for  deferred calls */
  	ushort kc_ipl;		/* IPL at which to call kc_func */
	ushort kc_flags;	/* Flags, private to kcont implementation */
} kc_t;

/* kc_flags values. */
#define KC_AUTOFREE 0x01	/* This struct kc * was malloc'ed by
				 * the kcont module; free it immediately 
				 * after calling the continuation function.
				 */
#define KC_DEFERRED 0x02	/* Deferral has already saved object, status */

/* kcont IPL values. */
#define KC_IPL_DEFER_PROCESS	0x00	/* Hand off continuation fn to a
					 * full process context (kthread).
					 */

#define KC_IPL_DEFER_SOFTCLOCK	0x01
#define KC_IPL_DEFER_SOFTNET	0x02	/* run continatiuon in an
					 * IPL_SOFTNET software callout. */
#define KC_IPL_DEFER_SOFTSERIAL	0x03

#define KC_IPL_IMMED	0x10	/*
				 * Execute continuation fn immediately,
				 * in the context of the object event,
				 * with no completion. Continuation
				 * assumed to be very quick.
				 */

/*
 * Head of a list of pending continuations.
 * For example, for continuation-based buffer I/O,
 * add a kcq_t to  struct buf, and pass a struct kc *
 * down through VFS and strategy layers.
 */
typedef SIMPLEQ_HEAD(kcqueue, kc)  kcq_t;
		    

/*
 * Prepare a struct kc continuation using caller-supplied memory
 */
struct kc * kcont(struct kc *,
	    void (* /*fn*/) __P((void* /*obj*/, void */*arg*/, int /*sts*/)),
	    void * /*env_arg*/, int /* continue_ipl*/);

/* 
 * Prepare a struct kc continuation using malloc'ed memory.
 * The malloc'ed memory will be automatically freed after the continuation
 * is finally called.
 */
struct kc *kcont_malloc(int /*allocmflags*/,
	    void (* /*fn*/) __P((void * /*obj*/, void */*arg*/, int /*sts*/)),
	    void * /*env_arg*/, int /* continue_ipl*/);


/*
 * Use a caller-supplied, already-constructed kcont to defer execution
 * from the current context to some lower interrupt priority.
 * Caller must supply the preallocated kcont (possibly via kcont_malloc()).
 */
void	kcont_defer(struct kc * /*kc*/, void * /*obj*/,
		    int /*status*/);
/*
 * Use a kcont to defer execution from the current context to some
 * lower interrupt priority. Space is malloc'ed and freed as for
 * kcont_malloc().Deprecated; use kcont_defer(0 and kcont_malloc().
 */
void  kcont_defer_malloc(int, /* mallocflags */
	    void (* /*fn*/) __P((void* /*obj*/, void */*arg*/, int /*sts*/)),
	    void * /*obj*/,
	    void * /*env_arg*/, int /*status*/, int /*continue_ipl*/);

/*
 * Enqueue a struct kc * into the kc_queue* field of some kernel struct.
 * The struct kc * argument is typically passed as an argument to
 * a function which requests an asynchronous operation on that kernel object.
 * When the operation completes, or the  object otherwise decides to
 * invoke a  wakeup() on itself, all continuations on the object's
 * kcqueue will be called, either immediately or if a  continuation 
 * requested it) after deferring to a kc_queue * servied at
 *  software-interrupt priority.
 */
void	kcont_enqueue(kcq_t * /*kcqueue*/, struct kc * /*kc*/);


/*
 * Execute (or defer) all continuations on an object's kcqueue
 * when  an asynchronous operation completes. Runs through the
 * struct kc_queue * of struct kcs, either running the continuations
 * with the given objcet and status; or saving the object and status
 * and deferring the kconts to a software-interrupt priority kc_queue.
 */
void	kcont_run __P((kcq_t * /*kcq*/, void * /*obj*/,
	    int /*status*/, int /*cur_ipl*/));


/* Initialize kcont framework at boot time. */
void	kcont_init __P((void));

