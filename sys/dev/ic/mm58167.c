/*	$NetBSD: mm58167.c,v 1.1.4.3 2001/08/24 00:09:32 nathanw Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matthew Fredette.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * National Semiconductor MM58167 time-of-day chip subroutines.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <dev/clock_subr.h>
#include <dev/ic/mm58167var.h>

int mm58167_gettime __P((todr_chip_handle_t, struct timeval *));
int mm58167_settime __P((todr_chip_handle_t, struct timeval *));
int mm58167_getcal __P((todr_chip_handle_t, int *));
int mm58167_setcal __P((todr_chip_handle_t, int));

/*
 * To quote SunOS's todreg.h:
 * "This brain damaged chip insists on keeping the time in
 *  MM/DD HH:MM:SS format, even though it doesn't know about
 *  leap years and Feb. 29, thus making it nearly worthless."
 */
#define mm58167_read(sc, r) bus_space_read_1(sc->mm58167_regt, sc->mm58167_regh, sc-> r)
#define mm58167_write(sc, r, v) bus_space_write_1(sc->mm58167_regt, sc->mm58167_regh, sc-> r, v)

todr_chip_handle_t
mm58167_attach(sc)
	struct mm58167_softc *sc;
{
	struct todr_chip_handle *handle;
	
	printf(": mm58167");

	handle = &sc->_mm58167_todr_handle;
	memset(handle, 0, sizeof(handle));
	handle->cookie = sc;
	handle->todr_gettime = mm58167_gettime;
	handle->todr_settime = mm58167_settime;
	handle->todr_getcal = mm58167_getcal;
	handle->todr_setcal = mm58167_setcal;
	return (handle);
}

/*
 * Set up the system's time, given a `reasonable' time value.
 */
int
mm58167_gettime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct mm58167_softc *sc = handle->cookie;
	struct clock_ymdhms dt_hardware;
	struct clock_ymdhms dt_reasonable;
	int s;
	u_int8_t byte_value;
	int leap_year, had_leap_day;

	/* First, read the date out of the chip. */

	/* No interrupts while we're in the chip. */
	s = splhigh();

	/* Reset the status bit: */
	byte_value = mm58167_read(sc, mm58167_status);

	/*
	 * Read the date values until we get a coherent read (one
	 * where the status stays zero, indicating no increment was
	 * rippling through while we were reading).
	 */
	do {
#define _MM58167_GET(dt_f, mm_f) byte_value = mm58167_read(sc, mm_f); dt_hardware.dt_f = FROMBCD(byte_value)
		_MM58167_GET(dt_mon, mm58167_mon);
		_MM58167_GET(dt_day, mm58167_day);
		_MM58167_GET(dt_hour, mm58167_hour);
		_MM58167_GET(dt_min, mm58167_min);
		_MM58167_GET(dt_sec, mm58167_sec);
#undef _MM58167_GET
	} while ((mm58167_read(sc, mm58167_status) & 1) == 0);

	splx(s);

	/* Convert the reasonable time into a date: */
	clock_secs_to_ymdhms(tv->tv_sec, &dt_reasonable);

	/*
	 * We need to fake a hardware year.  if the hardware MM/DD
	 * HH:MM:SS date is less than the reasonable MM/DD
	 * HH:MM:SS, call it the reasonable year plus one, else call
	 * it the reasonable year.
	 */
	if (dt_hardware.dt_mon < dt_reasonable.dt_mon ||
	    (dt_hardware.dt_mon == dt_reasonable.dt_mon &&
		(dt_hardware.dt_day < dt_reasonable.dt_day ||
		    (dt_hardware.dt_day == dt_reasonable.dt_day &&
			(dt_hardware.dt_hour < dt_reasonable.dt_hour ||
			    (dt_hardware.dt_hour == dt_reasonable.dt_hour &&
				(dt_hardware.dt_min < dt_reasonable.dt_min ||
				    (dt_hardware.dt_min == dt_reasonable.dt_min &&
					(dt_hardware.dt_sec < dt_reasonable.dt_sec))))))))) {
	  dt_hardware.dt_year = dt_reasonable.dt_year + 1;
	} else {
	  dt_hardware.dt_year = dt_reasonable.dt_year;
	}

	/* convert the hardware date into a time: */
	tv->tv_sec = clock_ymdhms_to_secs(&dt_hardware);
	tv->tv_usec = 0;
	
	/*
	 * Make a reasonable effort to see if a leap day has passed
	 * that we need to account for.  This does the right thing
	 * only when the system was shut down before a leap day, and
	 * it is now after that leap day.  It doesn't do the right
	 * thing when a leap day happened while the machine was last
	 * up.  When that happens, the hardware clock becomes
	 * instantly wrong forever, until it gets fixed for some
	 * reason.  Use NTP to deal.
	 */

	/*
	 * This may have happened if the hardware says we're into
	 * March in the following year.  Check that following year for
	 * a leap day.
	 */
	if (dt_hardware.dt_year > dt_reasonable.dt_year &&
	    dt_hardware.dt_mon >= 3) {
	  leap_year = dt_hardware.dt_year;
	}

	/*
	 * This may have happened if the hardware says we're in the
	 * following year, and the system was shut down before March
	 * the previous year.  check that previous year for a leap
	 * day.
	 */
	else if (dt_hardware.dt_year > dt_reasonable.dt_year &&
		 dt_reasonable.dt_mon < 3) {
	  leap_year = dt_reasonable.dt_year;
	}

	/*
	 * This may have happened if the hardware says we're in the
	 * same year, but we weren't to March before, and we're in or
	 * past March now.  Check this year for a leap day.
	 */
	else if (dt_hardware.dt_year == dt_reasonable.dt_year
		 && dt_reasonable.dt_mon < 3
		 && dt_hardware.dt_mon >= 3) {
	  leap_year = dt_reasonable.dt_year;
	}

	/*
	 * Otherwise, no leap year to check.
	 */
	else {
	  leap_year = 0;
	}

	/* Do the real leap day check. */
	had_leap_day = 0;
	if (leap_year > 0) {
		if ((leap_year & 3) == 0) {
			had_leap_day = 1;
			if ((leap_year % 100) == 0) {
				had_leap_day = 0;
				if ((leap_year % 400) == 0)
					had_leap_day = 1;
			}
		}
	}

	/*
	 * If we had a leap day, adjust the value we will return, and
	 * also update the hardware clock.
	 */	
	/* 
	 * XXX - Since this update just writes back a corrected
	 * version of what we read out above, we lose whatever 
	 * amount of time the clock has advanced since that read.
	 * Use NTP to deal.
	 */
	if (had_leap_day) {
	  tv->tv_sec += SECDAY;
	  todr_settime(handle, tv);
	}

	return (0);
}

int
mm58167_settime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct mm58167_softc *sc = handle->cookie;
	struct clock_ymdhms dt_hardware;
	int s;
	u_int8_t byte_value;

	/* Convert the seconds into ymdhms. */
	clock_secs_to_ymdhms(tv->tv_sec, &dt_hardware);

	/* No interrupts while we're in the chip. */
	s = splhigh();

	/* 
	 * Issue a GO command to reset everything less significant
	 * than the minutes to zero.
	 */
	mm58167_write(sc, mm58167_go, 0xFF);
	
	/* Load everything. */
#define _MM58167_PUT(dt_f, mm_f) byte_value = TOBCD(dt_hardware.dt_f); mm58167_write(sc, mm_f, byte_value)
	_MM58167_PUT(dt_mon, mm58167_mon);
	_MM58167_PUT(dt_day, mm58167_day);
	_MM58167_PUT(dt_hour, mm58167_hour);
	_MM58167_PUT(dt_min, mm58167_min);
	_MM58167_PUT(dt_sec, mm58167_sec);
#undef _MM58167_PUT

	splx(s);
	return (0);
}

int
mm58167_getcal(handle, vp)
	todr_chip_handle_t handle;
	int *vp;
{
	return (EOPNOTSUPP);
}

int
mm58167_setcal(handle, v)
	todr_chip_handle_t handle;
	int v;
{
	return (EOPNOTSUPP);
}
