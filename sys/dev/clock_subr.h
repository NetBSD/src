/*	$NetBSD: clock_subr.h,v 1.21 2009/12/12 15:10:34 tsutsui Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross
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

#ifndef _DEV_CLOCK_SUBR_H_
#define _DEV_CLOCK_SUBR_H_

/*
 * "POSIX time" to/from "YY/MM/DD/hh/mm/ss"
 */
struct clock_ymdhms {
	u_short dt_year;
	u_char dt_mon;
	u_char dt_day;
	u_char dt_wday;	/* Day of week */
	u_char dt_hour;
	u_char dt_min;
	u_char dt_sec;
};

time_t	clock_ymdhms_to_secs(struct clock_ymdhms *);
void	clock_secs_to_ymdhms(time_t, struct clock_ymdhms *);

/*
 * BCD to binary and binary to BCD.
 */
#define	FROMBCD(x)	bcdtobin((x))
#define	TOBCD(x)	bintobcd((x))

/* Some handy constants. */
#define SECDAY		(24 * 60 * 60)
#define SECYR		(SECDAY * 365)

/* Traditional POSIX base year */
#define	POSIX_BASE_YEAR	1970

/*
 * Interface to time-of-day clock devices.
 *
 * todr_gettime: convert time-of-day clock into a `struct timeval'
 * todr_settime: set time-of-day clock from a `struct timeval'
 *
 * (this is probably not so useful:)
 * todr_setwen: provide a machine-dependent TOD clock write-enable callback
 *		function which takes one boolean argument:
 *			1 to enable writes; 0 to disable writes.
 */
struct todr_chip_handle {
	void	*cookie;	/* Device specific data */
	void	*bus_cookie;	/* Bus specific data */
	time_t	base_time;	/* Base time (e.g. rootfs time) */

	int	(*todr_gettime)(struct todr_chip_handle *, struct timeval *);
	int	(*todr_settime)(struct todr_chip_handle *, struct timeval *);
	int	(*todr_gettime_ymdhms)(struct todr_chip_handle *,
	    			struct clock_ymdhms *);
	int	(*todr_settime_ymdhms)(struct todr_chip_handle *,
	    			struct clock_ymdhms *);
	int	(*todr_setwen)(struct todr_chip_handle *, int);

};
typedef struct todr_chip_handle *todr_chip_handle_t;

#define todr_wenable(ct, v)	if ((ct)->todr_setwen) \
					((*(ct)->todr_setwen)(ct, v))

/*
 * Probably these should evolve into internal routines in kern_todr.c.
 */
extern int todr_gettime(todr_chip_handle_t tch, struct timeval *);
extern int todr_settime(todr_chip_handle_t tch, struct timeval *);

/*
 * Machine-dependent function that machine-independent RTC drivers can
 * use to register their todr_chip_handle_t with inittodr()/resettodr().
 */
void	todr_attach(todr_chip_handle_t);

#endif /* _DEV_CLOCK_SUBR_H_ */
