/*	$NetBSD: clock.c,v 1.49.2.2 2009/07/23 23:31:37 jym Exp $	*/

/*
 *
 * Copyright (c) 2004 Christian Limpach.
 * All rights reserved.
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
 *      This product includes software developed by Christian Limpach.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_xen.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.49.2.2 2009/07/23 23:31:37 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/timevar.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include <xen/xen.h>
#include <xen/hypervisor.h>
#include <xen/evtchn.h>
#include <machine/cpu_counter.h>

#include <dev/clock_subr.h>
#include <x86/rtc.h>

static int xen_timer_handler(void *, struct intrframe *);

/* A timecounter: Xen system_time extrapolated with a TSC. */
u_int xen_get_timecount(struct timecounter*);
static struct timecounter xen_timecounter = {
	.tc_get_timecount = xen_get_timecount,
	.tc_poll_pps = NULL,
	.tc_counter_mask = ~0U,
	.tc_frequency = 1000000000ULL,
	.tc_name = "xen_system_time",
	.tc_quality = 10000 /*
			     * This needs to take precedence over any hardware
			     * timecounters (e.g., ACPI in Xen3 dom0), because
			     * they can't correct for Xen scheduling latency.
			     */
};

/* These are periodically updated in shared_info, and then copied here. */
static volatile uint64_t shadow_tsc_stamp;
static volatile uint64_t shadow_system_time;
static volatile unsigned long shadow_time_version; /* XXXSMP */
static volatile uint32_t shadow_freq_mul;
static volatile int8_t shadow_freq_shift;
static volatile struct timespec shadow_ts;

/* The time when the last hardclock(9) call should have taken place. */
static volatile uint64_t processed_system_time;

/*
 * The clock (as returned by xen_get_timecount) may need to be held
 * back to maintain the illusion that hardclock(9) was called when it
 * was supposed to be, not when Xen got around to scheduling us.
 */
static volatile uint64_t xen_clock_bias = 0;

#ifdef DOM0OPS
/* If we're dom0, send our time to Xen every minute or so. */
int xen_timepush_ticks = 0;
static callout_t xen_timepush_co;
#endif

#define NS_PER_TICK (1000000000ULL/hz)

/*
 * Reads a consistent set of time-base values from Xen, into a shadow data
 * area.  Must be called at splhigh (per timecounter requirements).
 */
static void
get_time_values_from_xen(void)
{
#ifdef XEN3
	volatile struct vcpu_time_info *t = &curcpu()->ci_vcpu->time;
	uint32_t tversion;

	do {
		shadow_time_version = t->version;
		xen_rmb();
		shadow_tsc_stamp = t->tsc_timestamp;
		shadow_system_time = t->system_time;
		shadow_freq_mul = t->tsc_to_system_mul;
		shadow_freq_shift = t->tsc_shift;
		xen_rmb();
	} while ((t->version & 1) || (shadow_time_version != t->version));
	do {
		tversion = HYPERVISOR_shared_info->wc_version;
		xen_rmb();
		shadow_ts.tv_sec = HYPERVISOR_shared_info->wc_sec;
		shadow_ts.tv_nsec = HYPERVISOR_shared_info->wc_nsec;
		xen_rmb();
	} while ((HYPERVISOR_shared_info->wc_version & 1) ||
	    (tversion != HYPERVISOR_shared_info->wc_version));
#else /* XEN3 */
	do {
		shadow_time_version = HYPERVISOR_shared_info->time_version2;
		xen_rmb();
		shadow_ts.tv_sec = HYPERVISOR_shared_info->wc_sec;
		shadow_ts.tv_nsec = HYPERVISOR_shared_info->wc_usec;
		shadow_tsc_stamp = HYPERVISOR_shared_info->tsc_timestamp;
		shadow_system_time = HYPERVISOR_shared_info->system_time;
		xen_rmb();
	} while (shadow_time_version != HYPERVISOR_shared_info->time_version1);
	shadow_ts.tv_nsec *= 1000;
#endif
}

/*
 * Are the values we have up to date?
 */
static inline int
time_values_up_to_date(void)
{
	int rv;

	xen_rmb();
#ifndef XEN3
	rv = shadow_time_version == HYPERVISOR_shared_info->time_version1;
#else
	rv = shadow_time_version == curcpu()->ci_vcpu->time.version;
#endif
	xen_rmb();

	return rv;
}

#ifdef XEN3
/*
 * Xen 3 helpfully provides the CPU clock speed in the form of a multiplier
 * and shift that can be used to convert a cycle count into nanoseconds
 * without using an actual (slow) divide insn.
 */
static inline uint64_t
scale_delta(uint64_t delta, uint32_t mul_frac, int8_t shift)
{
	if (shift < 0)
		delta >>= -shift;
	else
		delta <<= shift;

	/*
	 * Here, we multiply a 64-bit and a 32-bit value, and take the top
	 * 64 bits of that 96-bit product.  This is broken up into two
	 * 32*32=>64-bit multiplies and a 64-bit add.  The casts are needed
	 * to hint to GCC that both multiplicands really are 32-bit; the
	 * generated code is still fairly bad, but not insanely so.
	 */
	return ((uint64_t)(uint32_t)(delta >> 32) * mul_frac)
	    + ((((uint64_t)(uint32_t)(delta & 0xFFFFFFFF)) * mul_frac) >> 32);
}
#endif

/* 
 * Use cycle counter to determine ns elapsed since last Xen time update.
 * Must be called at splhigh (per timecounter requirements).
 */
static uint64_t
get_tsc_offset_ns(void)
{
	uint64_t tsc_delta, offset;
#ifndef XEN3
	struct cpu_info *ci = curcpu();
#endif

	tsc_delta = cpu_counter() - shadow_tsc_stamp;
#ifndef XEN3
	offset = tsc_delta * 1000000000ULL / cpu_frequency(ci);
#else
	offset = scale_delta(tsc_delta, shadow_freq_mul,
	    shadow_freq_shift);
#endif
#ifdef XEN_CLOCK_DEBUG
	if (tsc_delta > 100000000000ULL || offset > 10000000000ULL)
		printf("get_tsc_offset_ns: tsc_delta=%llu offset=%llu"
		    " pst=%llu sst=%llu\n", tsc_delta, offset,
		    processed_system_time, shadow_system_time);
#endif

	return offset;
}

/*
 * Returns the current system_time, taking care that the timestamp
 * used is valid for the TSC measurement in question.  Xen2 doesn't
 * ensure that this won't step backwards, so we enforce monotonicity
 * on our own in that case.  Must be called at splhigh.
 */
static uint64_t
get_system_time(void)
{
#ifndef XEN3
	static volatile uint64_t oldstime = 0;
#endif
	uint64_t offset, stime;
	
	for (;;) {
		offset = get_tsc_offset_ns();
		stime = shadow_system_time + offset;
		
		/* if the timestamp went stale before we used it, refresh */
		if (time_values_up_to_date()) {
			/*
			 * Work around an intermittent Xen2 bug where, for
			 * a period of 1<<32 ns, currently running domains
			 * don't get their timer events as usual (and also
			 * aren't preempted in favor of other runnable
			 * domains).  Setting the timer into the past in
			 * this way causes it to fire immediately.
			 */
#ifndef XEN3
			if (offset > 4*10000000ULL) {
#ifdef XEN_CLOCK_DEBUG
				printf("get_system_time: overlarge offset %llu"
				    " (pst=%llu sst=%llu); poking timer...\n",
				    offset, processed_system_time,
				    shadow_system_time);
#endif
				HYPERVISOR_set_timer_op(shadow_system_time);
			}
#endif
			break;
		}
		get_time_values_from_xen();
	}

#ifndef XEN3
	if (stime < oldstime) {
#ifdef XEN_CLOCK_DEBUG
		printf("xen_get_timecount: system_time backstep: %"
		    PRIu64" -> %"PRIu64" (%"PRIu64" ns)\n",
		    oldstime, stime, oldstime-stime);
#endif
		stime = oldstime;
	}
	oldstime = stime;
#endif

	return stime;
}

static void
xen_wall_time(struct timespec *wt)
{
	uint64_t nsec;
	int s;

	s = splhigh();
	get_time_values_from_xen();
	*wt = shadow_ts;
	nsec = wt->tv_nsec;
#ifdef XEN3
	/* Under Xen3, this is the wall time less system time */
	nsec += get_system_time();
	splx(s);
	wt->tv_sec += nsec / 1000000000L;
	wt->tv_nsec = nsec % 1000000000L;
#else
	/* Under Xen2 , this is the current wall time. */
	splx(s);
#endif
}

static int
xen_rtc_get(todr_chip_handle_t todr, volatile struct timeval *tvp)
{
	struct timespec wt;

	xen_wall_time(&wt);
	tvp->tv_sec = wt.tv_sec;
	tvp->tv_usec = wt.tv_nsec / 1000;

	return 0;
}

static int
xen_rtc_set(todr_chip_handle_t todr, volatile struct timeval *tvp)
{
#ifdef DOM0OPS
#if __XEN_INTERFACE_VERSION__ < 0x00030204
	dom0_op_t op;
#else
	xen_platform_op_t op;
#endif
	int s;

	if (xendomain_is_privileged()) {
#ifdef XEN3
 		/* needs to set the RTC chip too */
 		struct clock_ymdhms dt;
 		clock_secs_to_ymdhms(tvp->tv_sec, &dt);
 		rtc_set_ymdhms(NULL, &dt);
#endif
 
#if __XEN_INTERFACE_VERSION__ < 0x00030204
		op.cmd = DOM0_SETTIME;
#else
		op.cmd = XENPF_settime;
#endif
		/* XXX is rtc_offset handled correctly everywhere? */
		op.u.settime.secs	 = tvp->tv_sec;
#ifdef XEN3
		op.u.settime.nsecs	 = tvp->tv_usec * 1000;
#else
		op.u.settime.usecs	 = tvp->tv_usec;
#endif
		s = splhigh();
		op.u.settime.system_time = get_system_time();
		splx(s);
#if __XEN_INTERFACE_VERSION__ < 0x00030204
		return HYPERVISOR_dom0_op(&op);
#else
		return HYPERVISOR_platform_op(&op);
#endif
	}
#endif

	return 0;
}

void
startrtclock(void)
{
	static struct todr_chip_handle	tch;
	tch.todr_gettime = xen_rtc_get;
	tch.todr_settime = xen_rtc_set;
	tch.todr_setwen = NULL;

	todr_attach(&tch);
}

/*
 * Wait approximately `n' microseconds.
 */
void
xen_delay(unsigned int n)
{
	if (n < 500000) {
		/*
		 * shadow_system_time is updated every hz tick, it's not
		 * precise enouth for short delays. Use the CPU counter
		 * instead. We assume it's working at this point.
		 */
		uint64_t cc, cc2, when;
		struct cpu_info *ci = curcpu();

		cc = cpu_counter();
		when = cc + (uint64_t)n * cpu_frequency(ci) / 1000000LL;
		if (when < cc) {
			/* wait for counter to wrap */
			cc2 = cpu_counter();
			while (cc2 > cc)
				cc2 = cpu_counter();
		}
		cc2 = cpu_counter();
		while (cc2 < when)
			cc2 = cpu_counter();
		
		return;
	} else {
		uint64_t when;
		int s;
		/* for large delays, shadow_system_time is OK */
		
		s = splhigh();
		get_time_values_from_xen();
		when = shadow_system_time + n * 1000;
		while (shadow_system_time < when) {
			splx(s);
			s = splhigh();
			get_time_values_from_xen();
		}
		splx(s);
	}
}

#ifdef DOM0OPS
/* ARGSUSED */
static void
xen_timepush(void *arg)
{
	callout_t *co = arg;

	resettodr();
	if (xen_timepush_ticks > 0)
		callout_schedule(co, xen_timepush_ticks);
}

/* ARGSUSED */
static int
sysctl_xen_timepush(SYSCTLFN_ARGS)
{
	int error, new_ticks;
	struct sysctlnode node;

	new_ticks = xen_timepush_ticks;
	node = *rnode;
	node.sysctl_data = &new_ticks;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (new_ticks < 0)
		return EINVAL;
	if (new_ticks != xen_timepush_ticks) {
		xen_timepush_ticks = new_ticks;
		if (new_ticks > 0)
			callout_schedule(&xen_timepush_co, new_ticks);
		else
			callout_stop(&xen_timepush_co);
	}

	return 0;
}
#endif

/* ARGSUSED */
u_int
xen_get_timecount(struct timecounter *tc)
{
	uint64_t ns;
	int s;
	
	s = splhigh();
	ns = get_system_time() - xen_clock_bias;
	splx(s);

	return (u_int)ns;
}

void
xen_initclocks(void)
{
	int evtch;

#ifdef DOM0OPS
	callout_init(&xen_timepush_co, 0);
#endif
	evtch = bind_virq_to_evtch(VIRQ_TIMER);
	aprint_verbose("Xen clock: using event channel %d\n", evtch);

	get_time_values_from_xen();
	processed_system_time = shadow_system_time;
	tc_init(&xen_timecounter);
	/* The splhigh requirements start here. */

	event_set_handler(evtch, (int (*)(void *))xen_timer_handler,
	    NULL, IPL_CLOCK, "clock");
	hypervisor_enable_event(evtch);

#ifdef DOM0OPS
	xen_timepush_ticks = 53 * hz + 3; /* avoid exact # of min/sec */
	if (xendomain_is_privileged()) {
		sysctl_createv(NULL, 0, NULL, NULL, CTLFLAG_READWRITE,
		    CTLTYPE_INT, "xen_timepush_ticks", SYSCTL_DESCR("How often"
		    " to update the hypervisor's time-of-day; 0 to disable"),
		    sysctl_xen_timepush, 0, &xen_timepush_ticks, 0, 
		    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
		callout_reset(&xen_timepush_co, xen_timepush_ticks,
		    &xen_timepush, &xen_timepush_co);
	}
#endif
}

void
xen_suspendclocks(void) {

	int evtch;

	evtch = unbind_virq_from_evtch(VIRQ_TIMER);
	hypervisor_mask_event(evtch);
	event_remove_handler(evtch, (int (*)(void *))xen_timer_handler, NULL);

	aprint_verbose("Xen clock: removed event channel %d\n", evtch);

	tc_detach(&xen_timecounter);
}

/* ARGSUSED */
static int
xen_timer_handler(void *arg, struct intrframe *regs)
{
	int64_t delta;
	int s, ticks_done;

	s = splhigh();
#if 0
	get_time_values_from_xen();
#endif
	delta = (int64_t)(get_system_time() - processed_system_time);
	splx(s);

	ticks_done = 0;
	/* Several ticks may have passed without our being run; catch up. */
	while (delta >= (int64_t)NS_PER_TICK) {
		++ticks_done;
		s = splhigh();
		processed_system_time += NS_PER_TICK;
		xen_clock_bias = (delta -= NS_PER_TICK);
		splx(s);
		hardclock((struct clockframe *)regs);
	}
	
	if (xen_clock_bias) {
		s = splhigh();
 		xen_clock_bias = 0;
		splx(s);
	}

	/*
	 * Re-arm the timer here, if needed; Xen's auto-ticking while runnable
	 * is useful only for HZ==100, and even then may be out of phase with
	 * the processed_system_time steps.
	 */
	if (ticks_done != 0)
		HYPERVISOR_set_timer_op(processed_system_time + NS_PER_TICK);

	return 0;
}

void
setstatclockrate(int arg)
{
}

void
idle_block(void)
{
	int r;

	/*
	 * We set the timer to when we expect the next timer
	 * interrupt.  We could set the timer to later if we could
	 * easily find out when we will have more work (callouts) to
	 * process from hardclock.
	 */
	r = HYPERVISOR_set_timer_op(processed_system_time + NS_PER_TICK);
	if (r == 0)
		HYPERVISOR_block();
	else
		__sti();
}
