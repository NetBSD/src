/*	$NetBSD: clock.c,v 1.65 2017/11/06 15:27:09 cherry Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.65 2017/11/06 15:27:09 cherry Exp $");

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
#include <xen/xen-public/vcpu.h>
#include <machine/cpu_counter.h>

#include <dev/clock_subr.h>
#include <x86/rtc.h>

static int xen_timer_handler(void *);
static struct intrhand *ih;

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
struct shadow {
	uint64_t tsc_stamp;
	uint64_t system_time;
	unsigned long time_version; /* XXXSMP */
	uint32_t freq_mul;
	int8_t freq_shift;
	struct timespec ts;
};

/* Protects volatile variables ci_shadow & xen_clock_bias */
static kmutex_t tmutex;

/* Per CPU shadow time values */
static volatile struct shadow ci_shadow[MAXCPUS];

/* The time when the last hardclock(9) call should have taken place,
 * per cpu.
 */
static volatile uint64_t vcpu_system_time[MAXCPUS];

/*
 * The clock (as returned by xen_get_timecount) may need to be held
 * back to maintain the illusion that hardclock(9) was called when it
 * was supposed to be, not when Xen got around to scheduling us.
 */
static volatile uint64_t xen_clock_bias[MAXCPUS];

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
get_time_values_from_xen(struct cpu_info *ci)
{

	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];

	volatile struct vcpu_time_info *t = &ci->ci_vcpu->time;
	uint32_t tversion;

	KASSERT(mutex_owned(&tmutex));

	do {
		shadow->time_version = t->version;
		xen_rmb();
		shadow->tsc_stamp = t->tsc_timestamp;
		shadow->system_time = t->system_time;
		shadow->freq_mul = t->tsc_to_system_mul;
		shadow->freq_shift = t->tsc_shift;
		xen_rmb();
	} while ((t->version & 1) || (shadow->time_version != t->version));
	do {
		tversion = HYPERVISOR_shared_info->wc_version;
		xen_rmb();
		shadow->ts.tv_sec = HYPERVISOR_shared_info->wc_sec;
		shadow->ts.tv_nsec = HYPERVISOR_shared_info->wc_nsec;
		xen_rmb();
	} while ((HYPERVISOR_shared_info->wc_version & 1) ||
	    (tversion != HYPERVISOR_shared_info->wc_version));
}

/*
 * Are the values we have up to date?
 */
static inline int
time_values_up_to_date(struct cpu_info *ci)
{
	int rv;

	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];

	KASSERT(ci != NULL);
	KASSERT(mutex_owned(&tmutex));

	xen_rmb();
	rv = shadow->time_version == ci->ci_vcpu->time.version;
	xen_rmb();

	return rv;
}

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

/* 
 * Use cycle counter to determine ns elapsed since last Xen time update.
 * Must be called at splhigh (per timecounter requirements).
 */
static uint64_t
get_tsc_offset_ns(struct cpu_info *ci)
{
	uint64_t tsc_delta, offset;
	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];

	KASSERT(mutex_owned(&tmutex));
	tsc_delta = cpu_counter() - shadow->tsc_stamp;
	offset = scale_delta(tsc_delta, shadow->freq_mul,
	    shadow->freq_shift);

	return offset;
}

/*
 * Returns the current system_time on given vcpu, taking care that the
 * timestamp used is valid for the TSC measurement in question.  Xen2
 * doesn't ensure that this won't step backwards, so we enforce
 * monotonicity on our own in that case.  Must be called at splhigh.
 */
static uint64_t
get_vcpu_time(struct cpu_info *ci)
{
	uint64_t offset, stime;
	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];
	
		
	KASSERT(mutex_owned(&tmutex));
	do {
		get_time_values_from_xen(ci);
		offset = get_tsc_offset_ns(ci);
		stime = shadow->system_time + offset;
		/* if the timestamp went stale before we used it, refresh */

	} while (!time_values_up_to_date(ci));

	return stime;
}

static void
xen_wall_time(struct timespec *wt)
{
	uint64_t nsec;

	struct cpu_info *ci = curcpu();
	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];

	mutex_enter(&tmutex);
	do {
		/*
		 * Under Xen3, shadow->ts is the wall time less system time
		 * get_vcpu_time() will update shadow
		 */
		nsec = get_vcpu_time(ci);
		*wt = shadow->ts;
		nsec += wt->tv_nsec;
	} while (!time_values_up_to_date(ci));
	mutex_exit(&tmutex);

	wt->tv_sec += nsec / 1000000000L;
	wt->tv_nsec = nsec % 1000000000L;
}

static int
xen_rtc_get(todr_chip_handle_t todr, struct timeval *tvp)
{
	struct timespec wt;

	xen_wall_time(&wt);
	tvp->tv_sec = wt.tv_sec;
	tvp->tv_usec = wt.tv_nsec / 1000;

	return 0;
}

static int
xen_rtc_set(todr_chip_handle_t todr, struct timeval *tvp)
{
#ifdef DOM0OPS
#if __XEN_INTERFACE_VERSION__ < 0x00030204
	dom0_op_t op;
#else
	xen_platform_op_t op;
#endif
	if (xendomain_is_privileged()) {
 		/* needs to set the RTC chip too */
 		struct clock_ymdhms dt;
 		clock_secs_to_ymdhms(tvp->tv_sec, &dt);
 		rtc_set_ymdhms(NULL, &dt);
 
#if __XEN_INTERFACE_VERSION__ < 0x00030204
		op.cmd = DOM0_SETTIME;
#else
		op.cmd = XENPF_settime;
#endif
		/* XXX is rtc_offset handled correctly everywhere? */
		op.u.settime.secs	 = tvp->tv_sec;
		op.u.settime.nsecs	 = tvp->tv_usec * 1000;
		mutex_enter(&tmutex);
		op.u.settime.system_time = get_vcpu_time(curcpu());
		mutex_exit(&tmutex);
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
	struct cpu_info *ci = curcpu();
	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];

	if (n < 500000) {
		/*
		 * shadow->system_time is updated every hz tick, it's not
		 * precise enough for short delays. Use the CPU counter
		 * instead. We assume it's working at this point.
		 */
		uint64_t cc, cc2, when;

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

		/* for large delays, shadow->system_time is OK */
		mutex_enter(&tmutex);
		get_time_values_from_xen(ci);
		when = shadow->system_time + n * 1000;
		while (shadow->system_time < when) {
			mutex_exit(&tmutex);
			HYPERVISOR_yield();
			mutex_enter(&tmutex);
			get_time_values_from_xen(ci);
		}
		mutex_exit(&tmutex);
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

	struct cpu_info *ci = curcpu();

	mutex_enter(&tmutex);
	ns = get_vcpu_time(ci) - xen_clock_bias[ci->ci_cpuid];
	mutex_exit(&tmutex);

	return (u_int)ns;
}

/* 
 * Needs to be called per-cpu, from the local cpu, since VIRQ_TIMER is
 * bound per-cpu
 */

static struct evcnt hardclock_called[MAXCPUS];

void
xen_initclocks(void)
{
	int err __diagused;
	static bool tcdone = false;

	struct cpu_info *ci = curcpu();
	volatile struct shadow *shadow = &ci_shadow[ci->ci_cpuid];

	xen_clock_bias[ci->ci_cpuid] = 0;

	evcnt_attach_dynamic(&hardclock_called[ci->ci_cpuid],
			     EVCNT_TYPE_INTR,
			     NULL,
			     device_xname(ci->ci_dev),
			     "hardclock");

#ifdef DOM0OPS
	if (!tcdone) { /* Do this only once */
		callout_init(&xen_timepush_co, 0);
	}
#endif

	if (!tcdone) { /* Do this only once */
		mutex_init(&tmutex, MUTEX_DEFAULT, IPL_CLOCK);
	}
	mutex_enter(&tmutex);
	get_time_values_from_xen(ci);
	vcpu_system_time[ci->ci_cpuid] = shadow->system_time;
	mutex_exit(&tmutex);
	if (!tcdone) { /* Do this only once */
		tc_init(&xen_timecounter);
	}

	/* The splhigh requirements start here. */
	xen_resumeclocks(ci);

	/*
	 * The periodic timer looks buggy, we stop receiving events
	 * after a while. Use the one-shot timer every NS_PER_TICK
	 * and rearm it from the event handler.
	 */
	if (XEN_MAJOR(xen_version) > 3 || XEN_MINOR(xen_version) > 0) {
		/* exists only on Xen 3.1 and later */
		err = HYPERVISOR_vcpu_op(VCPUOP_stop_periodic_timer,
					 ci->ci_cpuid,
				 NULL);
		KASSERT(err == 0);
	}

	err = HYPERVISOR_set_timer_op(
	    vcpu_system_time[ci->ci_cpuid] + NS_PER_TICK);
	KASSERT(err == 0);

#ifdef DOM0OPS
	const struct sysctlnode *node = NULL;

	if (!tcdone) { /* Do this only once */

		xen_timepush_ticks = 53 * hz + 3; /* avoid exact # of min/sec */
		if (xendomain_is_privileged()) {
			sysctl_createv(NULL, 0, NULL, &node, 0,
			    CTLTYPE_NODE, "xen",
			    SYSCTL_DESCR("Xen top level node"),
			    NULL, 0, NULL, 0,
			    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
			if (node != NULL) {
				sysctl_createv(NULL, 0, &node, NULL,
				    CTLFLAG_READWRITE, CTLTYPE_INT,
				    "timepush_ticks",
				    SYSCTL_DESCR("How often to update the "
				    "hypervisor's time-of-day; 0 to disable"),
				    sysctl_xen_timepush, 0,
				    &xen_timepush_ticks, 0, 
				    CTL_CREATE, CTL_EOL);
			}
			callout_reset(&xen_timepush_co, xen_timepush_ticks,
			     &xen_timepush, &xen_timepush_co);
		}
	}
#endif
	tcdone = true;
}

void
xen_suspendclocks(struct cpu_info *ci)
{
	int evtch;

	evtch = unbind_virq_from_evtch(VIRQ_TIMER);
	KASSERT(evtch != -1);

	hypervisor_mask_event(evtch);
	intr_disestablish(ih);

	aprint_verbose("Xen clock: removed event channel %d\n", evtch);
}

void
xen_resumeclocks(struct cpu_info *ci)
{
	int evtch;
       
	evtch = bind_virq_to_evtch(VIRQ_TIMER);
	KASSERT(evtch != -1);

	ih = intr_establish_xname(0, &xen_pic, evtch, IST_LEVEL, IPL_CLOCK,
	    xen_timer_handler, ci, true, "clock");

	KASSERT(ih != NULL);

	hypervisor_enable_event(evtch);

	aprint_verbose("Xen clock: using event channel %d\n", evtch);
}

/* ARGSUSED */
static int
xen_timer_handler(void *arg)
{
	int64_t delta;
	struct cpu_info *ci = curcpu();
	struct intrframe *regs = arg;

	int err;
again:
	mutex_enter(&tmutex);
	delta = (int64_t)(get_vcpu_time(ci) - vcpu_system_time[ci->ci_cpuid]);
	mutex_exit(&tmutex);

	/* Several ticks may have passed without our being run; catch up. */
	while (delta >= (int64_t)NS_PER_TICK) {
		mutex_enter(&tmutex);
		vcpu_system_time[ci->ci_cpuid] += NS_PER_TICK;
		xen_clock_bias[ci->ci_cpuid] = (delta -= NS_PER_TICK);
		mutex_exit(&tmutex);
		hardclock((struct clockframe *)regs);
		hardclock_called[ci->ci_cpuid].ev_count++;
	}

	/*
	 * rearm the timer. If it fails it's probably because the date
	 * is in the past, update our local time and try again.
	 */
	err = HYPERVISOR_set_timer_op(
	    vcpu_system_time[ci->ci_cpuid] + NS_PER_TICK);
	if (err)
		goto again;
	
	if (xen_clock_bias[ci->ci_cpuid]) {
		mutex_enter(&tmutex);
		xen_clock_bias[ci->ci_cpuid] = 0;
		mutex_exit(&tmutex);
	}

	return 0;
}

void
setstatclockrate(int arg)
{
}

void
idle_block(void)
{
	KASSERT(curcpu()->ci_ipending == 0);
	HYPERVISOR_block();
}
