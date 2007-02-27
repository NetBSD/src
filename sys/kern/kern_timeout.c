/*	$NetBSD: kern_timeout.c,v 1.20.2.1 2007/02/27 16:54:26 yamt Exp $	*/

/*-
 * Copyright (c) 2003, 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 2001 Thomas Nordin <nordin@openbsd.org>
 * Copyright (c) 2000-2001 Artur Grabowski <art@openbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL  DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_timeout.c,v 1.20.2.1 2007/02/27 16:54:26 yamt Exp $");

/*
 * Adapted from OpenBSD: kern_timeout.c,v 1.15 2002/12/08 04:21:07 art Exp,
 * modified to match NetBSD's pre-existing callout API.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/callout.h>
#include <sys/mutex.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#endif

/*
 * Timeouts are kept in a hierarchical timing wheel. The c_time is the value
 * of the global variable "hardclock_ticks" when the timeout should be called.
 * There are four levels with 256 buckets each. See 'Scheme 7' in
 * "Hashed and Hierarchical Timing Wheels: Efficient Data Structures for
 * Implementing a Timer Facility" by George Varghese and Tony Lauck.
 */
#define BUCKETS 1024
#define WHEELSIZE 256
#define WHEELMASK 255
#define WHEELBITS 8

static struct callout_circq timeout_wheel[BUCKETS];	/* Queues of timeouts */
static struct callout_circq timeout_todo;		/* Worklist */

#define MASKWHEEL(wheel, time) (((time) >> ((wheel)*WHEELBITS)) & WHEELMASK)

#define BUCKET(rel, abs)						\
    (((rel) <= (1 << (2*WHEELBITS)))					\
    	? ((rel) <= (1 << WHEELBITS))					\
            ? &timeout_wheel[MASKWHEEL(0, (abs))]			\
            : &timeout_wheel[MASKWHEEL(1, (abs)) + WHEELSIZE]		\
        : ((rel) <= (1 << (3*WHEELBITS)))				\
            ? &timeout_wheel[MASKWHEEL(2, (abs)) + 2*WHEELSIZE]		\
            : &timeout_wheel[MASKWHEEL(3, (abs)) + 3*WHEELSIZE])

#define MOVEBUCKET(wheel, time)						\
    CIRCQ_APPEND(&timeout_todo,						\
        &timeout_wheel[MASKWHEEL((wheel), (time)) + (wheel)*WHEELSIZE])

/*
 * All wheels are locked with the same lock (which must also block out all
 * interrupts).
 */
kmutex_t callout_mutex;

/*
 * Circular queue definitions.
 */

#define CIRCQ_INIT(list)						\
do {									\
        (list)->cq_next_l = (list);					\
        (list)->cq_prev_l = (list);					\
} while (/*CONSTCOND*/0)

#define CIRCQ_INSERT(elem, list)					\
do {									\
        (elem)->cq_prev_e = (list)->cq_prev_e;				\
        (elem)->cq_next_l = (list);					\
        (list)->cq_prev_l->cq_next_l = (elem);				\
        (list)->cq_prev_l = (elem);					\
} while (/*CONSTCOND*/0)

#define CIRCQ_APPEND(fst, snd)						\
do {									\
        if (!CIRCQ_EMPTY(snd)) {					\
                (fst)->cq_prev_l->cq_next_l = (snd)->cq_next_l;		\
                (snd)->cq_next_l->cq_prev_l = (fst)->cq_prev_l;		\
                (snd)->cq_prev_l->cq_next_l = (fst);			\
                (fst)->cq_prev_l = (snd)->cq_prev_l;			\
                CIRCQ_INIT(snd);					\
        }								\
} while (/*CONSTCOND*/0)

#define CIRCQ_REMOVE(elem)						\
do {									\
        (elem)->cq_next_l->cq_prev_e = (elem)->cq_prev_e;		\
        (elem)->cq_prev_l->cq_next_e = (elem)->cq_next_e;		\
} while (/*CONSTCOND*/0)

#define CIRCQ_FIRST(list)	((list)->cq_next_e)
#define CIRCQ_NEXT(elem)	((elem)->cq_next_e)
#define CIRCQ_LAST(elem,list)	((elem)->cq_next_l == (list))
#define CIRCQ_EMPTY(list)	((list)->cq_next_l == (list))

/*
 * Some of the "math" in here is a bit tricky.
 *
 * We have to beware of wrapping ints.
 * We use the fact that any element added to the queue must be added with a
 * positive time. That means that any element `to' on the queue cannot be
 * scheduled to timeout further in time than INT_MAX, but c->c_time can
 * be positive or negative so comparing it with anything is dangerous.
 * The only way we can use the c->c_time value in any predictable way
 * is when we calculate how far in the future `to' will timeout -
 * "c->c_time - hardclock_ticks". The result will always be positive for
 * future timeouts and 0 or negative for due timeouts.
 */

#ifdef CALLOUT_EVENT_COUNTERS
static struct evcnt callout_ev_late;
#endif

/*
 * callout_barrier:
 *
 *	If the callout is running on another CPU, busy wait until it
 *	completes.
 */
static inline void
callout_barrier(struct callout *c)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci, *ci_cur;

	LOCK_ASSERT(mutex_owned(&callout_mutex));

	/*
	 * The callout may have already been dispatched to run on the
	 * current CPU.  It's possible for us to arrive here before it
	 * actually runs because the SPL is dropped from IPL_SCHED in
	 * softclock(), and IPL_SOFTCLOCK is low priority. We can't deal
	 * with that race easily, so for now the caller must deal with
	 * it.
	 */
#if 1
	ci_cur = curcpu();	/* XXXgcc get around alpha problem */
	while ((ci = c->c_oncpu) != NULL && ci != ci_cur &&
	    ci->ci_data.cpu_callout == c) {
#else
	while ((ci = c->c_oncpu) != NULL && ci != curcpu() &&
	    ci->ci_data.cpu_callout == c) {
#endif
		mutex_spin_exit(&callout_mutex);
		while (ci->ci_data.cpu_callout == c)
			;
		mutex_spin_enter(&callout_mutex);
	}
	c->c_oncpu = NULL;
#endif
}

/*
 * callout_startup:
 *
 *	Initialize the callout facility, called at system startup time.
 */
void
callout_startup(void)
{
	int b;

	CIRCQ_INIT(&timeout_todo);
	for (b = 0; b < BUCKETS; b++)
		CIRCQ_INIT(&timeout_wheel[b]);
	mutex_init(&callout_mutex, MUTEX_SPIN, IPL_SCHED);

#ifdef CALLOUT_EVENT_COUNTERS
	evcnt_attach_dynamic(&callout_ev_late, EVCNT_TYPE_MISC,
	    NULL, "callout", "late");
#endif
}

/*
 * callout_init:
 *
 *	Initialize a callout structure.
 */
void
callout_init(struct callout *c)
{

	memset(c, 0, sizeof(*c));
}

/*
 * callout_reset:
 *
 *	Reset a callout structure with a new function and argument, and
 *	schedule it to run.
 */
void
callout_reset(struct callout *c, int to_ticks, void (*func)(void *), void *arg)
{
	int old_time;

	KASSERT(to_ticks >= 0);

	mutex_spin_enter(&callout_mutex);

	callout_barrier(c);

	/* Initialize the time here, it won't change. */
	old_time = c->c_time;
	c->c_time = to_ticks + hardclock_ticks;
	c->c_flags &= ~(CALLOUT_FIRED|CALLOUT_INVOKING);

	c->c_func = func;
	c->c_arg = arg;

	/*
	 * If this timeout is already scheduled and now is moved
	 * earlier, reschedule it now. Otherwise leave it in place
	 * and let it be rescheduled later.
	 */
	if (callout_pending(c)) {
		if (c->c_time - old_time < 0) {
			CIRCQ_REMOVE(&c->c_list);
			CIRCQ_INSERT(&c->c_list, &timeout_todo);
		}
	} else {
		c->c_flags |= CALLOUT_PENDING;
		CIRCQ_INSERT(&c->c_list, &timeout_todo);
	}

	mutex_spin_exit(&callout_mutex);
}

/*
 * callout_schedule:
 *
 *	Schedule a callout to run.  The function and argument must
 *	already be set in the callout structure.
 */
void
callout_schedule(struct callout *c, int to_ticks)
{
	int old_time;

	KASSERT(to_ticks >= 0);

	mutex_spin_enter(&callout_mutex);

	callout_barrier(c);

	/* Initialize the time here, it won't change. */
	old_time = c->c_time;
	c->c_time = to_ticks + hardclock_ticks;
	c->c_flags &= ~(CALLOUT_FIRED|CALLOUT_INVOKING);

	/*
	 * If this timeout is already scheduled and now is moved
	 * earlier, reschedule it now. Otherwise leave it in place
	 * and let it be rescheduled later.
	 */
	if (callout_pending(c)) {
		if (c->c_time - old_time < 0) {
			CIRCQ_REMOVE(&c->c_list);
			CIRCQ_INSERT(&c->c_list, &timeout_todo);
		}
	} else {
		c->c_flags |= CALLOUT_PENDING;
		CIRCQ_INSERT(&c->c_list, &timeout_todo);
	}

	mutex_spin_exit(&callout_mutex);
}

/*
 * callout_stop:
 *
 *	Cancel a pending callout.
 */
void
callout_stop(struct callout *c)
{

	mutex_spin_enter(&callout_mutex);

	callout_barrier(c);

	if (callout_pending(c))
		CIRCQ_REMOVE(&c->c_list);

	c->c_flags &= ~(CALLOUT_PENDING|CALLOUT_FIRED);

	mutex_spin_exit(&callout_mutex);
}

/*
 * This is called from hardclock() once every tick.
 * We return !0 if we need to schedule a softclock.
 */
int
callout_hardclock(void)
{
	int needsoftclock;

	mutex_spin_enter(&callout_mutex);

	MOVEBUCKET(0, hardclock_ticks);
	if (MASKWHEEL(0, hardclock_ticks) == 0) {
		MOVEBUCKET(1, hardclock_ticks);
		if (MASKWHEEL(1, hardclock_ticks) == 0) {
			MOVEBUCKET(2, hardclock_ticks);
			if (MASKWHEEL(2, hardclock_ticks) == 0)
				MOVEBUCKET(3, hardclock_ticks);
		}
	}

	needsoftclock = !CIRCQ_EMPTY(&timeout_todo);
	mutex_spin_exit(&callout_mutex);

	return needsoftclock;
}

/* ARGSUSED */
void
softclock(void *v)
{
#ifdef MULTIPROCESSOR
	struct cpu_info *ci = curcpu();
#endif
	struct callout *c;
	void (*func)(void *);
	void *arg;

	mutex_spin_enter(&callout_mutex);

	while (!CIRCQ_EMPTY(&timeout_todo)) {
		c = CIRCQ_FIRST(&timeout_todo);
		CIRCQ_REMOVE(&c->c_list);

		/* If due run it, otherwise insert it into the right bucket. */
		if (c->c_time - hardclock_ticks > 0) {
			CIRCQ_INSERT(&c->c_list,
			    BUCKET((c->c_time - hardclock_ticks), c->c_time));
		} else {
#ifdef CALLOUT_EVENT_COUNTERS
			if (c->c_time - hardclock_ticks < 0)
				callout_ev_late.ev_count++;
#endif
			c->c_flags = (c->c_flags & ~CALLOUT_PENDING) |
			    (CALLOUT_FIRED|CALLOUT_INVOKING);

			func = c->c_func;
			arg = c->c_arg;

#ifdef MULTIPROCESSOR
			c->c_oncpu = ci;
			ci->ci_data.cpu_callout = c;
#endif
			mutex_spin_exit(&callout_mutex);
			(*func)(arg);
			mutex_spin_enter(&callout_mutex);
#ifdef MULTIPROCESSOR
			ci->ci_data.cpu_callout = NULL;
			/*
			 * we can't touch 'c' here because it might be
			 * freed already.
			 */
#endif
		}
	}

	mutex_spin_exit(&callout_mutex);
}

#ifdef DDB
static void
db_show_callout_bucket(struct callout_circq *bucket)
{
	struct callout *c;
	db_expr_t offset;
	const char *name;
	static char question[] = "?";

	if (CIRCQ_EMPTY(bucket))
		return;

	for (c = CIRCQ_FIRST(bucket); /*nothing*/; c = CIRCQ_NEXT(&c->c_list)) {
		db_find_sym_and_offset((db_addr_t)(intptr_t)c->c_func, &name,
		    &offset);
		name = name ? name : question;
#ifdef _LP64
#define	POINTER_WIDTH	"%16lx"
#else
#define	POINTER_WIDTH	"%8lx"
#endif
		db_printf("%9d %2d/%-4d " POINTER_WIDTH "  %s\n",
		    c->c_time - hardclock_ticks,
		    (int)((bucket - timeout_wheel) / WHEELSIZE),
		    (int)(bucket - timeout_wheel), (u_long) c->c_arg, name);

		if (CIRCQ_LAST(&c->c_list, bucket))
			break;
	}
}

void
db_show_callout(db_expr_t addr, bool haddr, db_expr_t count, const char *modif)
{
	int b;

	db_printf("hardclock_ticks now: %d\n", hardclock_ticks);
#ifdef _LP64
	db_printf("    ticks  wheel               arg  func\n");
#else
	db_printf("    ticks  wheel       arg  func\n");
#endif

	/*
	 * Don't lock the callwheel; all the other CPUs are paused
	 * anyhow, and we might be called in a circumstance where
	 * some other CPU was paused while holding the lock.
	 */

	db_show_callout_bucket(&timeout_todo);
	for (b = 0; b < BUCKETS; b++)
		db_show_callout_bucket(&timeout_wheel[b]);
}
#endif /* DDB */
