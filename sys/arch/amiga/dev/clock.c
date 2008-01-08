/*	$NetBSD: clock.c,v 1.46.32.1 2008/01/08 22:09:22 bouyer Exp $ */

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)clock.c	7.6 (Berkeley) 5/7/91
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
 *	@(#)clock.c	7.6 (Berkeley) 5/7/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.46.32.1 2008/01/08 22:09:22 bouyer Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <machine/psl.h>
#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#ifdef DRACO
#include <amiga/amiga/drcustom.h>
#include <m68k/include/asm_single.h>
#endif
#include <amiga/dev/rtc.h>
#include <amiga/dev/zbusvar.h>

#if defined(PROF) && defined(PROFTIMER)
#include <sys/PROF.h>
#endif

/* the clocks run at NTSC: 715.909kHz or PAL: 709.379kHz.
   We're using a 100 Hz clock. */
int amiga_clk_interval;
int eclockfreq;
unsigned int fast_delay_limit;
struct CIA *clockcia;

static u_int clk_getcounter(struct timecounter *);

static struct timecounter clk_timecounter = {
	clk_getcounter,	/* get_timecount */
	0,		/* no poll_pps */
	~0u,		/* counter_mask */
	0,		/* frequency */
	"clock",	/* name, overriden later */
	100,		/* quality */
	NULL,		/* prev */
	NULL,		/* next */
};

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
 * We actually load the clock with amiga_clk_interval-1 instead of amiga_clk_interval.
 * This is because the counter decrements to zero after N+1 enabled clock
 * periods where N is the value loaded into the counter.
 */

int clockmatch(struct device *, struct cfdata *, void *);
void clockattach(struct device *, struct device *, void *);
void cpu_initclocks(void);

CFATTACH_DECL(clock, sizeof(struct device),
    clockmatch, clockattach, NULL, NULL);

int
clockmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	if (matchname("clock", auxp))
		return(1);
	return(0);
}

/*
 * Start the real-time clock.
 */
void
clockattach(struct device *pdp, struct device *dp, void *auxp)
{
	const char *clockchip;
	unsigned short interval;
#ifdef DRACO
	u_char dracorev;
#endif

#ifdef DRACO
	dracorev = is_draco();
#endif

	if (eclockfreq == 0)
		eclockfreq = 715909;	/* guess NTSC */

#ifdef DRACO
	if (dracorev >= 4) {
		if (amiga_clk_interval == 0)	/* Only do this 1st time */
			eclockfreq /= 7;
		clockchip = "QuickLogic";
	} else if (dracorev) {
		clockcia = (struct CIA *)CIAAbase;
		clockchip = "CIA A";
	} else
#endif
	{
		clockcia = (struct CIA *)CIABbase;
		clockchip = "CIA B";
	}

	amiga_clk_interval = (eclockfreq / hz);

	clk_timecounter.tc_name = clockchip;
	clk_timecounter.tc_frequency = eclockfreq;

	fast_delay_limit = UINT_MAX / amiga_clk_interval;

	if (dp != NULL) {	/* real autoconfig? */
		printf(": %s system hz %d hardware hz %d\n", clockchip, hz,
		    eclockfreq);
		tc_init(&clk_timecounter);
	}

#ifdef DRACO
	if (dracorev >= 4) {
		/*
		 * can't preload anything beforehand, timer is free_running;
		 * but need this for delay calibration.
		 */

		draco_ioct->io_timerlo = amiga_clk_interval & 0xff;
		draco_ioct->io_timerhi = amiga_clk_interval >> 8;

		return;
	}
#endif
	/*
	 * stop timer A
	 */
	clockcia->cra = clockcia->cra & 0xc0;
	clockcia->icr = 1 << 0;		/* disable timer A interrupt */
	interval = clockcia->icr;		/* and make sure it's clear */

	/*
	 * load interval into registers.
         * the clocks run at NTSC: 715.909kHz or PAL: 709.379kHz
	 * supprort for PAL WHEN?!?! XXX
	 */
	interval = amiga_clk_interval - 1;

	/*
	 * order of setting is important !
	 */
	clockcia->talo = interval & 0xff;
	clockcia->tahi = interval >> 8;
	/*
	 * start timer A in continuous mode
	 */
	clockcia->cra = (clockcia->cra & 0xc0) | 1;
}

void
cpu_initclocks(void)
{
#ifdef DRACO
	unsigned char dracorev;
	dracorev = is_draco();
	if (dracorev >= 4) {
		draco_ioct->io_timerlo = amiga_clk_interval & 0xFF;
		draco_ioct->io_timerhi = amiga_clk_interval >> 8;
		draco_ioct->io_timerrst = 0;	/* any value resets */
		single_inst_bset_b(draco_ioct->io_status2, DRSTAT2_TMRINTENA);

		return;
	}
#endif
	/*
	 * enable interrupts for timer A
	 */
	clockcia->icr = (1<<7) | (1<<0);

	/*
	 * start timer A in continuous shot mode
	 */
	clockcia->cra = (clockcia->cra & 0xc0) | 1;

	/*
	 * and globally enable interrupts for ciab
	 */
#ifdef DRACO
	if (dracorev)		/* we use cia a on DraCo */
		single_inst_bset_b(*draco_intena, DRIRQ_INT2);
	else
#endif
		custom.intena = INTF_SETCLR | INTF_EXTER;

}

void
setstatclockrate(int hertz)
{
}

/*
 * Returns ticks  since last recorded clock "tick"
 * (i.e. clock interrupt).
 */
static u_int
clk_gettick(void)
{
	u_int interval;
	u_char hi, hi2, lo;

#ifdef DRACO
	if (is_draco() >= 4) {
		hi2 = draco_ioct->io_chiprev;	/* latch timer */
		hi = draco_ioct->io_timerhi;
		lo = draco_ioct->io_timerlo;
		interval = ((hi<<8) | lo);
		if (interval > amiga_clk_interval)	/* timer underflow */
			interval = 65536 + amiga_clk_interval - interval;
		else
			interval = amiga_clk_interval - interval;

	} else
#endif
	{
		hi  = clockcia->tahi;
		lo  = clockcia->talo;
		hi2 = clockcia->tahi;
		if (hi != hi2) {
			lo = clockcia->talo;
			hi = hi2;
		}

		interval = (amiga_clk_interval - 1) - ((hi<<8) | lo);

		/*
		 * should read ICR and if there's an int pending, adjust
		 * interval. However, since reading ICR clears the interrupt,
		 * we'd lose a hardclock int, and this is not tolerable.
		 */
	}

	return interval;
}

static u_int
clk_getcounter(struct timecounter *tc)
{
	int old_hardclock_ticks;
	u_int clock_tick;

	do {
		old_hardclock_ticks = hardclock_ticks;
		clock_tick = clk_gettick();
	} while (old_hardclock_ticks != hardclock_ticks);

	return old_hardclock_ticks * amiga_clk_interval + clock_tick;
}

#if notyet

/* implement this later. I'd suggest using both timers in CIA-A, they're
   not yet used. */

#include "clock.h"
#if NCLOCK > 0
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
#include <uvm/uvm_extern.h>
#include <amiga/amiga/clockioctl.h>
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
#endif
	/*
	 * If any user processes are profiling, give up.
	 */
	if (profprocs)
		return(EBUSY);
#endif
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
	(void) clockunmmap(dev, (void *)0, curproc);	/* XXX */
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
void
clockmap(dev_t dev, int off, int prot)
{
	return((off + (INTIOBASE+CLKBASE+CLKSR-1)) >> PGSHIFT);
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
	error = vm_mmap(&p->p_vmspace->vm_map, (vm_offset_t *)addrp,
			PAGE_SIZE, VM_PROT_ALL, flags, (void *)&vn, 0);
	return(error);
}

int
clockunmmap(dev_t dev, void *addr, struct proc *p)
{
	int rv;

	if (addr == 0)
		return(EINVAL);		/* XXX: how do we deal with this? */
	uvm_deallocate(p->p_vmspace->vm_map, (vm_offset_t)addr, PAGE_SIZE);
	return 0;
}

void
startclock(void)
{
	register struct clkreg *clk = (struct clkreg *)clkstd[0];

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
	register struct clkreg *clk = (struct clkreg *)clkstd[0];

	clk->clk_cr2 = CLK_CR3;
	clk->clk_cr3 = 0;
	clk->clk_cr2 = CLK_CR1;
	clk->clk_cr1 = CLK_IENAB;
}
#endif

#endif


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
#if NCLOCK > 0
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
#endif
	/*
	 * The profile interrupt interval must be an even divisor
	 * of the amiga_clk_interval so that scaling from a system clock
	 * tick to a profile clock tick is possible using integer math.
	 */
	if (profint > amiga_clk_interval || (amiga_clk_interval % profint) != 0)
		profint = amiga_clk_interval;
	profscale = amiga_clk_interval / profint;
}

void
startprofclock(void)
{
  unsigned short interval;

  /* stop timer B */
  clockcia->crb = clockcia->crb & 0xc0;

  /* load interval into registers.
     the clocks run at NTSC: 715.909kHz or PAL: 709.379kHz */

  interval = profint - 1;

  /* order of setting is important ! */
  clockcia->tblo = interval & 0xff;
  clockcia->tbhi = interval >> 8;

  /* enable interrupts for timer B */
  clockcia->icr = (1<<7) | (1<<1);

  /* start timer B in continuous shot mode */
  clockcia->crb = (clockcia->crb & 0xc0) | 1;
}

void
stopprofclock(void)
{
  /* stop timer B */
  clockcia->crb = clockcia->crb & 0xc0;
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
#endif
#endif

void
delay(unsigned int n)
{
	unsigned int cur_tick, initial_tick;
	int remaining;

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	initial_tick = clk_gettick();

	if (amiga_clk_interval == 0) {
		/*
		 * Clock is not initialised yet,
		 * so just do some ad-hoc loop.
		 */
		static uint32_t dummy;

		n *= 4;
		while (n--)
			dummy *= eclockfreq;
		return;
	}

	if (n <= fast_delay_limit) {
		/*
		 * For unsigned arithmetic, division can be replaced with
		 * multiplication with the inverse and a shift.
		 */
		remaining = n * eclockfreq / 1000000;
	} else {
		/* This is a very long delay.
		 * Being slow here doesn't matter.
		 */
		remaining = (unsigned long long) n * eclockfreq / 1000000;
	}

	while (remaining > 0) {
		cur_tick = clk_gettick();
		if (cur_tick > initial_tick)
			remaining -= amiga_clk_interval - (cur_tick - initial_tick);
		else
			remaining -= initial_tick - cur_tick;
		initial_tick = cur_tick;
	}
}
