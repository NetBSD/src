/*	$NetBSD: intersil7170.c,v 1.1 2000/07/25 22:33:02 pk Exp $ */
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * Intersil 7170 time-of-day chip subroutines.
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/errno.h>

#include <machine/bus.h>
#include <dev/clock_subr.h>
#include <dev/ic/intersil7170.h>

#define intersil_command(run, interrupt) \
	(run | interrupt | INTERSIL_CMD_FREQ_32K | INTERSIL_CMD_24HR_MODE | \
	 INTERSIL_CMD_NORMAL_MODE)

struct intersil7170_softc {
	bus_space_tag_t		sil_bt;
	bus_space_handle_t	sil_bh;
	int			sil_year0;
};

int intersil7170_gettime(todr_chip_handle_t, struct timeval *);
int intersil7170_settime(todr_chip_handle_t, struct timeval *);
int intersil7170_getcal(todr_chip_handle_t, int *);
int intersil7170_setcal(todr_chip_handle_t, int);

int intersil7170_auto_century_adjust = 1;

todr_chip_handle_t
intersil7170_attach(bt, bh, year0)
	bus_space_tag_t bt;
	bus_space_handle_t bh;
	int year0;
{
	todr_chip_handle_t handle;
	struct intersil7170_softc *sil;
	int sz;

	printf(": intersil7170");

	sz = ALIGN(sizeof(struct todr_chip_handle)) + sizeof(struct intersil7170);
	handle = malloc(sz, M_DEVBUF, M_NOWAIT);
	sil = (struct intersil7170_softc *)((u_long)handle +
					ALIGN(sizeof(struct todr_chip_handle)));
	handle->cookie = sil;
	handle->todr_gettime = intersil7170_gettime;
	handle->todr_settime = intersil7170_settime;
	handle->todr_getcal = intersil7170_getcal;
	handle->todr_setcal = intersil7170_setcal;
	sil->sil_bt = bt;
	sil->sil_bh = bh;
	sil->sil_year0 = year0;

	return (handle);
}

/*
 * Set up the system's time, given a `reasonable' time value.
 */
int
intersil7170_gettime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct intersil7170_softc *sil = handle->cookie;
	bus_space_tag_t bt = sil->sil_bt;
	bus_space_handle_t bh = sil->sil_bh;
	struct clock_ymdhms dt;
	u_int8_t cmd;
	int year;
	int s;

	/* No interrupts while we're fiddling with the chip */
	s = splhigh();

	/* Enable read (stop time) */
	cmd = intersil_command(INTERSIL_CMD_STOP, INTERSIL_CMD_IENABLE);
	bus_space_write_1(bt, bh, INTERSIL_ICMD, cmd);

	/* The order of reading out the clock elements is important */
	bus_space_read_1(bt, bh, INTERSIL_ICSEC);	/* not used */
	dt.dt_hour = bus_space_read_1(bt, bh, INTERSIL_IHOUR);
	dt.dt_min = bus_space_read_1(bt, bh, INTERSIL_IMIN);
	dt.dt_sec = bus_space_read_1(bt, bh, INTERSIL_ISEC);
	dt.dt_mon = bus_space_read_1(bt, bh, INTERSIL_IMON);
	dt.dt_day = bus_space_read_1(bt, bh, INTERSIL_IDAY);
	year = bus_space_read_1(bt, bh, INTERSIL_IYEAR);
	dt.dt_wday = bus_space_read_1(bt, bh, INTERSIL_IDOW);

	/* Done writing (time wears on) */
	cmd = intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);
	bus_space_write_1(bt, bh, INTERSIL_ICMD, cmd);
	splx(s);

	year += sil->sil_year0;
	if (year < 1970 && intersil7170_auto_century_adjust != 0)
		year += 100;

	dt.dt_year = year;

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return (0);
}

/*
 * Reset the clock based on the current time.
 */
int
intersil7170_settime(handle, tv)
	todr_chip_handle_t handle;
	struct timeval *tv;
{
	struct intersil7170_softc *sil = handle->cookie;
	bus_space_tag_t bt = sil->sil_bt;
	bus_space_handle_t bh = sil->sil_bh;
	struct clock_ymdhms dt;
	u_int8_t cmd;
	int year;
	int s;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	year = dt.dt_year - sil->sil_year0;
	if (year > 99 && intersil7170_auto_century_adjust != 0)
		year -= 100;

	/* No interrupts while we're fiddling with the chip */
	s = splhigh();

	/* Enable write (stop time) */
	cmd = intersil_command(INTERSIL_CMD_STOP, INTERSIL_CMD_IENABLE);
	bus_space_write_1(bt, bh, INTERSIL_ICMD, cmd);

	/* The order of reading writing the clock elements is important */
	bus_space_write_1(bt, bh, INTERSIL_ICSEC, 0);
	bus_space_write_1(bt, bh, INTERSIL_IHOUR, dt.dt_hour);
	bus_space_write_1(bt, bh, INTERSIL_IMIN, dt.dt_min);
	bus_space_write_1(bt, bh, INTERSIL_ISEC, dt.dt_sec);
	bus_space_write_1(bt, bh, INTERSIL_IMON, dt.dt_mon);
	bus_space_write_1(bt, bh, INTERSIL_IDAY, dt.dt_day);
	bus_space_write_1(bt, bh, INTERSIL_IYEAR, year);
	bus_space_write_1(bt, bh, INTERSIL_IDOW, dt.dt_wday);

	/* Done writing (time wears on) */
	cmd = intersil_command(INTERSIL_CMD_RUN, INTERSIL_CMD_IENABLE);
	bus_space_write_1(bt, bh, INTERSIL_ICMD, cmd);
	splx(s);

	return (0);
}

int
intersil7170_getcal(handle, vp)
	todr_chip_handle_t handle;
	int *vp;
{
	return (EOPNOTSUPP);
}

int
intersil7170_setcal(handle, v)
	todr_chip_handle_t handle;
	int v;
{
	return (EOPNOTSUPP);
}
