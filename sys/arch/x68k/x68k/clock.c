/*	$NetBSD: clock.c,v 1.8 2000/01/19 02:52:21 msaitoh Exp $	*/

/*
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */

#include "clock.h"

#if NCLOCK > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <arch/x68k/dev/mfp.h>
#include <arch/x68k/dev/rtclock_var.h>


struct clock_softc {
	struct device		sc_dev;
};

static int clock_match __P((struct device *, struct cfdata *, void *));
static void clock_attach __P((struct device *, struct device *, void *));

struct cfattach clock_ca = {
	sizeof(struct clock_softc), clock_match, clock_attach
};


static int
clock_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	if (strcmp (aux, "clock") != 0)
		return (0);
	if (cf->cf_unit != 0)
		return (0);
	return 1;
}


static void
clock_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	printf (": MFP timer C\n");

	return;
}


/* We're using a 100 Hz clock. */

#define CLK_INTERVAL 200
#define CLOCKS_PER_SEC 100

static int clkread __P((void));

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
 *
 * Inittodr initializes the time of day hardware which provides
 * date functions.
 *
 * Resettodr restores the time of day hardware after a time change.
 *
 * A note on the real-time clock:
 * We actually load the clock with CLK_INTERVAL-1 instead of CLK_INTERVAL.
 * This is because the counter decrements to zero after N+1 enabled clock
 * periods where N is the value loaded into the counter.
 */

/*
 * Set up the real-time and statistics clocks.  Leave stathz 0 only if
 * no alternative timer is available.
 *
 */
void
cpu_initclocks()
{
	mfp_set_tcdcr(mfp_get_tcdcr() & 0x0f); /* stop timer C */
	mfp_set_tcdr(CLK_INTERVAL);

	mfp_set_tcdcr(mfp_get_tcdcr() | 0x70); /* 1/200 delay mode */
	mfp_bit_set_ierb(MFP_INTR_TIMER_C);
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(hz)
	int hz;
{
}

/*
 * Returns number of usec since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
int
clkread()
{
	return (mfp_get_tcdr() * CLOCKS_PER_SEC) / CLK_INTERVAL;
}


#if 0
void
DELAY(mic)
	int mic;
{
	u_long n;
	short hpos;

	/*
	 * busy-poll for mic microseconds. This is *no* general timeout function,
	 * it's meant for timing in hardware control, and as such, may not lower
	 * interrupt priorities to really `sleep'.
	 */

	/*
	 * this function uses HSync pulses as base units. The custom chips 
	 * display only deals with 31.6kHz/2 refresh, this gives us a
	 * resolution of 1/15800 s, which is ~63us (add some fuzz so we really
	 * wait awhile, even if using small timeouts)
	 */
	n = mic/32 + 2;
	do {
		while ((mfp.gpip & MFP_GPIP_HSYNC) != 0)
			asm("nop");
		while ((mfp.gpip & MFP_GPIP_HSYNC) == 0)
			asm("nop");
	} while (n--);
}
#endif


#if notyet

/* implement this later. I'd suggest using both timers in CIA-A, they're
   not yet used. */

/*
 * /dev/clock: mappable high resolution timer.
 *
 * This code implements a 32-bit recycling counter (with a 4 usec period)
 * using timers 2 & 3 on the 6840 clock chip.  The counter can be mapped
 * RO into a user's address space to achieve low overhead (no system calls),
 * high-precision timing.
 *
 * Note that timer 3 is also used for the high precision profiling timer
 * (PROFTIMER code above).  Care should be taken when both uses are
 * configured as only a token effort is made to avoid conflicting use.
 */
#include <sys/proc.h>
#include <sys/resourcevar.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <vm/vm.h>
#include <x68k/x68k/clockioctl.h>
#include <sys/specdev.h>
#include <sys/vnode.h>
#include <sys/mman.h>

int clockon = 0;		/* non-zero if high-res timer enabled */
#ifdef PROFTIMER
int  profprocs = 0;		/* # of procs using profiling timer */
#endif
#ifdef DEBUG
int clockdebug = 0;
#endif

/*ARGSUSED*/
clockopen(dev, flags)
	dev_t dev;
{
#ifdef PROFTIMER
#ifdef PROF
	/*
	 * Kernel profiling enabled, give up.
	 */
	if (profiling)
		return(EBUSY);
#endif	/* PROF */
	/*
	 * If any user processes are profiling, give up.
	 */
	if (profprocs)
		return(EBUSY);
#endif	/* PROFTIMER */
	if (!clockon) {
		startclock();
		clockon++;
	}
	return(0);
}

/*ARGSUSED*/
clockclose(dev, flags)
	dev_t dev;
{
	(void) clockunmmap(dev, (caddr_t)0, curproc);	/* XXX */
	stopclock();
	clockon = 0;
	return(0);
}

/*ARGSUSED*/
clockioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	int error = 0;
	
	switch (cmd) {

	case CLOCKMAP:
		error = clockmmap(dev, (caddr_t *)data, p);
		break;

	case CLOCKUNMAP:
		error = clockunmmap(dev, *(caddr_t *)data, p);
		break;

	case CLOCKGETRES:
		*(int *)data = CLK_RESOLUTION;
		break;

	default:
		error = EINVAL;
		break;
	}
	return(error);
}

/*ARGSUSED*/
clockmap(dev, off, prot)
	dev_t dev;
{
	return((off + (INTIOBASE+CLKBASE+CLKSR-1)) >> PGSHIFT);
}

clockmmap(dev, addrp, p)
	dev_t dev;
	caddr_t *addrp;
	struct proc *p;
{
	int error;
	struct vnode vn;
	struct specinfo si;
	int flags;

	flags = MAP_FILE|MAP_SHARED;
	if (*addrp)
		flags |= MAP_FIXED;
	else
		*addrp = (caddr_t)0x1000000;	/* XXX */
	vn.v_type = VCHR;			/* XXX */
	vn.v_specinfo = &si;			/* XXX */
	vn.v_rdev = dev;			/* XXX */
	error = vm_mmap(&p->p_vmspace->vm_map, (vaddr_t *)addrp,
			PAGE_SIZE, VM_PROT_ALL, flags, (caddr_t)&vn, 0);
	return(error);
}

clockunmmap(dev, addr, p)
	dev_t dev;
	caddr_t addr;
	struct proc *p;
{
	int rv;

	if (addr == 0)
		return(EINVAL);		/* XXX: how do we deal with this? */
	rv = vm_deallocate(p->p_vmspace->vm_map, (vaddr_t)addr, PAGE_SIZE);
	return(rv == KERN_SUCCESS ? 0 : EINVAL);
}

startclock()
{
	register struct clkreg *clk = (struct clkreg *)clkstd[0];

	clk->clk_msb2 = -1; clk->clk_lsb2 = -1;
	clk->clk_msb3 = -1; clk->clk_lsb3 = -1;

	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = CLK_OENAB|CLK_8BIT;
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_IENAB;
}

stopclock()
{
	register struct clkreg *clk = (struct clkreg *)clkstd[0];

	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = 0;
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_IENAB;
}

#endif	/* notyet */


#ifdef PROFTIMER
/*
 * This code allows the amiga kernel to use one of the extra timers on
 * the clock chip for profiling, instead of the regular system timer.
 * The advantage of this is that the profiling timer can be turned up to
 * a higher interrupt rate, giving finer resolution timing. The profclock
 * routine is called from the lev6intr in locore, and is a specialized
 * routine that calls addupc. The overhead then is far less than if
 * hardclock/softclock was called. Further, the context switch code in
 * locore has been changed to turn the profile clock on/off when switching
 * into/out of a process that is profiling (startprofclock/stopprofclock).
 * This reduces the impact of the profiling clock on other users, and might
 * possibly increase the accuracy of the profiling. 
 */
int  profint   = PRF_INTERVAL;	/* Clock ticks between interrupts */
int  profscale = 0;		/* Scale factor from sys clock to prof clock */
char profon    = 0;		/* Is profiling clock on? */

/* profon values - do not change, locore.s assumes these values */
#define PRF_NONE	0x00
#define	PRF_USER	0x01
#define	PRF_KERNEL	0x80

initprofclock()
{
	struct proc *p = curproc;		/* XXX */

	/*
	 * If the high-res timer is running, force profiling off.
	 * Unfortunately, this gets reflected back to the user not as
	 * an error but as a lack of results.
	 */
	if (clockon) {
		p->p_stats->p_prof.pr_scale = 0;
		return;
	}
	/*
	 * Keep track of the number of user processes that are profiling
	 * by checking the scale value.
	 *
	 * XXX: this all assumes that the profiling code is well behaved;
	 * i.e. profil() is called once per process with pcscale non-zero
	 * to turn it on, and once with pcscale zero to turn it off.
	 * Also assumes you don't do any forks or execs.  Oh well, there
	 * is always adb...
	 */
	if (p->p_stats->p_prof.pr_scale)
		profprocs++;
	else
		profprocs--;
	/*
	 * The profile interrupt interval must be an even divisor
	 * of the CLK_INTERVAL so that scaling from a system clock
	 * tick to a profile clock tick is possible using integer math.
	 */
	if (profint > CLK_INTERVAL || (CLK_INTERVAL % profint) != 0)
		profint = CLK_INTERVAL;
	profscale = CLK_INTERVAL / profint;
}

startprofclock()
{
}

stopprofclock()
{
}

#ifdef PROF
/*
 * profclock() is expanded in line in lev6intr() unless profiling kernel.
 * Assumes it is called with clock interrupts blocked.
 */
profclock(pc, ps)
	caddr_t pc;
	int ps;
{
	/*
	 * Came from user mode.
	 * If this process is being profiled record the tick.
	 */
	if (USERMODE(ps)) {
		if (p->p_stats.p_prof.pr_scale)
			addupc(pc, &curproc->p_stats.p_prof, 1);
	}
	/*
	 * Came from kernel (supervisor) mode.
	 * If we are profiling the kernel, record the tick.
	 */
	else if (profiling < 2) {
		register int s = pc - s_lowpc;

		if (s < s_textsize)
			kcount[s / (HISTFRACTION * sizeof (*kcount))]++;
	}
	/*
	 * Kernel profiling was on but has been disabled.
	 * Mark as no longer profiling kernel and if all profiling done,
	 * disable the clock.
	 */
	if (profiling && (profon & PRF_KERNEL)) {
		profon &= ~PRF_KERNEL;
		if (profon == PRF_NONE)
			stopprofclock();
	}
}
#endif	/* PROF */
#endif	/* PROFTIMER */

/*
 * Return the best possible estimate of the current time.
 */
void
microtime(tvp)
	register struct timeval *tvp;
{
	static struct timeval lasttime;

	*tvp = time;
	tvp->tv_usec += clkread();
	while (tvp->tv_usec >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	if (tvp->tv_sec == lasttime.tv_sec &&
	    tvp->tv_usec <= lasttime.tv_usec &&
	    (tvp->tv_usec = lasttime.tv_usec + 1) >= 1000000) {
		tvp->tv_sec++;
		tvp->tv_usec -= 1000000;
	}
	lasttime = *tvp;
}

/* this is a hook set by a clock driver for the configured realtime clock,
   returning plain current unix-time */
long (*gettod) __P((void)) = 0;
long (*settod) __P((long)) = 0;

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
	time_t base;
{
	u_long timbuf = base;	/* assume no battery clock exists */
  
	if (!gettod)
		printf ("WARNING: no battery clock\n");
	else
		timbuf = gettod();
  
	if (timbuf < base) {
		printf ("WARNING: bad date in battery clock\n");
		timbuf = base;
	}
	if (base < 5*SECYR) {
		printf("WARNING: preposterous time in file system");
		timbuf = 6*SECYR + 186*SECDAY + SECDAY/2;
		printf(" -- CHECK AND RESET THE DATE!\n");
	}

	/* Battery clock does not store usec's, so forget about it. */
	time.tv_sec = timbuf;
}

void
resettodr()
{
	if (settod)
		if (settod (time.tv_sec) != 1)
			printf("Cannot set battery backed clock\n");
}
#else	/* NCLOCK */
#error loose.
#endif
