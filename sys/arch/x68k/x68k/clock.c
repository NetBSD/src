/*	$NetBSD: clock.c,v 1.25.4.1 2007/07/11 20:03:08 mjf Exp $	*/

/*
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
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */
/*
 * Copyright (c) 1988 University of Utah.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.25.4.1 2007/07/11 20:03:08 mjf Exp $");

#include "clock.h"

#if NCLOCK > 0

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/timetc.h>

#include <machine/psl.h>
#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/clock_subr.h>

#include <arch/x68k/dev/mfp.h>
#include <arch/x68k/dev/rtclock_var.h>


struct clock_softc {
	struct device		sc_dev;
};

static int clock_match(struct device *, struct cfdata *, void *);
static void clock_attach(struct device *, struct device *, void *);

CFATTACH_DECL(clock, sizeof(struct clock_softc),
    clock_match, clock_attach, NULL, NULL);

static int clock_attached;

static unsigned mfp_get_timecount(struct timecounter *);

static int
clock_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (strcmp (aux, "clock") != 0)
		return (0);
	if (clock_attached)
		return (0);
	return 1;
}


static void
clock_attach(struct device *parent, struct device *self, void *aux)
{

	clock_attached = 1;

	printf(": MFP timer C\n");
}


/*
 * MFP of X68k uses 4MHz clock always and we use 1/200 prescaler here.
 * Therefore, clock interval is 50 usec.
 *
 * Note that for timecounters, we'd like to use a finger grained clock, but
 * since we only have an 8-bit clock, we can't do that without increasing
 * the system clock rate.  (Otherwise the counter would roll in less than
 * a single system clock.)
 */
#define CLK_RESOLUTION	(50)
#define CLOCKS_PER_SEC	(1000000 / CLK_RESOLUTION)

/*
 * Machine-dependent clock routines.
 *
 * Startrtclock restarts the real-time clock, which provides
 * hardclock interrupts to kern_clock.c.
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
cpu_initclocks(void)
{
	static struct	timecounter tc = {
		.tc_name = "mfp",
		.tc_frequency = CLOCKS_PER_SEC,
		.tc_counter_mask = 0xff,
		.tc_get_timecount = mfp_get_timecount,
		.tc_quality = 100,
	};
	
	if (CLOCKS_PER_SEC % hz ||
	    hz <= (CLOCKS_PER_SEC / 256) || hz > CLOCKS_PER_SEC) {
		printf("cannot set %d Hz clock. using 100 Hz\n", hz);
		hz = 100;
	}

	mfp_set_tcdcr(0);		/* stop timers C and D */
	mfp_set_tcdcr(mfp_get_tcdcr() | 0x70); /* 1/200 delay mode */

	mfp_set_tcdr(CLOCKS_PER_SEC / hz);
	mfp_bit_set_ierb(MFP_INTR_TIMER_C);

	mfp_set_tddr(0xff);	/* maximum free run -- only 8 bits wide */
	mfp_set_tcdcr(mfp_get_tcdcr() | 0x07);	/* 1/200 prescaler */

	tc_init(&tc);
}

/*
 * We assume newhz is either stathz or profhz, and that neither will
 * change after being set up above.  Could recalculate intervals here
 * but that would be a drag.
 */
void
setstatclockrate(int newhz)
{
}

/*
 * Returns number of usec since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
unsigned
mfp_get_timecount(struct timecounter *tc)
{
	uint8_t	val;
	val = ~(mfp_get_tddr());
	return (val);
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
			__asm("nop");
		while ((mfp.gpip & MFP_GPIP_HSYNC) == 0)
			__asm("nop");
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
#include <uvm/uvm_extern.h>	/* XXX needed? */
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
int
clockopen(dev_t dev, int flags)
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
int
clockclose(dev_t dev, int flags)
{
	(void) clockunmmap(dev, NULL, curproc);	/* XXX */
	stopclock();
	clockon = 0;
	return(0);
}

/*ARGSUSED*/
int
clockioctl(dev_t dev, u_long cmd, void *data, int flag, struct proc *p)
{
	int error = 0;
	
	switch (cmd) {

	case CLOCKMAP:
		error = clockmmap(dev, (void **)data, p);
		break;

	case CLOCKUNMAP:
		error = clockunmmap(dev, *(void **)data, p);
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
int
clockmap(dev_t dev, off_t off, int prot)
{
	return ((off + (INTIOBASE + CLKBASE + CLKSR - 1)) >> PGSHIFT);
}

int
clockmmap(dev_t dev, void **addrp, struct proc *p)
{
	int error;
	struct vnode vn;
	struct specinfo si;
	int flags;

	flags = MAP_FILE|MAP_SHARED;
	if (*addrp)
		flags |= MAP_FIXED;
	else
		*addrp = (void *)0x1000000;	/* XXX */
	vn.v_type = VCHR;			/* XXX */
	vn.v_specinfo = &si;			/* XXX */
	vn.v_rdev = dev;			/* XXX */
	error = vm_mmap(&p->p_vmspace->vm_map, (vaddr_t *)addrp,
			PAGE_SIZE, VM_PROT_ALL, flags, (void *)&vn, 0);
	return(error);
}

int
clockunmmap(dev_t dev, void *addr, struct proc *p)
{
	int rv;

	if (addr == 0)
		return(EINVAL);		/* XXX: how do we deal with this? */
	uvm_deallocate(p->p_vmspace->vm_map, (vaddr_t)addr, PAGE_SIZE);
	return 0;
}

void
startclock(void)
{
	struct clkreg *clk = (struct clkreg *)clkstd[0];

	clk->clk_msb2 = -1; clk->clk_lsb2 = -1;
	clk->clk_msb3 = -1; clk->clk_lsb3 = -1;

	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = CLK_OENAB|CLK_8BIT;
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_IENAB;
}

void
stopclock(void)
{
	struct clkreg *clk = (struct clkreg *)clkstd[0];

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

void
initprofclock(void)
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

void
startprofclock(void)
{
}

void
stopprofclock(void)
{
}

#ifdef PROF
/*
 * profclock() is expanded in line in lev6intr() unless profiling kernel.
 * Assumes it is called with clock interrupts blocked.
 */
void
profclock(void *pc, int ps)
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
		int s = pc - s_lowpc;

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

#else	/* NCLOCK */
#error loose.
#endif
