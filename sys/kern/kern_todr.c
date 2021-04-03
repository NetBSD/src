/*	$NetBSD: kern_todr.c,v 1.47 2021/04/03 12:06:53 simonb Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department and Ralph Campbell.
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
 * from: Utah Hdr: clock.c 1.18 91/01/21
 *
 *	@(#)clock.c	8.1 (Berkeley) 6/10/93
 */

#include "opt_todr.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_todr.c,v 1.47 2021/04/03 12:06:53 simonb Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/timetc.h>
#include <sys/intr.h>
#include <sys/rndsource.h>
#include <sys/mutex.h>

#include <dev/clock_subr.h>	/* hmm.. this should probably move to sys */

static int todr_gettime(todr_chip_handle_t, struct timeval *);
static int todr_settime(todr_chip_handle_t, struct timeval *);

static kmutex_t todr_mutex;
static todr_chip_handle_t todr_handle;
static bool todr_initialized;

/* The minimum reasonable RTC date before preposterousness */
#define	PREPOSTEROUS_YEARS	(2021 - POSIX_BASE_YEAR)

/*
 * todr_init:
 *	Initialize TOD clock data.
 */
void
todr_init(void)
{

	mutex_init(&todr_mutex, MUTEX_DEFAULT, IPL_NONE);
	todr_initialized = true;
}

/*
 * todr_lock:
 *	Acquire the TODR lock.
 */
void
todr_lock(void)
{

	mutex_enter(&todr_mutex);
}

/*
 * todr_unlock:
 *	Release the TODR lock.
 */
void
todr_unlock(void)
{

	mutex_exit(&todr_mutex);
}

/*
 * todr_lock_owned:
 *	Return true if the current thread owns the TODR lock.
 *	This is to be used by diagnostic assertions only.
 */
bool
todr_lock_owned(void)
{

	return mutex_owned(&todr_mutex) ? true : false;
}

/*
 * todr_attach:
 *	Attach the clock device to todr_handle.
 */
void
todr_attach(todr_chip_handle_t todr)
{

	/*
	 * todr_init() is called very early in main(), but this is
	 * here to catch a case where todr_attach() is called before
	 * main().
	 */
	KASSERT(todr_initialized);

	todr_lock();
	if (todr_handle) {
		todr_unlock();
		printf("todr_attach: TOD already configured\n");
		return;
	}
	todr_handle = todr;
	todr_unlock();
}

static bool timeset = false;

/*
 * todr_set_systime:
 *	Set up the system's time.  The "base" argument is a best-guess
 *	close-enough value to use if the TOD clock is unavailable or
 *	contains garbage.  Must be called with the TODR lock held.
 */
void
todr_set_systime(time_t base)
{
	bool badbase = false;
	bool waszero = (base == 0);
	bool goodtime = false;
	bool badrtc = false;
	struct timespec ts;
	struct timeval tv;

	KASSERT(todr_lock_owned());

	rnd_add_data(NULL, &base, sizeof(base), 0);

	if (base < 5 * SECS_PER_COMMON_YEAR) {
		struct clock_ymdhms basedate;

		/*
		 * If base is 0, assume filesystem time is just unknown
		 * instead of preposterous. Don't bark.
		 */
		if (base != 0)
			printf("WARNING: preposterous time in file system\n");
		/* not going to use it anyway, if the chip is readable */
		basedate.dt_year = 2010;
		basedate.dt_mon = 1;
		basedate.dt_day = 1;
		basedate.dt_hour = 12;
		basedate.dt_min = 0;
		basedate.dt_sec = 0;
		base = clock_ymdhms_to_secs(&basedate);
		badbase = true;
	}

	/*
	 * Some ports need to be supplied base in order to fabricate a time_t.
	 */
	if (todr_handle)
		todr_handle->base_time = base;

	memset(&tv, 0, sizeof(tv));

	if ((todr_handle == NULL) ||
	    (todr_gettime(todr_handle, &tv) != 0) ||
	    (tv.tv_sec < (PREPOSTEROUS_YEARS * SECS_PER_COMMON_YEAR))) {

		if (todr_handle != NULL)
			printf("WARNING: preposterous TOD clock time\n");
		else
			printf("WARNING: no TOD clock present\n");
		badrtc = true;
	} else {
		time_t deltat = tv.tv_sec - base;

		if (deltat < 0)
			deltat = -deltat;

		if (!badbase && deltat >= 2 * SECS_PER_DAY) {
			
			if (tv.tv_sec < base) {
				/*
				 * The clock should never go backwards
				 * relative to filesystem time.  If it
				 * does by more than the threshold,
				 * believe the filesystem.
				 */
				printf("WARNING: clock lost %" PRId64 " days\n",
				    deltat / SECS_PER_DAY);
				badrtc = true;
			} else {
				aprint_verbose("WARNING: clock gained %" PRId64
				    " days\n", deltat / SECS_PER_DAY);
				goodtime = true;
			}
		} else {
			goodtime = true;
		}

		rnd_add_data(NULL, &tv, sizeof(tv), 0);
	}

	/* if the rtc time is bad, use the filesystem time */
	if (badrtc) {
		if (badbase) {
			printf("WARNING: using default initial time\n");
		} else {
			printf("WARNING: using filesystem time\n");
		}
		tv.tv_sec = base;
		tv.tv_usec = 0;
	}

	timeset = true;

	ts.tv_sec = tv.tv_sec;
	ts.tv_nsec = tv.tv_usec * 1000;
	tc_setclock(&ts);

	if (waszero || goodtime)
		return;

	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * todr_save_systime:
 *	Save the current system time back to the TOD clock.
 *	Must be called with the TODR lock held.
 */
void
todr_save_systime(void)
{
	struct timeval tv;

	KASSERT(todr_lock_owned());

	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip if we don't know what time
	 * it is.
	 */
	if (!timeset)
		return;

	getmicrotime(&tv);

	if (tv.tv_sec == 0)
		return;

	if (todr_handle)
		if (todr_settime(todr_handle, &tv) != 0)
			printf("Cannot set TOD clock time\n");
}

/*
 * inittodr:
 *	Legacy wrapper around todr_set_systime().
 */
void
inittodr(time_t base)
{

	todr_lock();
	todr_set_systime(base);
	todr_unlock();
}

/*
 * resettodr:
 *	Legacy wrapper around todr_save_systime().
 */
void
resettodr(void)
{

	/*
	 * If we're shutting down, we don't want to get stuck in case
	 * someone was already fiddling with the TOD clock.
	 */
	if (shutting_down) {
		if (mutex_tryenter(&todr_mutex) == 0) {
			printf("WARNING: Cannot set TOD clock time (busy)\n");
			return;
		}
	} else {
		todr_lock();
	}
	todr_save_systime();
	todr_unlock();
}

#ifdef	TODR_DEBUG
static void
todr_debug(const char *prefix, int rv, struct clock_ymdhms *dt,
    struct timeval *tvp)
{
	struct timeval tv_val;
	struct clock_ymdhms dt_val;

	if (dt == NULL) {
		clock_secs_to_ymdhms(tvp->tv_sec, &dt_val);
		dt = &dt_val;
	}
	if (tvp == NULL) {
		tvp = &tv_val;
		tvp->tv_sec = clock_ymdhms_to_secs(dt);
		tvp->tv_usec = 0;
	}
	printf("%s: rv = %d\n", prefix, rv);
	printf("%s: rtc_offset = %d\n", prefix, rtc_offset);
	printf("%s: %4u/%02u/%02u %02u:%02u:%02u, (wday %d) (epoch %u.%06u)\n",
	    prefix,
	    (unsigned)dt->dt_year, dt->dt_mon, dt->dt_day,
	    dt->dt_hour, dt->dt_min, dt->dt_sec,
	    dt->dt_wday, (unsigned)tvp->tv_sec, (unsigned)tvp->tv_usec);
}
#else	/* !TODR_DEBUG */
#define	todr_debug(prefix, rv, dt, tvp)
#endif	/* TODR_DEBUG */

static int
todr_wenable(todr_chip_handle_t todr, int onoff)
{

	if (todr->todr_setwen != NULL)
		return todr->todr_setwen(todr, onoff);
	
	return 0;
}

#define	ENABLE_TODR_WRITES()						\
do {									\
	if ((rv = todr_wenable(tch, 1)) != 0) {				\
		printf("%s: cannot enable TODR writes\n", __func__);	\
		return rv;						\
	}								\
} while (/*CONSTCOND*/0)

#define	DISABLE_TODR_WRITES()						\
do {									\
	if (todr_wenable(tch, 0) != 0)					\
		printf("%s: WARNING: cannot disable TODR writes\n",	\
		    __func__);						\
} while (/*CONSTCOND*/0)

static int
todr_gettime(todr_chip_handle_t tch, struct timeval *tvp)
{
	int rv;

	/*
	 * Write-enable is used even when reading the TODR because
	 * writing to registers may be required in order to do so.
	 */

	if (tch->todr_gettime) {
		ENABLE_TODR_WRITES();
		rv = tch->todr_gettime(tch, tvp);
		DISABLE_TODR_WRITES();
		/*
		 * Some unconverted ports have their own references to
		 * rtc_offset.   A converted port must not do that.
		 */
		if (rv == 0)
			tvp->tv_sec += rtc_offset * 60;
		todr_debug("TODR-GET-SECS", rv, NULL, tvp);
		return rv;
	} else if (tch->todr_gettime_ymdhms) {
		struct clock_ymdhms dt = { 0 };
		ENABLE_TODR_WRITES();
		rv = tch->todr_gettime_ymdhms(tch, &dt);
		DISABLE_TODR_WRITES();
		todr_debug("TODR-GET-YMDHMS", rv, &dt, NULL);
		if (rv)
			return rv;

		/*
		 * Simple sanity checks.  Note that this includes a
		 * value for clocks that can return a leap second.
		 * Note that we don't support double leap seconds,
		 * since this was apparently an error/misunderstanding
		 * on the part of the ISO C committee, and can never
		 * actually occur.  If your clock issues us a double
		 * leap second, it must be broken.  Ultimately, you'd
		 * have to be trying to read time at precisely that
		 * instant to even notice, so even broken clocks will
		 * work the vast majority of the time.  In such a case
		 * it is recommended correction be applied in the
		 * clock driver.
		 */
		if (dt.dt_mon < 1 || dt.dt_mon > 12 ||
		    dt.dt_day < 1 || dt.dt_day > 31 ||
		    dt.dt_hour > 23 || dt.dt_min > 59 || dt.dt_sec > 60) {
			return EINVAL;
		}
		tvp->tv_sec = clock_ymdhms_to_secs(&dt) + rtc_offset * 60;
		tvp->tv_usec = 0;
		return tvp->tv_sec < 0 ? EINVAL : 0;
	}

	return ENXIO;
}

static int
todr_settime(todr_chip_handle_t tch, struct timeval *tvp)
{
	int rv;

	if (tch->todr_settime) {
		struct timeval copy = *tvp;
		copy.tv_sec -= rtc_offset * 60;
		ENABLE_TODR_WRITES();
		rv = tch->todr_settime(tch, &copy);
		DISABLE_TODR_WRITES();
		todr_debug("TODR-SET-SECS", rv, NULL, tvp);
		return rv;
	} else if (tch->todr_settime_ymdhms) {
		struct clock_ymdhms dt;
		time_t sec = tvp->tv_sec - rtc_offset * 60;
		if (tvp->tv_usec >= 500000)
			sec++;
		clock_secs_to_ymdhms(sec, &dt);
		ENABLE_TODR_WRITES();
		rv = tch->todr_settime_ymdhms(tch, &dt);
		DISABLE_TODR_WRITES();
		todr_debug("TODR-SET-YMDHMS", rv, &dt, NULL);
		return rv;
	}

	return ENXIO;
}
