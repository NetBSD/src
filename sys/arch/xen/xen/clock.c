/*	$NetBSD: clock.c,v 1.65.2.3 2018/07/28 04:37:43 pgoyette Exp $	*/

/*-
 * Copyright (c) 2017, 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#include "opt_xen.h"

#ifndef XEN_CLOCK_DEBUG
#define	XEN_CLOCK_DEBUG	0
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.65.2.3 2018/07/28 04:37:43 pgoyette Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>

#include <dev/clock_subr.h>

#include <machine/cpu.h>
#include <machine/cpu_counter.h>
#include <machine/lock.h>

#include <xen/evtchn.h>
#include <xen/hypervisor.h>
#include <xen/xen-public/vcpu.h>
#include <xen/xen.h>

#include <x86/rtc.h>

#define NS_PER_TICK ((uint64_t)1000000000ULL/hz)

static uint64_t	xen_vcputime_systime_ns(void);
static uint64_t	xen_vcputime_raw_systime_ns(void);
static void	xen_wallclock_time(struct timespec *);
static uint64_t	xen_global_systime_ns(void);
static unsigned	xen_get_timecount(struct timecounter *);
static int	xen_rtc_get(struct todr_chip_handle *, struct timeval *);
static int	xen_rtc_set(struct todr_chip_handle *, struct timeval *);
static int	xen_timer_handler(void *, struct clockframe *);

/*
 * xen timecounter:
 *
 *	Xen vCPU system time, plus an adjustment with rdtsc.
 */
static struct timecounter xen_timecounter = {
	.tc_get_timecount = xen_get_timecount,
	.tc_poll_pps = NULL,
	.tc_counter_mask = ~0U,
	.tc_frequency = 1000000000ULL,	/* 1 GHz, i.e. units of nanoseconds */
	.tc_name = "xen_system_time",
	.tc_quality = 10000,
};

/*
 * xen_global_systime_ns_stamp
 *
 *	The latest Xen vCPU system time that has been observed on any
 *	CPU, for a global monotonic view of the Xen system time clock.
 */
static volatile uint64_t xen_global_systime_ns_stamp __cacheline_aligned;

/*
 * xen time of day register:
 *
 *	Xen wall clock time, plus a Xen vCPU system time adjustment.
 */
static struct todr_chip_handle xen_todr_chip = {
	.todr_gettime = xen_rtc_get,
	.todr_settime = xen_rtc_set,
};

#ifdef DOM0OPS
/*
 * xen timepush state:
 *
 *	Callout to periodically, after a sysctl-configurable number of
 *	NetBSD ticks, set the Xen hypervisor's wall clock time.
 */
static struct {
	struct callout	ch;
	int		ticks;
} xen_timepush;

static void	xen_timepush_init(void);
static void	xen_timepush_intr(void *);
static int	sysctl_xen_timepush(SYSCTLFN_ARGS);
#endif

/*
 * startrtclock()
 *
 *	Initialize the real-time clock from x86 machdep autoconf.
 */
void
startrtclock(void)
{

	todr_attach(&xen_todr_chip);
}

/*
 * setstatclockrate(rate)
 *
 *	Set the statclock to run at rate, in units of ticks per second.
 *
 *	Currently Xen does not have a separate statclock, so this is a
 *	noop; instad the statclock runs in hardclock.
 */
void
setstatclockrate(int rate)
{
}

/*
 * idle_block()
 *
 *	Called from the idle loop when we have nothing to do but wait
 *	for an interrupt.
 */
void
idle_block(void)
{

	KASSERT(curcpu()->ci_ipending == 0);
	HYPERVISOR_block();
}

/*
 * xen_rdtsc()
 *
 *	Read the local pCPU's tsc.
 */
static inline uint64_t
xen_rdtsc(void)
{
	uint32_t lo, hi;

	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

	return ((uint64_t)hi << 32) | lo;
}

/*
 * struct xen_vcputime_ticket
 *
 *	State for a vCPU read section, during which a caller may read
 *	from fields of a struct vcpu_time_info and call xen_rdtsc.
 *	Caller must enter with xen_vcputime_enter, exit with
 *	xen_vcputime_exit, and be prepared to retry if
 *	xen_vcputime_exit fails.
 */
struct xen_vcputime_ticket {
	uint64_t	version;
};

/*
 * xen_vcputime_enter(tp)
 *
 *	Enter a vCPU time read section and store a ticket in *tp, which
 *	the caller must use with xen_vcputime_exit.  Return a pointer
 *	to the current CPU's vcpu_time_info structure.  Caller must
 *	already be bound to the CPU.
 */
static inline volatile struct vcpu_time_info *
xen_vcputime_enter(struct xen_vcputime_ticket *tp)
{
	volatile struct vcpu_time_info *vt = &curcpu()->ci_vcpu->time;

	while (__predict_false(1 & (tp->version = vt->version)))
		SPINLOCK_BACKOFF_HOOK;

	/*
	 * Must read the version before reading the tsc on the local
	 * pCPU.  We are racing only with interruption by the
	 * hypervisor, so no need for a stronger memory barrier.
	 */
	__insn_barrier();

	return vt;
}

/*
 * xen_vcputime_exit(vt, tp)
 *
 *	Exit a vCPU time read section with the ticket in *tp from
 *	xen_vcputime_enter.  Return true on success, false if caller
 *	must retry.
 */
static inline bool
xen_vcputime_exit(volatile struct vcpu_time_info *vt,
    struct xen_vcputime_ticket *tp)
{

	KASSERT(vt == &curcpu()->ci_vcpu->time);

	/*
	 * Must read the tsc before re-reading the version on the local
	 * pCPU.  We are racing only with interruption by the
	 * hypervisor, so no need for a stronger memory barrier.
	 */
	__insn_barrier();

	return tp->version == vt->version;
}

/*
 * xen_tsc_to_ns_delta(delta_tsc, mul_frac, shift)
 *
 *	Convert a difference in tsc units to a difference in
 *	nanoseconds given a multiplier and shift for the unit
 *	conversion.
 */
static inline uint64_t
xen_tsc_to_ns_delta(uint64_t delta_tsc, uint32_t tsc_to_system_mul,
    int8_t tsc_shift)
{
	uint32_t delta_tsc_hi, delta_tsc_lo;

	if (tsc_shift < 0)
		delta_tsc >>= -tsc_shift;
	else
		delta_tsc <<= tsc_shift;

	delta_tsc_hi = delta_tsc >> 32;
	delta_tsc_lo = delta_tsc & 0xffffffffUL;

	/* d*m/2^32 = (2^32 d_h + d_l)*m/2^32 = d_h*m + (d_l*m)/2^32 */
	return ((uint64_t)delta_tsc_hi * tsc_to_system_mul) +
	    (((uint64_t)delta_tsc_lo * tsc_to_system_mul) >> 32);
}

/*
 * xen_vcputime_systime_ns()
 *
 *	Return a snapshot of the Xen system time plus an adjustment
 *	from the tsc, in units of nanoseconds.  Caller must be bound to
 *	the current CPU.
 */
static uint64_t
xen_vcputime_systime_ns(void)
{
	volatile struct vcpu_time_info *vt;
	struct cpu_info *ci = curcpu();
	struct xen_vcputime_ticket ticket;
	uint64_t raw_systime_ns, tsc_timestamp, tsc, delta_tsc, delta_ns;
	uint32_t tsc_to_system_mul;
	int8_t tsc_shift;
	uint64_t systime_ns;

	/* We'd better be bound to the CPU in _some_ way.  */
	KASSERT(cpu_intr_p() || cpu_softintr_p() || kpreempt_disabled() ||
	    (curlwp->l_flag & LP_BOUND));

	/*
	 * Repeatedly try to read the system time, corresponding tsc
	 * timestamp, and tsc frequency until we get a consistent view.
	 */
	do {
		vt = xen_vcputime_enter(&ticket);

		/* Grab Xen's snapshot of raw system time and tsc.  */
		raw_systime_ns = vt->system_time;
		tsc_timestamp = vt->tsc_timestamp;

		/* Get Xen's current idea of how fast the tsc is counting.  */
		tsc_to_system_mul = vt->tsc_to_system_mul;
		tsc_shift = vt->tsc_shift;

		/* Read the CPU's tsc.  */
		tsc = xen_rdtsc();
	} while (!xen_vcputime_exit(vt, &ticket));

	/*
	 * Out of paranoia, check whether the tsc has gone backwards
	 * since Xen's timestamp.
	 *
	 * This shouldn't happen because the Xen hypervisor is supposed
	 * to have read the tsc _before_ writing to the vcpu_time_info
	 * page, _before_ we read the tsc.
	 *
	 * Further, if we switched pCPUs after reading the tsc
	 * timestamp but before reading the CPU's tsc, the hypervisor
	 * had better notify us by updating the version too and forcing
	 * us to retry the vCPU time read.
	 */
	if (__predict_false(tsc < tsc_timestamp)) {
		/*
		 * Notify the console that the CPU's tsc appeared to
		 * run behind Xen's idea of it, and pretend it hadn't.
		 */
#if XEN_CLOCK_DEBUG		/* XXX dtrace hook */
		printf("xen cpu tsc %"PRIu64
		    " ran backwards from timestamp %"PRIu64
		    " by %"PRIu64"\n",
		    tsc, tsc_timestamp, tsc_timestamp - tsc);
#endif
		ci->ci_xen_cpu_tsc_backwards_evcnt.ev_count++;
		delta_ns = delta_tsc = 0;
	} else {
		/* Find how far the CPU's tsc has advanced.  */
		delta_tsc = tsc - tsc_timestamp;

		/* Convert the tsc delta to a nanosecond delta.  */
		delta_ns = xen_tsc_to_ns_delta(delta_tsc, tsc_to_system_mul,
		    tsc_shift);
	}

	/*
	 * Notify the console if the delta computation yielded a
	 * negative, and pretend it hadn't.
	 *
	 * This doesn't make sense but I include it out of paranoia.
	 */
	if (__predict_false((int64_t)delta_ns < 0)) {
#if XEN_CLOCK_DEBUG		/* XXX dtrace hook */
		printf("xen tsc delta in ns went negative: %"PRId64"\n",
		    delta_ns);
#endif
		ci->ci_xen_tsc_delta_negative_evcnt.ev_count++;
		delta_ns = 0;
	}

	/*
	 * Compute the TSC-adjusted system time.
	 */
	systime_ns = raw_systime_ns + delta_ns;

	/*
	 * Notify the console if the addition wrapped around.
	 *
	 * This shouldn't happen because system time should be relative
	 * to a reasonable reference point, not centuries in the past.
	 * (2^64 ns is approximately half a millennium.)
	 */
	if (__predict_false(systime_ns < raw_systime_ns)) {
#if XEN_CLOCK_DEBUG		/* XXX dtrace hook */
		printf("xen raw systime + tsc delta wrapped around:"
		    " %"PRIu64" + %"PRIu64" = %"PRIu64"\n",
		    raw_systime_ns, delta_ns, systime_ns);
#endif
		ci->ci_xen_raw_systime_wraparound_evcnt.ev_count++;
	}

	/*
	 * Notify the console if the TSC-adjusted Xen system time
	 * appears to have gone backwards, and pretend we had gone
	 * forward.  This seems to happen pretty regularly under load.
	 */
	if (__predict_false(ci->ci_xen_last_systime_ns > systime_ns)) {
#if XEN_CLOCK_DEBUG		/* XXX dtrace hook */
		printf("xen raw systime + tsc delta went backwards:"
		    " %"PRIu64" > %"PRIu64"\n",
		    ci->ci_xen_last_systime_ns, systime_ns);
		printf(" raw_systime_ns=%"PRIu64"\n tsc_timestamp=%"PRIu64"\n"
		    " tsc=%"PRIu64"\n tsc_to_system_mul=%"PRIu32"\n"
		    " tsc_shift=%"PRId8"\n delta_tsc=%"PRIu64"\n"
		    " delta_ns=%"PRIu64"\n",
		    raw_systime_ns, tsc_timestamp, tsc, tsc_to_system_mul,
		    tsc_shift, delta_tsc, delta_ns);
#endif
		ci->ci_xen_raw_systime_backwards_evcnt.ev_count++;
		systime_ns = ci->ci_xen_last_systime_ns + 1;
	}

	/* Remember the TSC-adjusted Xen system time.  */
	ci->ci_xen_last_systime_ns = systime_ns;

	/* We had better not have migrated CPUs.  */
	KASSERT(ci == curcpu());

	/* And we're done: return the TSC-adjusted systime in nanoseconds.  */
	return systime_ns;
}

/*
 * xen_vcputime_raw_systime_ns()
 *
 *	Return a snapshot of the current Xen system time to the
 *	resolution of the Xen hypervisor tick, in units of nanoseconds.
 */
static uint64_t
xen_vcputime_raw_systime_ns(void)
{
	volatile struct vcpu_time_info *vt;
	struct xen_vcputime_ticket ticket;
	uint64_t raw_systime_ns;

	do {
		vt = xen_vcputime_enter(&ticket);
		raw_systime_ns = vt->system_time;
	} while (!xen_vcputime_exit(vt, &ticket));

	return raw_systime_ns;
}

/*
 * struct xen_wallclock_ticket
 *
 *	State for a wall clock read section, during which a caller may
 *	read from the wall clock fields of HYPERVISOR_shared_info.
 *	Caller must enter with xen_wallclock_enter, exit with
 *	xen_wallclock_exit, and be prepared to retry if
 *	xen_wallclock_exit fails.
 */
struct xen_wallclock_ticket {
	uint32_t version;
};

/*
 * xen_wallclock_enter(tp)
 *
 *	Enter a wall clock read section and store a ticket in *tp,
 *	which the caller must use with xen_wallclock_exit.
 */
static inline void
xen_wallclock_enter(struct xen_wallclock_ticket *tp)
{

	while (__predict_false(1 & (tp->version =
		    HYPERVISOR_shared_info->wc_version)))
		SPINLOCK_BACKOFF_HOOK;

	/*
	 * Must read the version from memory before reading the
	 * timestamp from memory, as written potentially by another
	 * pCPU.
	 */
	membar_consumer();
}

/*
 * xen_wallclock_exit(tp)
 *
 *	Exit a wall clock read section with the ticket in *tp from
 *	xen_wallclock_enter.  Return true on success, false if caller
 *	must retry.
 */
static inline bool
xen_wallclock_exit(struct xen_wallclock_ticket *tp)
{

	/*
	 * Must read the timestamp from memory before re-reading the
	 * version from memory, as written potentially by another pCPU.
	 */
	membar_consumer();

	return tp->version == HYPERVISOR_shared_info->wc_version;
}

/*
 * xen_wallclock_time(tsp)
 *
 *	Return a snapshot of the current low-resolution wall clock
 *	time, as reported by the hypervisor, in tsp.
 */
static void
xen_wallclock_time(struct timespec *tsp)
{
	struct xen_wallclock_ticket ticket;
	uint64_t systime_ns;

	/* Read the last wall clock sample from the hypervisor. */
	do {
		xen_wallclock_enter(&ticket);
		tsp->tv_sec = HYPERVISOR_shared_info->wc_sec;
		tsp->tv_nsec = HYPERVISOR_shared_info->wc_nsec;
	} while (!xen_wallclock_exit(&ticket));

	/* Get the global system time.  */
	systime_ns = xen_global_systime_ns();

	/* Add the system time to the wall clock time.  */
	systime_ns += tsp->tv_nsec;
	tsp->tv_sec += systime_ns / 1000000000ull;
	tsp->tv_nsec = systime_ns % 1000000000ull;
}

/*
 * xen_global_systime_ns()
 *
 *	Return a global monotonic view of the system time in
 *	nanoseconds, computed by the per-CPU Xen raw system time plus
 *	an rdtsc adjustment, and advance the view of the system time
 *	for all other CPUs.
 */
static uint64_t
xen_global_systime_ns(void)
{
	struct cpu_info *ci;
	uint64_t local, global, result;
	int bound;

	/*
	 * Find the local timecount on this CPU, and make sure it does
	 * not precede the latest global timecount witnessed so far by
	 * any CPU.  If it does, add to the local CPU's skew from the
	 * fastest CPU.
	 *
	 * XXX Can we avoid retrying if the CAS fails?
	 */
	bound = curlwp_bind();
	ci = curcpu();
	do {
		local = xen_vcputime_systime_ns();
		local += ci->ci_xen_systime_ns_skew;
		global = xen_global_systime_ns_stamp;
		if (__predict_false(local < global + 1)) {
			result = global + 1;
			ci->ci_xen_systime_ns_skew += global + 1 - local;
		} else {
			result = local;
		}
	} while (atomic_cas_64(&xen_global_systime_ns_stamp, global, result)
	    != global);
	KASSERT(ci == curcpu());
	curlwp_bindx(bound);

	return result;
}

/*
 * xen_get_timecount(tc)
 *
 *	Return the low 32 bits of a global monotonic view of the Xen
 *	system time.
 */
static unsigned
xen_get_timecount(struct timecounter *tc)
{

	KASSERT(tc == &xen_timecounter);

	return (unsigned)xen_global_systime_ns();
}

/*
 * xen_rtc_get(todr, tv)
 *
 *	Get the current real-time clock from the Xen wall clock time
 *	and vCPU system time adjustment.
 */
static int
xen_rtc_get(struct todr_chip_handle *todr, struct timeval *tvp)
{
	struct timespec ts;

	xen_wallclock_time(&ts);
	TIMESPEC_TO_TIMEVAL(tvp, &ts);

	return 0;
}

/*
 * xen_rtc_set(todr, tv)
 *
 *	Set the Xen wall clock time, if we can.
 */
static int
xen_rtc_set(struct todr_chip_handle *todr, struct timeval *tvp)
{
#ifdef DOM0OPS
	struct clock_ymdhms dt;
	xen_platform_op_t op;
	uint64_t systime_ns;

	if (xendomain_is_privileged()) {
		/* Convert to ymdhms and set the x86 ISA RTC.  */
		clock_secs_to_ymdhms(tvp->tv_sec, &dt);
		rtc_set_ymdhms(NULL, &dt);

		/* Get the global system time so we can preserve it.  */
		systime_ns = xen_global_systime_ns();

		/* Set the hypervisor wall clock time.  */
		op.cmd = XENPF_settime;
		op.u.settime.secs = tvp->tv_sec;
		op.u.settime.nsecs = tvp->tv_usec * 1000;
		op.u.settime.system_time = systime_ns;
		return HYPERVISOR_platform_op(&op);
	}
#endif

	/* XXX Should this fail if not on privileged dom0?  */
	return 0;
}

/*
 * xen_delay(n)
 *
 *	Wait approximately n microseconds.
 */
void
xen_delay(unsigned n)
{
	int bound;

	/* Bind to the CPU so we don't compare tsc on different CPUs.  */
	bound = curlwp_bind();

	/* Short wait (<500us) or long wait?  */
	if (n < 500000) {
		/*
		 * Xen system time is not precise enough for short
		 * delays, so use the tsc instead.
		 *
		 * We work with the current tsc frequency, and figure
		 * that if it changes while we're delaying, we've
		 * probably delayed long enough -- up to 500us.
		 *
		 * We do not use cpu_frequency(ci), which uses a
		 * quantity detected at boot time, and which may have
		 * changed by now if Xen has migrated this vCPU to
		 * another pCPU.
		 *
		 * XXX How long does it take to migrate pCPUs?
		 */
		volatile struct vcpu_time_info *vt;
		struct xen_vcputime_ticket ticket;
		uint64_t tsc_start, last_tsc, tsc;
		uint32_t tsc_to_system_mul;
		int8_t tsc_shift;

		/* Get the starting tsc and tsc frequency.  */
		do {
			vt = xen_vcputime_enter(&ticket);
			tsc_start = last_tsc = xen_rdtsc();
			tsc_to_system_mul = vt->tsc_to_system_mul;
			tsc_shift = vt->tsc_shift;
		} while (!xen_vcputime_exit(vt, &ticket));

		/*
		 * Wait until as many tsc ticks as there are in n
		 * microseconds have elapsed, or the tsc has gone
		 * backwards meaning we've probably migrated pCPUs.
		 */
		for (;;) {
			tsc = xen_rdtsc();
			if (__predict_false(tsc < last_tsc))
				break;
			if (xen_tsc_to_ns_delta(tsc - tsc_start,
				tsc_to_system_mul, tsc_shift)/1000 >= n)
				break;
			last_tsc = tsc;
		}
	} else {
		/*
		 * Use the Xen system time for >=500us delays.  From my
		 * testing, it seems to sometimes run backward by about
		 * 110us, which is not so bad.
		 */
		uint64_t n_ns = 1000*(uint64_t)n;
		uint64_t start_ns;

		/* Get the start time.  */
		start_ns = xen_vcputime_raw_systime_ns();

		/* Wait until the system time has passed the end.  */
		do {
			HYPERVISOR_yield();
		} while (xen_vcputime_raw_systime_ns() - start_ns < n_ns);
	}

	/* Unbind from the CPU if we weren't already bound.  */
	curlwp_bindx(bound);
}

/*
 * xen_suspendclocks(ci)
 *
 *	Stop handling the Xen timer event on the CPU of ci.  Caller
 *	must be running on and bound to ci's CPU.
 *
 *	Actually, caller must have kpreemption disabled, because that's
 *	easier to assert at the moment.
 */
void
xen_suspendclocks(struct cpu_info *ci)
{
	int evtch;

	KASSERT(ci == curcpu());
	KASSERT(kpreempt_disabled());
	KASSERT(ci->ci_xen_timer_intrhand != NULL);

	evtch = unbind_virq_from_evtch(VIRQ_TIMER);
	KASSERT(evtch != -1);

	hypervisor_mask_event(evtch);
	intr_disestablish(ci->ci_xen_timer_intrhand);
	ci->ci_xen_timer_intrhand = NULL;

	aprint_verbose("Xen clock: removed event channel %d\n", evtch);

	/* We'd better not have switched CPUs.  */
	KASSERT(ci == curcpu());
}

/*
 * xen_resumeclocks(ci)
 *
 *	Start handling the Xen timer event on the CPU of ci.  Arm the
 *	Xen timer.  Caller must be running on and bound to ci's CPU.
 *
 *	Actually, caller must have kpreemption disabled, because that's
 *	easier to assert at the moment.
 */
void
xen_resumeclocks(struct cpu_info *ci)
{
	char intr_xname[INTRDEVNAMEBUF];
	int evtch;
	int error;

	KASSERT(ci == curcpu());
	KASSERT(kpreempt_disabled());
	KASSERT(ci->ci_xen_timer_intrhand == NULL);

	evtch = bind_virq_to_evtch(VIRQ_TIMER);
	KASSERT(evtch != -1);

	snprintf(intr_xname, sizeof(intr_xname), "%s clock",
	    device_xname(ci->ci_dev));
	/* XXX sketchy function pointer cast -- fix the API, please */
	ci->ci_xen_timer_intrhand = intr_establish_xname(0, &xen_pic, evtch,
	    IST_LEVEL, IPL_CLOCK, (int (*)(void *))xen_timer_handler, ci, true,
	    intr_xname);
	if (ci->ci_xen_timer_intrhand == NULL)
		panic("failed to establish timer interrupt handler");

	hypervisor_enable_event(evtch);

	aprint_verbose("Xen %s: using event channel %d\n", intr_xname, evtch);

	/* Disarm the periodic timer on Xen>=3.1 which is allegedly buggy.  */
	if (XEN_MAJOR(xen_version) > 3 || XEN_MINOR(xen_version) > 0) {
		error = HYPERVISOR_vcpu_op(VCPUOP_stop_periodic_timer,
		    ci->ci_cpuid, NULL);
		KASSERT(error == 0);
	}

	/* Pretend the last hardclock happened right now.  */
	ci->ci_xen_hardclock_systime_ns = xen_vcputime_systime_ns();

	/* Arm the one-shot timer.  */
	error = HYPERVISOR_set_timer_op(ci->ci_xen_hardclock_systime_ns +
	    NS_PER_TICK);
	KASSERT(error == 0);

	/* We'd better not have switched CPUs.  */
	KASSERT(ci == curcpu());
}

/*
 * xen_timer_handler(cookie, frame)
 *
 *	Periodic Xen timer event handler for NetBSD hardclock.  Calls
 *	to this may get delayed, so we run hardclock as many times as
 *	we need to in order to cover the Xen system time that elapsed.
 *	After that, re-arm the timer to run again at the next tick.
 *	The cookie is the pointer to struct cpu_info.
 */
static int
xen_timer_handler(void *cookie, struct clockframe *frame)
{
	struct cpu_info *ci = curcpu();
	uint64_t last, now, delta, next;
	int error;

	KASSERT(cpu_intr_p());
	KASSERT(cookie == ci);

again:
	/*
	 * Find how many nanoseconds of Xen system time has elapsed
	 * since the last hardclock tick.
	 */
	last = ci->ci_xen_hardclock_systime_ns;
	now = xen_vcputime_systime_ns();
	if (now < last) {
#if XEN_CLOCK_DEBUG		/* XXX dtrace hook */
		printf("xen systime ran backwards in hardclock %"PRIu64"ns\n",
		    last - now);
#endif
		ci->ci_xen_systime_backwards_hardclock_evcnt.ev_count++;
		now = last;
	}
	delta = now - last;

	/*
	 * Play hardclock catchup: run the hardclock timer as many
	 * times as appears necessary based on how much time has
	 * passed.
	 */
	while (delta >= NS_PER_TICK) {
		ci->ci_xen_hardclock_systime_ns += NS_PER_TICK;
		delta -= NS_PER_TICK;
		hardclock(frame);
		if (__predict_false(delta >= NS_PER_TICK))
			ci->ci_xen_missed_hardclock_evcnt.ev_count++;
	}

	/*
	 * Re-arm the timer.  If it fails, it's probably because the
	 * time is in the past, so update our idea of what the Xen
	 * system time is and try again.
	 */
	next = ci->ci_xen_hardclock_systime_ns + NS_PER_TICK;
	error = HYPERVISOR_set_timer_op(next);
	if (error)
		goto again;

	/* Success!  */
	return 0;
}

/*
 * xen_initclocks()
 *
 *	Initialize the Xen clocks on the current CPU.
 */
void
xen_initclocks(void)
{
	struct cpu_info *ci = curcpu();

	/* If this is the primary CPU, do global initialization first.  */
	if (ci == &cpu_info_primary) {
		/* Initialize the systemwide Xen timecounter.  */
		tc_init(&xen_timecounter);

#ifdef DOM0OPS
		/*
		 * If this is a privileged dom0, start pushing the wall
		 * clock time back to the Xen hypervisor.
		 */
		if (xendomain_is_privileged())
			xen_timepush_init();
#endif
	}

	/* Attach the event counters.  */
	evcnt_attach_dynamic(&ci->ci_xen_cpu_tsc_backwards_evcnt,
	    EVCNT_TYPE_INTR, NULL, device_xname(ci->ci_dev),
	    "cpu tsc ran backwards");
	evcnt_attach_dynamic(&ci->ci_xen_tsc_delta_negative_evcnt,
	    EVCNT_TYPE_INTR, NULL, device_xname(ci->ci_dev),
	    "tsc delta went negative");
	evcnt_attach_dynamic(&ci->ci_xen_raw_systime_wraparound_evcnt,
	    EVCNT_TYPE_INTR, NULL, device_xname(ci->ci_dev),
	    "raw systime wrapped around");
	evcnt_attach_dynamic(&ci->ci_xen_raw_systime_backwards_evcnt,
	    EVCNT_TYPE_INTR, NULL, device_xname(ci->ci_dev),
	    "raw systime went backwards");
	evcnt_attach_dynamic(&ci->ci_xen_systime_backwards_hardclock_evcnt,
	    EVCNT_TYPE_INTR, NULL, device_xname(ci->ci_dev),
	    "systime went backwards in hardclock");
	evcnt_attach_dynamic(&ci->ci_xen_missed_hardclock_evcnt,
	    EVCNT_TYPE_INTR, NULL, device_xname(ci->ci_dev),
	    "missed hardclock");

	/* Fire up the clocks.  */
	xen_resumeclocks(ci);
}

#ifdef DOM0OPS

/*
 * xen_timepush_init()
 *
 *	Initialize callout to periodically set Xen hypervisor's wall
 *	clock time.
 */
static void
xen_timepush_init(void)
{
	struct sysctllog *log = NULL;
	const struct sysctlnode *node = NULL;
	int error;

	/* Start periodically updating the hypervisor's wall clock time.  */
	callout_init(&xen_timepush.ch, 0);
	callout_setfunc(&xen_timepush.ch, xen_timepush_intr, NULL);

	/* Pick a default frequency for timepush.  */
	xen_timepush.ticks = 53*hz + 3; /* avoid exact # of min/sec */

	/* Create machdep.xen node.  */
	/* XXX Creation of the `machdep.xen' node should be elsewhere.  */
	error = sysctl_createv(&log, 0, NULL, &node, 0,
	    CTLTYPE_NODE, "xen",
	    SYSCTL_DESCR("Xen top level node"),
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;
	KASSERT(node != NULL);

	/* Create int machdep.xen.timepush_ticks knob.  */
	error = sysctl_createv(&log, 0, NULL, NULL, CTLFLAG_READWRITE,
	    CTLTYPE_INT, "timepush_ticks",
	    SYSCTL_DESCR("How often to update the hypervisor's time-of-day;"
		" 0 to disable"),
	    sysctl_xen_timepush, 0, &xen_timepush.ticks, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto fail;

	/* Start the timepush callout.  */
	callout_schedule(&xen_timepush.ch, xen_timepush.ticks);

	/* Success!  */
	return;

fail:	sysctl_teardown(&log);
}

/*
 * xen_timepush_intr(cookie)
 *
 *	Callout interrupt handler to push NetBSD's idea of the wall
 *	clock time, usually synchronized with NTP, back to the Xen
 *	hypervisor.
 */
static void
xen_timepush_intr(void *cookie)
{

	resettodr();
	if (xen_timepush.ticks)
		callout_schedule(&xen_timepush.ch, xen_timepush.ticks);
}

/*
 * sysctl_xen_timepush(...)
 *
 *	Sysctl handler to set machdep.xen.timepush_ticks.
 */
static int
sysctl_xen_timepush(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int ticks;
	int error;

	ticks = xen_timepush.ticks;
	node = *rnode;
	node.sysctl_data = &ticks;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (ticks < 0)
		return EINVAL;

	if (ticks != xen_timepush.ticks) {
		xen_timepush.ticks = ticks;

		if (ticks == 0)
			callout_stop(&xen_timepush.ch);
		else
			callout_schedule(&xen_timepush.ch, ticks);
	}

	return 0;
}

#endif	/* DOM0OPS */
