/*	$NetBSD: kern_timeout.c,v 1.7 2003/07/14 14:59:01 lukem Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: kern_timeout.c,v 1.7 2003/07/14 14:59:01 lukem Exp $");

/*
 * Adapted from OpenBSD: kern_timeout.c,v 1.15 2002/12/08 04:21:07 art Exp,
 * modified to match NetBSD's pre-existing callout API.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/callout.h>

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
static struct simplelock callout_slock;

#define	CALLOUT_LOCK(s)							\
do {									\
	s = splsched();							\
	simple_lock(&callout_slock);					\
} while (/*CONSTCOND*/0)

#define	CALLOUT_UNLOCK(s)						\
do {									\
	simple_unlock(&callout_slock);					\
	splx((s));							\
} while (/*CONSTCOND*/0)

/*
 * Circular queue definitions.
 */

#define CIRCQ_INIT(elem)						\
do {									\
        (elem)->cq_next = (elem);					\
        (elem)->cq_prev = (elem);					\
} while (/*CONSTCOND*/0)

#define CIRCQ_INSERT(elem, list)					\
do {									\
        (elem)->cq_prev = (list)->cq_prev;				\
        (elem)->cq_next = (list);					\
        (list)->cq_prev->cq_next = (elem);				\
        (list)->cq_prev = (elem);					\
} while (/*CONSTCOND*/0)

#define CIRCQ_APPEND(fst, snd)						\
do {									\
        if (!CIRCQ_EMPTY(snd)) {					\
                (fst)->cq_prev->cq_next = (snd)->cq_next;		\
                (snd)->cq_next->cq_prev = (fst)->cq_prev;		\
                (snd)->cq_prev->cq_next = (fst);			\
                (fst)->cq_prev = (snd)->cq_prev;			\
                CIRCQ_INIT(snd);					\
        }								\
} while (/*CONSTCOND*/0)

#define CIRCQ_REMOVE(elem)						\
do {									\
        (elem)->cq_next->cq_prev = (elem)->cq_prev;			\
        (elem)->cq_prev->cq_next = (elem)->cq_next;			\
} while (/*CONSTCOND*/0)

#define CIRCQ_FIRST(elem) ((elem)->cq_next)

#define CIRCQ_EMPTY(elem) (CIRCQ_FIRST(elem) == (elem))

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
	simple_lock_init(&callout_slock);

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
 * callout_setfunc:
 *
 *	Initialize a callout structure and set the function and
 *	argument.
 */
void
callout_setfunc(struct callout *c, void (*func)(void *), void *arg)
{

	memset(c, 0, sizeof(*c));
	c->c_func = func;
	c->c_arg = arg;
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
	int s, old_time;

	KASSERT(to_ticks >= 0);

	CALLOUT_LOCK(s);

	/* Initialize the time here, it won't change. */
	old_time = c->c_time;
	c->c_time = to_ticks + hardclock_ticks;
	c->c_flags &= ~CALLOUT_FIRED;

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

	CALLOUT_UNLOCK(s);
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
	int s, old_time;

	KASSERT(to_ticks >= 0);

	CALLOUT_LOCK(s);

	/* Initialize the time here, it won't change. */
	old_time = c->c_time;
	c->c_time = to_ticks + hardclock_ticks;
	c->c_flags &= ~CALLOUT_FIRED;

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

	CALLOUT_UNLOCK(s);
}

/*
 * callout_stop:
 *
 *	Cancel a pending callout.
 */
void
callout_stop(struct callout *c)
{
	int s;

	CALLOUT_LOCK(s);

	if (callout_pending(c))
		CIRCQ_REMOVE(&c->c_list);

	c->c_flags &= ~(CALLOUT_PENDING|CALLOUT_FIRED);

	CALLOUT_UNLOCK(s);
}

/*
 * This is called from hardclock() once every tick.
 * We return !0 if we need to schedule a softclock.
 */
int
callout_hardclock(void)
{
	int s;
	int needsoftclock;

	CALLOUT_LOCK(s);

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
	CALLOUT_UNLOCK(s);

	return needsoftclock;
}

/* ARGSUSED */
void
softclock(void *v)
{
	struct callout *c;
	void (*func)(void *);
	void *arg;
	int s;

	CALLOUT_LOCK(s);

	while (!CIRCQ_EMPTY(&timeout_todo)) {

		c = (struct callout *)CIRCQ_FIRST(&timeout_todo); /* XXX */
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
			c->c_flags = (c->c_flags  & ~CALLOUT_PENDING) |
			    CALLOUT_FIRED;

			func = c->c_func;
			arg = c->c_arg;

			CALLOUT_UNLOCK(s);
			(*func)(arg);
			CALLOUT_LOCK(s);
		}
	}

	CALLOUT_UNLOCK(s);
}

#ifdef DDB
static void
db_show_callout_bucket(struct callout_circq *bucket)
{
	struct callout *c;
	struct callout_circq *p;
	db_expr_t offset;
	char *name;

	for (p = CIRCQ_FIRST(bucket); p != bucket; p = CIRCQ_FIRST(p)) {
		c = (struct callout *)p; /* XXX */
		db_find_sym_and_offset((db_addr_t)c->c_func, &name, &offset);
		name = name ? name : "?";
#ifdef _LP64
#define	POINTER_WIDTH	"%16lx"
#else
#define	POINTER_WIDTH	"%8lx"
#endif
		db_printf("%9d %2d/%-4d " POINTER_WIDTH "  %s\n",
		    c->c_time - hardclock_ticks,
		    (int)((bucket - timeout_wheel) / WHEELSIZE),
		    (int)(bucket - timeout_wheel), (u_long) c->c_arg, name);
	}
}

void
db_show_callout(db_expr_t addr, int haddr, db_expr_t count, char *modif)
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
