/*	$NetBSD: timevar.h,v 1.51 2023/07/17 13:44:24 riastradh Exp $	*/

/*
 *  Copyright (c) 2005, 2008, 2020 The NetBSD Foundation, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 *  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 *  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)time.h	8.5 (Berkeley) 5/4/95
 */

#ifndef _SYS_TIMEVAR_H_
#define _SYS_TIMEVAR_H_

#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/queue.h>
#include <sys/signal.h>
#include <sys/systm.h>

struct itimer;
LIST_HEAD(itlist, itimer);

/*
 * Interval timer operations vector.
 *
 * Required fields:
 *
 *	- ito_fire: A function to be called when the itimer fires.
 *	  The timer implementation should perform whatever processing
 *	  is necessary for that timer type.
 *
 * Optional fields:
 *
 *	- ito_realtime_changed: A function that is called when the system
 *	  time (CLOCK_REALTIME) is called.
 */
struct itimer_ops {
	void	(*ito_fire)(struct itimer *);
	void	(*ito_realtime_changed)(struct itimer *);
};

/*
 * Common interval timer data.
 */
struct itimer {
	union {
		struct {
			callout_t		it_ch;
			LIST_ENTRY(itimer)	it_rtchgq;
		} it_real;
		struct {
			struct itlist		*it_vlist;
			LIST_ENTRY(itimer)	it_list;
			bool			it_active;
		} it_virtual;
	};
	const struct itimer_ops *it_ops;
	struct itimerspec it_time;
	clockid_t it_clockid;
	int	it_overruns;	/* Overruns currently accumulating */
	bool	it_dying;
};

#define	it_ch		it_real.it_ch
#define	it_rtchgq	it_real.it_rtchgq

#define	it_vlist	it_virtual.it_vlist
#define	it_list		it_virtual.it_list
#define	it_active	it_virtual.it_active

/*
 * Structure used to manage timers in a process.
 */
struct ptimer {
	struct itimer pt_itimer;/* common interval timer data */

	TAILQ_ENTRY(ptimer) pt_chain; /* link in signalling queue */
	struct	sigevent pt_ev;	/* event notification info */
	int	pt_poverruns;	/* Overruns associated w/ a delivery */
	int	pt_entry;	/* slot in proc's timer table */
	struct proc *pt_proc;	/* associated process */
	bool	pt_queued;	/* true if linked into signalling queue */
};

#define	TIMER_MIN	4	/* [0..3] are reserved for setitimer(2) */
				/* REAL=0,VIRTUAL=1,PROF=2,MONOTONIC=3 */
#define	TIMER_MAX	36	/* 32 is minimum user timers per POSIX */
#define	TIMERS_ALL	0
#define	TIMERS_POSIX	1

struct ptimers {
	struct itlist pts_virtual;
	struct itlist pts_prof;
	struct itimer *pts_timers[TIMER_MAX];
};

/*
 * Functions for looking at our clock: [get]{bin,nano,micro}[up]time()
 *
 * Functions without the "get" prefix returns the best timestamp
 * we can produce in the given format.
 *
 * "bin"   == struct bintime  == seconds + 64 bit fraction of seconds.
 * "nano"  == struct timespec == seconds + nanoseconds.
 * "micro" == struct timeval  == seconds + microseconds.
 *
 * Functions containing "up" returns time relative to boot and
 * should be used for calculating time intervals.
 *
 * Functions without "up" returns GMT time.
 *
 * Functions with the "get" prefix returns a less precise result
 * much faster than the functions without "get" prefix and should
 * be used where a precision of 1/HZ (eg 10 msec on a 100HZ machine)
 * is acceptable or where performance is priority.
 * (NB: "precision", _not_ "resolution" !)
 *
 */

void	binuptime(struct bintime *);
void	nanouptime(struct timespec *);
void	microuptime(struct timeval *);

void	bintime(struct bintime *);
void	nanotime(struct timespec *);
void	microtime(struct timeval *);

void	getbinuptime(struct bintime *);
void	getnanouptime(struct timespec *);
void	getmicrouptime(struct timeval *);

void	getbintime(struct bintime *);
void	getnanotime(struct timespec *);
void	getmicrotime(struct timeval *);

void	getbinboottime(struct bintime *);
void	getnanoboottime(struct timespec *);
void	getmicroboottime(struct timeval *);

/* Other functions */
int	ts2timo(clockid_t, int, struct timespec *, int *, struct timespec *);
void	adjtime1(const struct timeval *, struct timeval *, struct proc *);
int	clock_getres1(clockid_t, struct timespec *);
int	clock_gettime1(clockid_t, struct timespec *);
int	clock_settime1(struct proc *, clockid_t, const struct timespec *, bool);
void	clock_timeleft(clockid_t, struct timespec *, struct timespec *);
int	dogetitimer(struct proc *, int, struct itimerval *);
int	dosetitimer(struct proc *, int, struct itimerval *);
int	dotimer_gettime(int, struct proc *, struct itimerspec *);
int	dotimer_settime(int, struct itimerspec *, struct itimerspec *, int,
	    struct proc *);
int	tshzto(const struct timespec *);
int	tshztoup(const struct timespec *);
int	tvhzto(const struct timeval *);
void	inittimecounter(void);
int	itimerfix(struct timeval *);
int	itimespecfix(struct timespec *);
int	ppsratecheck(struct timeval *, int *, int);
int	ratecheck(struct timeval *, const struct timeval *);
int	settime(struct proc *p, struct timespec *);
int	nanosleep1(struct lwp *, clockid_t, int, struct timespec *,
	    struct timespec *);
int	settimeofday1(const struct timeval *, bool,
	    const void *, struct lwp *, bool);
int	timer_create1(timer_t *, clockid_t, struct sigevent *, copyin_t,
	    struct lwp *);
int	tstohz(const struct timespec *);
int	tvtohz(const struct timeval *);
int	inittimeleft(struct timespec *, struct timespec *);
int	gettimeleft(struct timespec *, struct timespec *);
void	timerupcall(struct lwp *);
void	time_init(void);
bool	time_wraps(struct timespec *, struct timespec *);

void	itimer_init(struct itimer *, const struct itimer_ops *,
	    clockid_t, struct itlist *);
void	itimer_poison(struct itimer *);
void	itimer_fini(struct itimer *);

void	itimer_lock(void);
void	itimer_unlock(void);
bool	itimer_lock_held(void);		/* for diagnostic assertions only */
int	itimer_settime(struct itimer *);
void	itimer_gettime(const struct itimer *, struct itimerspec *);

void	ptimer_tick(struct lwp *, bool);
void	ptimers_free(struct proc *, int);

#define	time_second	getrealtime()
#define	time_uptime	getuptime()
#define	time_uptime32	getuptime32()

#ifdef __HAVE_ATOMIC64_LOADSTORE

extern volatile time_t time__second;	/* current second in the epoch */
extern volatile time_t time__uptime;	/* system uptime in seconds */

static inline time_t
getrealtime(void)
{
	return atomic_load_relaxed(&time__second);
}

static inline time_t
getuptime(void)
{
	return atomic_load_relaxed(&time__uptime);
}

static inline time_t
getboottime(void)
{
	return getrealtime() - getuptime();
}

static inline uint32_t
getuptime32(void)
{
	return getuptime() & 0xffffffff;
}

#else

time_t		getrealtime(void);
time_t		getuptime(void);
time_t		getboottime(void);
uint32_t	getuptime32(void);

#endif

extern int time_adjusted;

#define	DEFAULT_TIMEOUT_EPSILON						      \
	(&(const struct bintime) {					      \
		.sec = 0,						      \
		.frac = ((uint64_t)1 << 32)/hz << 32,			      \
	})

static __inline time_t time_mono_to_wall(time_t t)
{

	return t + getboottime();
}

static __inline time_t time_wall_to_mono(time_t t)
{

	return t - getboottime();
}

#endif /* !_SYS_TIMEVAR_H_ */
