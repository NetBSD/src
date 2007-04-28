/*	$NetBSD: clock.c,v 1.98.8.2 2007/04/28 03:55:24 mrg Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1996 Paul Kranenburg
 * Copyright (c) 1996
 * 	The President and Fellows of Harvard College. All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by Harvard University.
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 *	This product includes software developed by Paul Kranenburg.
 *	This product includes software developed by Harvard University.
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
 *	@(#)clock.c	8.1 (Berkeley) 6/11/93
 *
 */

/*
 * Common timer, clock and eeprom routines.
 *
 * Overview of timer and time-of-day devices on sparc machines:
 *
 * sun4/100 & sun4/200
 *	have the Intersil 7170 time-of-day and timer chip (oclock.c)
 *	eeprom device in OBIO space (eeprom.c)
 *
 * sun4/300 & sun4/400
 *	Mostek MK48T02 clock/nvram device, includes eeprom (mkclock.c)
 *	2 system timers (timer.c)
 *
 * sun4c
 *	Mostek MK48T02 or MK48T08 clock/nvram device (mkclock.c)
 *	system timer in OBIO space
 *	2 system timers (timer.c)
 *
 * sun4m
 *	Mostek MK48T08 clock/nvram device (mkclock.c)
 *	1 global system timer (timer.c)
 *	1 configurable counter/timer per CPU (timer.c)
 *
 * microSPARC-IIep:
 *	DS1287A time-of-day chip on EBUS (dev/rtc.c)
 *	the system timer is part of the PCI controller (timer.c)
 *
 * All system use the timer interrupt (at IPL 10) to drive hardclock().
 * The second or per-CPU timer interrupt (at IPL 14) is used to drive
 * statclock() (except on sun4/100 and sun4/200 machines, which don't
 * have a spare timer device).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.98.8.2 2007/04/28 03:55:24 mrg Exp $");

#include "opt_sparc_arch.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/timetc.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/eeprom.h>
#include <machine/cpu.h>

#include <dev/clock_subr.h>

/* Variables shared with timer.c, mkclock.c, oclock.c */
int timerblurb = 10;	/* Guess a value; used before clock is attached */
int oldclk = 0;

void	(*timer_init)(void);	/* Called from cpu_initclocks() */
int	(*eeprom_nvram_wenable)(int);

/*
 * Statistics clock interval and variance, in usec.  Variance must be a
 * power of two.  Since this gives us an even number, not an odd number,
 * we discard one case and compensate.  That is, a variance of 1024 would
 * give us offsets in [0..1023].  Instead, we take offsets in [1..1023].
 * This is symmetric about the point 512, or statvar/2, and thus averages
 * to that value (assuming uniform random numbers).
 */
/* XXX fix comment to match value */
int statvar = 8192;
int statmin;			/* statclock interval - 1/2*variance */
int statint;


/*
 * Common eeprom I/O routines.
 */
char		*eeprom_va = NULL;
#if defined(SUN4)
static int	eeprom_busy = 0;
static int	eeprom_wanted = 0;
static int	eeprom_take(void);
static void	eeprom_give(void);
static int	eeprom_update(char *, int, int);
#endif

/*
 * Set up the real-time and statistics clocks.
 * Leave stathz 0 only if no alternative timer is available.
 *
 * The frequencies of these clocks must be an even number of microseconds.
 */
void
cpu_initclocks(void)
{
	int minint;

	if (1000000 % hz) {
		printf("cannot get %d Hz clock; using 100 Hz\n", hz);
		hz = 100;
		tick = 1000000 / hz;
	}
	if (stathz == 0)
		stathz = hz;
	if (1000000 % stathz) {
		printf("cannot get %d Hz statclock; using 100 Hz\n", stathz);
		stathz = 100;
	}
	profhz = stathz;		/* always */

	statint = 1000000 / stathz;
	minint = statint / 2 + 100;
	while (statvar > minint)
		statvar >>= 1;
	statmin = statint - (statvar >> 1);

	if (timer_init != NULL)
		(*timer_init)();

	/*
	 * The scheduler clock runs every 8 statclock ticks,
	 * assuming stathz == 100. If it's not, compute a mask
	 * for use in the various statintr() functions approx.
	 * like this:
	 *	mask = round_power2(stathz / schedhz) - 1
	 */
	schedhz = 12;
}

/*
 * Dummy setstatclockrate(), since we know profhz==hz.
 */
/* ARGSUSED */
void
setstatclockrate(int newhz)
{
	/* nothing */
}


/*
 * Scheduler pseudo-clock interrupt handler.
 * Runs off a soft interrupt at IPL_SCHED, scheduled by statintr().
 */
void
schedintr(void *v)
{

	schedclock(curlwp);
}

/*
 * XXX: these may actually belong somewhere else, but since the
 * EEPROM is so closely tied to the clock on some models, perhaps
 * it needs to stay here...
 */
int
eeprom_uio(struct uio *uio)
{
#if defined(SUN4)
	int error;
	int off;	/* NOT off_t */
	u_int cnt, bcnt;
	char *buf = NULL;

	if (!CPU_ISSUN4)
		return (ENODEV);

	if (eeprom_va == NULL) {
		error = ENXIO;
		goto out;
	}

	off = uio->uio_offset;
	if (off > EEPROM_SIZE)
		return (EFAULT);

	cnt = uio->uio_resid;
	if (cnt > (EEPROM_SIZE - off))
		cnt = (EEPROM_SIZE - off);

	if ((error = eeprom_take()) != 0)
		return (error);

	/*
	 * The EEPROM can only be accessed one byte at a time, yet
	 * uiomove() will attempt long-word access.  To circumvent
	 * this, we byte-by-byte copy the eeprom contents into a
	 * temporary buffer.
	 */
	buf = malloc(EEPROM_SIZE, M_DEVBUF, M_WAITOK);
	if (buf == NULL) {
		error = EAGAIN;
		goto out;
	}

	if (uio->uio_rw == UIO_READ)
		for (bcnt = 0; bcnt < EEPROM_SIZE; ++bcnt)
			buf[bcnt] = eeprom_va[bcnt];

	if ((error = uiomove(buf + off, (int)cnt, uio)) != 0)
		goto out;

	if (uio->uio_rw != UIO_READ)
		error = eeprom_update(buf, off, cnt);

 out:
	if (buf)
		free(buf, M_DEVBUF);
	eeprom_give();
	return (error);
#else /* ! SUN4 */
	return (ENODEV);
#endif /* SUN4 */
}

#if defined(SUN4)
/*
 * Update the EEPROM from the passed buf.
 */
static int
eeprom_update(char *buf, int off, int cnt)
{
	int error = 0;
	volatile char *ep;
	char *bp;

	if (eeprom_va == NULL)
		return (ENXIO);

	ep = eeprom_va + off;
	bp = buf + off;

	if (eeprom_nvram_wenable != NULL)
		(*eeprom_nvram_wenable)(1);

	while (cnt > 0) {
		/*
		 * DO NOT WRITE IT UNLESS WE HAVE TO because the
		 * EEPROM has a limited number of write cycles.
		 * After some number of writes it just fails!
		 */
		if (*ep != *bp) {
			*ep = *bp;
			/*
			 * We have written the EEPROM, so now we must
			 * sleep for at least 10 milliseconds while
			 * holding the lock to prevent all access to
			 * the EEPROM while it recovers.
			 */
			(void)tsleep(eeprom_va, PZERO - 1, "eeprom", hz/50);
		}
		/* Make sure the write worked. */
		if (*ep != *bp) {
			error = EIO;
			goto out;
		}
		++ep;
		++bp;
		--cnt;
	}
 out:
	if (eeprom_nvram_wenable != NULL)
		(*eeprom_nvram_wenable)(0);

	return (error);
}

/* Take a lock on the eeprom. */
static int
eeprom_take(void)
{
	int error = 0;

	while (eeprom_busy) {
		eeprom_wanted = 1;
		error = tsleep(&eeprom_busy, PZERO | PCATCH, "eeprom", 0);
		eeprom_wanted = 0;
		if (error)	/* interrupted */
			goto out;
	}
	eeprom_busy = 1;
 out:
	return (error);
}

/* Give a lock on the eeprom away. */
static void
eeprom_give(void)
{

	eeprom_busy = 0;
	if (eeprom_wanted) {
		eeprom_wanted = 0;
		wakeup(&eeprom_busy);
	}
}
#endif /* SUN4 */
