/*	$NetBSD: kern_timeout.c,v 1.21.4.3 2007/07/01 21:37:34 ad Exp $	*/

/*-
 * Copyright (c) 2003, 2006, 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, and by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: kern_timeout.c,v 1.21.4.3 2007/07/01 21:37:34 ad Exp $");

/*
 * Timeouts are kept in a hierarchical timing wheel.  The c_time is the
 * value of the global variable "hardclock_ticks" when the timeout should
 * be called.  There are four levels with 256 buckets each. See 'Scheme 7'
 * in "Hashed and Hierarchical Timing Wheels: Efficient Data Structures
 * for Implementing a Timer Facility" by George Varghese and Tony Lauck.
 *
 * Some of the "math" in here is a bit tricky.  We have to beware of
 * wrapping ints.
 *
 * We use the fact that any element added to the queue must be added with
 * a positive time.  That means that any element `to' on the queue cannot
 * be scheduled to timeout further in time than INT_MAX, but c->c_time can
 * be positive or negative so comparing it with anything is dangerous. 
 * The only way we can use the c->c_time value in any predictable way is
 * when we calculate how far in the future `to' will timeout - "c->c_time
 * - hardclock_ticks".  The result will always be positive for future
 * timeouts and 0 or negative for due timeouts.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/lock.h>
#include <sys/callout.h>
#include <sys/mutex.h>
#include <sys/proc.h>
#include <sys/sleepq.h>
#include <sys/syncobj.h>
#include <sys/intr.h>

#ifdef DDB
#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_access.h>
#include <ddb/db_sym.h>
#include <ddb/db_output.h>
#endif

#define BUCKETS		1024
#define WHEELSIZE	256
#define WHEELMASK	255
#define WHEELBITS	8

/* The following funkyness is to appease gcc3's strict aliasing. */
struct callout_circq {
	/* next element */
	union {
		struct callout_impl	*elem;
		struct callout_circq	*list;
	} cq_next;
	/* previous element */
	union {
		struct callout_impl	*elem;
		struct callout_circq	*list;
	} cq_prev;
};
#define	cq_next_e	cq_next.elem
#define	cq_prev_e	cq_prev.elem
#define	cq_next_l	cq_next.list
#define	cq_prev_l	cq_prev.list

typedef struct callout_impl {
	struct callout_circq c_list;		/* linkage on queue */
	void	(*c_func)(void *);		/* function to call */
	void	*c_arg;				/* function argument */
	void	*c_oncpu;			/* non-NULL while running */
	void	*c_onlwp;			/* non-NULL while running */
	int	c_time;				/* when callout fires */
	u_int	c_flags;			/* state of this entry */
	u_int	c_runwait;			/* number of waiters */
	u_int	c_magic;			/* magic number */
} callout_impl_t;
#define	CALLOUT_MAGIC		0x11deeba1

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

static void	callout_softclock(void *);

/*
 * All wheels are locked with the same lock (which must also block out
 * all interrupts).  Eventually this should become per-CPU.
 */
kmutex_t callout_lock;
sleepq_t callout_sleepq;
void	*callout_si;

static struct evcnt callout_ev_late;
static struct evcnt callout_ev_block;

/*
 * callout_barrier:
 *
 *	If the callout is already running, wait until it completes.
 *	XXX This should do priority inheritance.
 */
static void
callout_barrier(callout_impl_t *c)
{
	extern syncobj_t sleep_syncobj;
	struct cpu_info *ci;
	struct lwp *l;

	l = curlwp;

	if ((c->c_flags & CALLOUT_MPSAFE) == 0) {
		/*
		 * Note: we must be called with the kernel lock held,
		 * as we use it to synchronize with callout_softclock().
		 */
		ci = c->c_oncpu;
		ci->ci_data.cpu_callout_cancel = c;
		return;
	}

	while ((ci = c->c_oncpu) != NULL && ci->ci_data.cpu_callout == c) {
		KASSERT(l->l_wchan == NULL);

		ci->ci_data.cpu_callout_nwait++;
		callout_ev_block.ev_count++;

		lwp_lock(l);
		lwp_unlock_to(l, &callout_lock);
		sleepq_enqueue(&callout_sleepq, sched_kpri(l), ci,
		    "callout", &sleep_syncobj);
		sleepq_block(0, false);
		mutex_spin_enter(&callout_lock);
	}
}

/*
 * callout_running:
 *
 *	Return non-zero if callout 'c' is currently executing.
 */
static inline bool
callout_running(callout_impl_t *c)
{
	struct cpu_info *ci;

	if ((ci = c->c_oncpu) == NULL)
		return false;
	if (ci->ci_data.cpu_callout != c)
		return false;
	if (c->c_onlwp == curlwp)
		return false;
	return true;
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

	KASSERT(sizeof(callout_impl_t) <= sizeof(callout_t));

	CIRCQ_INIT(&timeout_todo);
	for (b = 0; b < BUCKETS; b++)
		CIRCQ_INIT(&timeout_wheel[b]);

	mutex_init(&callout_lock, MUTEX_SPIN, IPL_SCHED);
	sleepq_init(&callout_sleepq, &callout_lock);

	evcnt_attach_dynamic(&callout_ev_late, EVCNT_TYPE_MISC,
	    NULL, "callout", "late");
	evcnt_attach_dynamic(&callout_ev_block, EVCNT_TYPE_MISC,
	    NULL, "callout", "block waiting");
}

/*
 * callout_startup2:
 *
 *	Complete initialization once soft interrupts are available.
 */
void
callout_startup2(void)
{

	callout_si = softint_establish(SOFTINT_CLOCK | SOFTINT_MPSAFE,
	    callout_softclock, NULL);
	if (callout_si == NULL)
		panic("callout_startup2: unable to register softclock intr");
}

/*
 * callout_init:
 *
 *	Initialize a callout structure.
 */
void
callout_init(callout_t *cs, u_int flags)
{
	callout_impl_t *c = (callout_impl_t *)cs;

	KASSERT((flags & ~CALLOUT_FLAGMASK) == 0);

	memset(c, 0, sizeof(*c));
	c->c_flags = flags;
	c->c_magic = CALLOUT_MAGIC;
}

/*
 * callout_destroy:
 *
 *	Destroy a callout structure.  The callout must be stopped.
 */
void
callout_destroy(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;

	/*
	 * It's not necessary to lock in order to see the correct value
	 * of c->c_flags.  If the callout could potentially have been
	 * running, the current thread should have stopped it.
	 */
	KASSERT((c->c_flags & CALLOUT_PENDING) == 0);
	if (c->c_oncpu != NULL) {
		KASSERT(
		    ((struct cpu_info *)c->c_oncpu)->ci_data.cpu_callout != c);
	}
	KASSERT(c->c_magic == CALLOUT_MAGIC);

	c->c_magic = 0;
}


/*
 * callout_reset:
 *
 *	Reset a callout structure with a new function and argument, and
 *	schedule it to run.
 */
void
callout_reset(callout_t *cs, int to_ticks, void (*func)(void *), void *arg)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	int old_time;

	KASSERT(to_ticks >= 0);
	KASSERT(c->c_magic == CALLOUT_MAGIC);
	KASSERT(func != NULL);

	mutex_spin_enter(&callout_lock);

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
	if ((c->c_flags & CALLOUT_PENDING) != 0) {
		if (c->c_time - old_time < 0) {
			CIRCQ_REMOVE(&c->c_list);
			CIRCQ_INSERT(&c->c_list, &timeout_todo);
		}
	} else {
		c->c_flags |= CALLOUT_PENDING;
		CIRCQ_INSERT(&c->c_list, &timeout_todo);
	}

	mutex_spin_exit(&callout_lock);
}

/*
 * callout_schedule:
 *
 *	Schedule a callout to run.  The function and argument must
 *	already be set in the callout structure.
 */
void
callout_schedule(callout_t *cs, int to_ticks)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	int old_time;

	KASSERT(to_ticks >= 0);
	KASSERT(c->c_magic == CALLOUT_MAGIC);
	KASSERT(c->c_func != NULL);

	mutex_spin_enter(&callout_lock);

	/* Initialize the time here, it won't change. */
	old_time = c->c_time;
	c->c_time = to_ticks + hardclock_ticks;
	c->c_flags &= ~CALLOUT_FIRED;

	/*
	 * If this timeout is already scheduled and now is moved
	 * earlier, reschedule it now. Otherwise leave it in place
	 * and let it be rescheduled later.
	 */
	if ((c->c_flags & CALLOUT_PENDING) != 0) {
		if (c->c_time - old_time < 0) {
			CIRCQ_REMOVE(&c->c_list);
			CIRCQ_INSERT(&c->c_list, &timeout_todo);
		}
	} else {
		c->c_flags |= CALLOUT_PENDING;
		CIRCQ_INSERT(&c->c_list, &timeout_todo);
	}

	mutex_spin_exit(&callout_lock);
}

/*
 * callout_stop:
 *
 *	Cancel a pending callout.
 */
bool
callout_stop(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	bool expired;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);

	if (callout_running(c))
		callout_barrier(c);

	if ((c->c_flags & CALLOUT_PENDING) != 0)
		CIRCQ_REMOVE(&c->c_list);

	expired = ((c->c_flags & CALLOUT_FIRED) != 0);
	c->c_flags &= ~(CALLOUT_PENDING|CALLOUT_FIRED);

	mutex_spin_exit(&callout_lock);

	return expired;
}

void
callout_setfunc(callout_t *cs, void (*func)(void *), void *arg)
{
	callout_impl_t *c = (callout_impl_t *)cs;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);
	c->c_func = func;
	c->c_arg = arg;
	mutex_spin_exit(&callout_lock);
}

bool
callout_expired(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	bool rv;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);
	rv = ((c->c_flags & CALLOUT_FIRED) != 0);
	mutex_spin_exit(&callout_lock);

	return rv;
}

bool
callout_active(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	bool rv;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);
	rv = ((c->c_flags & (CALLOUT_PENDING|CALLOUT_FIRED)) != 0);
	mutex_spin_exit(&callout_lock);

	return rv;
}

bool
callout_pending(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	bool rv;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);
	rv = ((c->c_flags & CALLOUT_PENDING) != 0);
	mutex_spin_exit(&callout_lock);

	return rv;
}

bool
callout_invoking(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;
	bool rv;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);
	rv = ((c->c_flags & CALLOUT_INVOKING) != 0);
	mutex_spin_exit(&callout_lock);

	return rv;
}

void
callout_ack(callout_t *cs)
{
	callout_impl_t *c = (callout_impl_t *)cs;

	KASSERT(c->c_magic == CALLOUT_MAGIC);

	mutex_spin_enter(&callout_lock);
	c->c_flags &= ~CALLOUT_INVOKING;
	mutex_spin_exit(&callout_lock);
}

/*
 * This is called from hardclock() once every tick.
 * We schedule callout_softclock() if there is work
 * to be done.
 */
void
callout_hardclock(void)
{
	int needsoftclock;

	mutex_spin_enter(&callout_lock);

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
	mutex_spin_exit(&callout_lock);

	if (needsoftclock)
		softint_schedule(callout_si);
}

/* ARGSUSED */
static void
callout_softclock(void *v)
{
	callout_impl_t *c;
	struct cpu_info *ci;
	void (*func)(void *);
	void *arg;
	u_int mpsafe, count;
	lwp_t *l;

	l = curlwp;
	ci = l->l_cpu;

	mutex_spin_enter(&callout_lock);

	while (!CIRCQ_EMPTY(&timeout_todo)) {
		c = CIRCQ_FIRST(&timeout_todo);
		KASSERT(c->c_magic == CALLOUT_MAGIC);
		KASSERT(c->c_func != NULL);
		CIRCQ_REMOVE(&c->c_list);

		/* If due run it, otherwise insert it into the right bucket. */
		if (c->c_time - hardclock_ticks > 0) {
			CIRCQ_INSERT(&c->c_list,
			    BUCKET((c->c_time - hardclock_ticks), c->c_time));
		} else {
			if (c->c_time - hardclock_ticks < 0)
				callout_ev_late.ev_count++;

			c->c_flags ^= (CALLOUT_PENDING | CALLOUT_FIRED);
			mpsafe = (c->c_flags & CALLOUT_MPSAFE);
			func = c->c_func;
			arg = c->c_arg;
			c->c_oncpu = ci;
			c->c_onlwp = l;

			mutex_spin_exit(&callout_lock);
			if (!mpsafe) {
				KERNEL_LOCK(1, curlwp);
				if (ci->ci_data.cpu_callout_cancel != c)
					(*func)(arg);
				KERNEL_UNLOCK_ONE(curlwp);
			} else
					(*func)(arg);
			mutex_spin_enter(&callout_lock);

			/*
			 * We can't touch 'c' here because it might be
			 * freed already.  If LWPs waiting for callout
			 * to complete, awaken them.
			 */
			ci->ci_data.cpu_callout_cancel = NULL;
			ci->ci_data.cpu_callout = NULL;
			if ((count = ci->ci_data.cpu_callout_nwait) != 0) {
				ci->ci_data.cpu_callout_nwait = 0;
				/* sleepq_wake() drops the lock. */
				sleepq_wake(&callout_sleepq, ci, count);
				mutex_spin_enter(&callout_lock);
			}
		}
	}

	mutex_spin_exit(&callout_lock);
}

#ifdef DDB
static void
db_show_callout_bucket(struct callout_circq *bucket)
{
	callout_impl_t *c;
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
