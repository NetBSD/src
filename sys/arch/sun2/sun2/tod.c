/*	$NetBSD: tod.c,v 1.1 2001/04/06 15:05:57 fredette Exp $	*/

/*
 * Copyright (c) 2001 Matthew Fredette
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: Utah Hdr: clock.c 1.18 91/01/21$
 *	from: @(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * Machine-dependent clock routines for the NS mm58167 time-of-day chip.
 * Written by Matthew Fredette, based on the sun3 clock.c by 
 * Adam Glass and Gordon Ross.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <m68k/asm_single.h>

#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/cpu.h>

#include <sun2/sun2/machdep.h>
#include <sun2/sun2/tod.h>

#include <dev/clock_subr.h>
#include <dev/ic/mm58167var.h>

static todr_chip_handle_t todr_handle;

static int  tod_match __P((struct device *, struct cfdata *, void *args));
static void tod_attach __P((struct device *, struct device *, void *));

struct cfattach tod_ca = {
	sizeof(struct mm58167_softc), tod_match, tod_attach
};

static int
tod_match(parent, cf, args)
    struct device *parent;
	struct cfdata *cf;
    void *args;
{
	struct confargs *ca = args;

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return (0);

	/* Make sure there is something there... */
	if (!bus_space_probe(ca->ca_bustag, 0, ca->ca_paddr,
				1,	/* probe size */
				0,	/* offset */
				0,	/* flags */
				NULL, NULL))
		return (0);

	return (1);
}

static void
tod_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct confargs *ca = args;
	struct mm58167_softc *sc;

	sc = (struct mm58167_softc *) self;

	/* Map the device. */
	sc->mm58167_regt = ca->ca_bustag;
	if (bus_space_map(ca->ca_bustag, ca->ca_paddr, sizeof(struct mm58167regs), 0, &sc->mm58167_regh))
		panic("tod_attach: can't map");

	/* Call the IC attach code. */
	sc->mm58167_msec_xxx = offsetof(struct mm58167regs, mm58167_msec_xxx);
	sc->mm58167_csec = offsetof(struct mm58167regs, mm58167_csec);
	sc->mm58167_sec = offsetof(struct mm58167regs, mm58167_sec);
	sc->mm58167_min = offsetof(struct mm58167regs, mm58167_min);
	sc->mm58167_hour = offsetof(struct mm58167regs, mm58167_hour);
	sc->mm58167_wday = offsetof(struct mm58167regs, mm58167_wday);
	sc->mm58167_day = offsetof(struct mm58167regs, mm58167_day);
	sc->mm58167_mon = offsetof(struct mm58167regs, mm58167_mon);
	sc->mm58167_status = offsetof(struct mm58167regs, mm58167_status);
	sc->mm58167_go = offsetof(struct mm58167regs, mm58167_go);
	if ((todr_handle = mm58167_attach(sc)) == NULL)
		panic("tod_attach: can't attach ic");

	printf("\n");
}

/*
 * Machine-dependent clock routines.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 */

/*
 * Initialize the time of day register, based on the time base
 * which is, e.g. from a filesystem.
 */
void inittodr(fs_time)
	time_t fs_time;
{
	struct timeval tv;
	time_t diff, clk_time;
	time_t long_ago = (5 * SECYR);
	int clk_bad = 0;

	/*
	 * Sanity check time from file system.
	 * If it is zero,assume filesystem time is just unknown
	 * instead of preposterous.  Don't bark.
	 */
	if (fs_time < long_ago) {
		/*
		 * If fs_time is zero, assume filesystem time is just
		 * unknown instead of preposterous.  Don't bark.
		 */
		if (fs_time != 0)
			printf("WARNING: preposterous time in file system\n");
		/* 1991/07/01  12:00:00 */
		fs_time = 21*SECYR + 186*SECDAY + SECDAY/2;
	}

	todr_gettime(todr_handle, &tv);
	clk_time = tv.tv_sec;

	/* Sanity check time from clock. */
	if (clk_time < long_ago) {
		printf("WARNING: bad date in battery clock");
		clk_bad = 1;
		clk_time = fs_time;
	} else {
		/* Does the clock time jive with the file system? */
		diff = clk_time - fs_time;
		if (diff < 0)
			diff = -diff;
		if (diff >= (SECDAY*2)) {
			printf("WARNING: clock %s %d days",
				   (clk_time < fs_time) ? "lost" : "gained",
				   (int) (diff / SECDAY));
			clk_bad = 1;
		}
	}
	if (clk_bad)
		printf(" -- CHECK AND RESET THE DATE!\n");
	time.tv_sec = clk_time;
}

/*
 * Resettodr restores the time of day hardware after a time change.
 */
void resettodr()
{
	struct timeval tv;
	tv = time;
	todr_settime(todr_handle, &tv);
}
