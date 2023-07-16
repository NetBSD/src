/*	$NetBSD: kern_heartbeat.c,v 1.5 2023/07/16 10:18:19 riastradh Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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
 * heartbeat(9) -- periodic checks to ensure CPUs are making progress
 *
 * Manual tests to run when changing this file.  Magic numbers are for
 * evbarm; adjust for other platforms.  Tests involving cpuctl
 * online/offline assume a 2-CPU system -- for full testing on a >2-CPU
 * system, offline all but one CPU.
 *
 * 1.	cpuctl offline 0
 *	sleep 20
 *	cpuctl online 0
 *
 * 2.	cpuctl offline 1
 *	sleep 20
 *	cpuctl online 1
 *
 * 3.	cpuctl offline 0
 *	sysctl -w kern.heartbeat.max_period=5
 *	sleep 10
 *	sysctl -w kern.heartbeat.max_period=0
 *	sleep 10
 *	sysctl -w kern.heartbeat.max_period=5
 *	sleep 10
 *	cpuctl online 0
 *
 * 4.	sysctl -w debug.crashme_enable=1
 *	sysctl -w debug.crashme.spl_spinout=1   # IPL_SOFTCLOCK
 *	# verify system panics after 15sec
 *
 * 5.	sysctl -w debug.crashme_enable=1
 *	sysctl -w debug.crashme.spl_spinout=6   # IPL_SCHED
 *	# verify system panics after 15sec
 *
 * 6.	cpuctl offline 0
 *	sysctl -w debug.crashme_enable=1
 *	sysctl -w debug.crashme.spl_spinout=1   # IPL_SOFTCLOCK
 *	# verify system panics after 15sec
 *
 * 7.	cpuctl offline 0
 *	sysctl -w debug.crashme_enable=1
 *	sysctl -w debug.crashme.spl_spinout=5   # IPL_VM
 *	# verify system panics after 15sec
 *
 *	# Not this -- IPL_SCHED and IPL_HIGH spinout on a single CPU
 *	# require a hardware watchdog timer.
 *	#cpuctl offline 0
 *	#sysctl -w debug.crashme_enable
 *	#sysctl -w debug.crashme.spl_spinout=6   # IPL_SCHED
 *	# hope watchdog timer kicks in
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kern_heartbeat.c,v 1.5 2023/07/16 10:18:19 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "opt_heartbeat.h"
#endif

#include "heartbeat.h"

#include <sys/param.h>
#include <sys/types.h>

#include <sys/atomic.h>
#include <sys/cpu.h>
#include <sys/errno.h>
#include <sys/heartbeat.h>
#include <sys/ipi.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/xcall.h>

#ifdef DDB
#include <ddb/ddb.h>
#endif

/*
 * Global state.
 *
 *	heartbeat_lock serializes access to heartbeat_max_period_secs
 *	and heartbeat_max_period_ticks.  Two separate variables so we
 *	can avoid multiplication or division in the heartbeat routine.
 *
 *	heartbeat_sih is stable after initialization in
 *	heartbeat_start.
 */
kmutex_t heartbeat_lock			__cacheline_aligned;
unsigned heartbeat_max_period_secs	__read_mostly;
unsigned heartbeat_max_period_ticks	__read_mostly;

void *heartbeat_sih			__read_mostly;

/*
 * heartbeat_suspend()
 *
 *	Suspend heartbeat monitoring of the current CPU.
 *
 *	Called after the current CPU has been marked offline but before
 *	it has stopped running.  Caller must have preemption disabled.
 */
void
heartbeat_suspend(void)
{

	KASSERT(curcpu_stable());

	/*
	 * Nothing to do -- we just check the SPCF_OFFLINE flag.
	 */
}

/*
 * heartbeat_resume_cpu(ci)
 *
 *	Resume heartbeat monitoring of ci.
 *
 *	Called at startup while cold, and whenever heartbeat monitoring
 *	is re-enabled after being disabled or the period is changed.
 *	When not cold, ci must be the current CPU.
 */
static void
heartbeat_resume_cpu(struct cpu_info *ci)
{

	KASSERT(__predict_false(cold) || curcpu_stable());
	KASSERT(__predict_false(cold) || ci == curcpu());

	ci->ci_heartbeat_count = 0;
	ci->ci_heartbeat_uptime_cache = time_uptime;
	ci->ci_heartbeat_uptime_stamp = 0;
}

/*
 * heartbeat_resume()
 *
 *	Resume heartbeat monitoring of the current CPU.
 *
 *	Called after the current CPU has started running but before it
 *	has been marked online.  Also used internally when starting up
 *	heartbeat monitoring at boot or when the maximum period is set
 *	from zero to nonzero.  Caller must have preemption disabled.
 */
void
heartbeat_resume(void)
{
	struct cpu_info *ci = curcpu();
	int s;

	KASSERT(curcpu_stable());

	/*
	 * Block heartbeats while we reset the state so we don't
	 * spuriously think we had a heart attack in the middle of
	 * resetting the count and the uptime stamp.
	 */
	s = splsched();
	heartbeat_resume_cpu(ci);
	splx(s);
}

/*
 * heartbeat_reset_xc(a, b)
 *
 *	Cross-call handler to reset heartbeat state just prior to
 *	enabling heartbeat checks.
 */
static void
heartbeat_reset_xc(void *a, void *b)
{

	heartbeat_resume();
}

/*
 * set_max_period(max_period)
 *
 *	Set the maximum period, in seconds, for heartbeat checks.
 *
 *	- If max_period is zero, disable them.
 *
 *	- If the max period was zero and max_period is nonzero, ensure
 *	  all CPUs' heartbeat uptime caches are up-to-date before
 *	  re-enabling them.
 *
 *	max_period must be below UINT_MAX/4/hz to avoid arithmetic
 *	overflow and give room for slop.
 *
 *	Caller must hold heartbeat_lock.
 */
static void
set_max_period(unsigned max_period)
{

	KASSERTMSG(max_period <= UINT_MAX/4/hz,
	    "max_period=%u must not exceed UINT_MAX/4/hz=%u (hz=%u)",
	    max_period, UINT_MAX/4/hz, hz);
	KASSERT(mutex_owned(&heartbeat_lock));

	/*
	 * If we're enabling heartbeat checks, make sure we have a
	 * reasonably up-to-date time_uptime cache on all CPUs so we
	 * don't think we had an instant heart attack.
	 */
	if (heartbeat_max_period_secs == 0 && max_period != 0) {
		if (cold) {
			CPU_INFO_ITERATOR cii;
			struct cpu_info *ci;

			for (CPU_INFO_FOREACH(cii, ci))
				heartbeat_resume_cpu(ci);
		} else {
			const uint64_t ticket =
			    xc_broadcast(0, &heartbeat_reset_xc, NULL, NULL);
			xc_wait(ticket);
		}
	}

	/*
	 * Once the heartbeat state has been updated on all (online)
	 * CPUs, set the period.  At this point, heartbeat checks can
	 * begin.
	 */
	atomic_store_relaxed(&heartbeat_max_period_secs, max_period);
	atomic_store_relaxed(&heartbeat_max_period_ticks, max_period*hz);
}

/*
 * heartbeat_max_period_ticks(SYSCTLFN_ARGS)
 *
 *	Sysctl handler for sysctl kern.heartbeat.max_period.  Verifies
 *	it lies within a reasonable interval and sets it.
 */
static int
heartbeat_max_period_sysctl(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	unsigned max_period;
	int error;

	mutex_enter(&heartbeat_lock);

	max_period = heartbeat_max_period_secs;
	node = *rnode;
	node.sysctl_data = &max_period;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto out;

	/*
	 * Ensure there's plenty of slop between heartbeats.
	 */
	if (max_period > UINT_MAX/4/hz) {
		error = EOVERFLOW;
		goto out;
	}

	/*
	 * Success!  Set the period.  This enables heartbeat checks if
	 * we went from zero period to nonzero period, or disables them
	 * if the other way around.
	 */
	set_max_period(max_period);
	error = 0;

out:	mutex_exit(&heartbeat_lock);
	return error;
}

/*
 * sysctl_heartbeat_setup()
 *
 *	Set up the kern.heartbeat.* sysctl subtree.
 */
SYSCTL_SETUP(sysctl_heartbeat_setup, "sysctl kern.heartbeat setup")
{
	const struct sysctlnode *rnode;
	int error;

	mutex_init(&heartbeat_lock, MUTEX_DEFAULT, IPL_NONE);

	/* kern.heartbeat */
	error = sysctl_createv(NULL, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "heartbeat",
	    SYSCTL_DESCR("Kernel heartbeat parameters"),
	    NULL, 0, NULL, 0,
	    CTL_KERN, CTL_CREATE, CTL_EOL);
	if (error) {
		printf("%s: failed to create kern.heartbeat: %d\n",
		    __func__, error);
		return;
	}

	/* kern.heartbeat.max_period */
	error = sysctl_createv(NULL, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "max_period",
	    SYSCTL_DESCR("Max seconds between heartbeats before panic"),
	    &heartbeat_max_period_sysctl, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error) {
		printf("%s: failed to create kern.heartbeat.max_period: %d\n",
		    __func__, error);
		return;
	}
}

/*
 * heartbeat_intr(cookie)
 *
 *	Soft interrupt handler to update the local CPU's view of the
 *	system uptime.  This runs at the same priority level as
 *	callouts, so if callouts are stuck on this CPU, it won't run,
 *	and eventually another CPU will notice that this one is stuck.
 *
 *	Don't do spl* here -- keep it to a minimum so if anything goes
 *	wrong we don't end up with hard interrupts blocked and unable
 *	to detect a missed heartbeat.
 */
static void
heartbeat_intr(void *cookie)
{
	unsigned count = atomic_load_relaxed(&curcpu()->ci_heartbeat_count);
	unsigned uptime = time_uptime;

	atomic_store_relaxed(&curcpu()->ci_heartbeat_uptime_stamp, count);
	atomic_store_relaxed(&curcpu()->ci_heartbeat_uptime_cache, uptime);
}

/*
 * heartbeat_start()
 *
 *	Start system heartbeat monitoring.
 */
void
heartbeat_start(void)
{
	const unsigned max_period = HEARTBEAT_MAX_PERIOD_DEFAULT;

	/*
	 * Establish a softint so we can schedule it once ready.  This
	 * should be at the lowest softint priority level so that we
	 * ensure all softint priorities are making progress.
	 */
	heartbeat_sih = softint_establish(SOFTINT_CLOCK|SOFTINT_MPSAFE,
	    &heartbeat_intr, NULL);

	/*
	 * Now that the softint is established, kick off heartbeat
	 * monitoring with the default period.  This will initialize
	 * the per-CPU state to an up-to-date cache of time_uptime.
	 */
	mutex_enter(&heartbeat_lock);
	set_max_period(max_period);
	mutex_exit(&heartbeat_lock);
}

/*
 * defibrillator(cookie)
 *
 *	IPI handler for defibrillation.  If the CPU's heart has stopped
 *	beating normally, but the CPU can still execute things,
 *	acknowledge the IPI to the doctor and then panic so we at least
 *	get a stack trace from whatever the current CPU is stuck doing,
 *	if not a core dump.
 *
 *	(This metaphor is a little stretched, since defibrillation is
 *	usually administered when the heart is beating errattically but
 *	hasn't stopped, and causes the heart to stop temporarily, and
 *	one hopes it is not fatal.  But we're (software) engineers, so
 *	we can stretch metaphors like silly putty in a blender.)
 */
static void
defibrillator(void *cookie)
{
	bool *ack = cookie;

	atomic_store_relaxed(ack, true);
	panic("%s[%d %s]: heart stopped beating", cpu_name(curcpu()),
	    curlwp->l_lid,
	    curlwp->l_name ? curlwp->l_name : curproc->p_comm);
}

/*
 * defibrillate(ci, unsigned d)
 *
 *	The patient CPU ci's heart has stopped beating after d seconds.
 *	Force the patient CPU ci to panic, or panic on this CPU if the
 *	patient CPU doesn't respond within 1sec.
 */
static void __noinline
defibrillate(struct cpu_info *ci, unsigned d)
{
	bool ack = false;
	ipi_msg_t msg = {
		.func = &defibrillator,
		.arg = &ack,
	};
	unsigned countdown = 1000; /* 1sec */

	KASSERT(curcpu_stable());

	/*
	 * First notify the console that the patient CPU's heart seems
	 * to have stopped beating.
	 */
	printf("%s: found %s heart stopped beating after %u seconds\n",
	    cpu_name(curcpu()), cpu_name(ci), d);

	/*
	 * Next, give the patient CPU a chance to panic, so we get a
	 * stack trace on that CPU even if we don't get a crash dump.
	 */
	ipi_unicast(&msg, ci);

	/*
	 * Busy-wait up to 1sec for the patient CPU to print a stack
	 * trace and panic.  If the patient CPU acknowledges the IPI,
	 * or if we're panicking anyway, just give up and stop here --
	 * the system is coming down soon and we should avoid getting
	 * in the way.
	 */
	while (countdown --> 0) {
		if (atomic_load_relaxed(&ack) ||
		    atomic_load_relaxed(&panicstr) != NULL)
			return;
		DELAY(1000);	/* 1ms */
	}

	/*
	 * The patient CPU failed to acknowledge the panic request.
	 * Panic now; with any luck, we'll get a crash dump.
	 */
	panic("%s: found %s heart stopped beating and unresponsive",
	    cpu_name(curcpu()), cpu_name(ci));
}

/*
 * select_patient()
 *
 *	Select another CPU to check the heartbeat of.  Returns NULL if
 *	there are no other online CPUs.  Never returns curcpu().
 *	Caller must have kpreemption disabled.
 */
static struct cpu_info *
select_patient(void)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *first = NULL, *patient = NULL, *ci;
	bool passedcur = false;

	KASSERT(curcpu_stable());

	/*
	 * In the iteration order of all CPUs, find the next online CPU
	 * after curcpu(), or the first online one if curcpu() is last
	 * in the iteration order.
	 */
	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_schedstate.spc_flags & SPCF_OFFLINE)
			continue;
		if (passedcur) {
			/*
			 * (...|curcpu()|ci|...)
			 *
			 * Found the patient right after curcpu().
			 */
			KASSERT(patient != ci);
			patient = ci;
			break;
		}
		if (ci == curcpu()) {
			/*
			 * (...|prev|ci=curcpu()|next|...)
			 *
			 * Note that we want next (or first, if there's
			 * nothing after curcpu()).
			 */
			passedcur = true;
			continue;
		}
		if (first == NULL) {
			/*
			 * (ci|...|curcpu()|...)
			 *
			 * Record ci as first in case there's nothing
			 * after curcpu().
			 */
			first = ci;
			continue;
		}
	}

	/*
	 * If we hit the end, wrap around to the beginning.
	 */
	if (patient == NULL) {
		KASSERT(passedcur);
		patient = first;
	}

	return patient;
}

/*
 * heartbeat()
 *
 *	1. Count a heartbeat on the local CPU.
 *
 *	2. Panic if the system uptime doesn't seem to have advanced in
 *	   a while.
 *
 *	3. Panic if the soft interrupt on this CPU hasn't advanced the
 *	   local view of the system uptime.
 *
 *	4. Schedule the soft interrupt to advance the local view of the
 *	   system uptime.
 *
 *	5. Select another CPU to check the heartbeat of.
 *
 *	6. Panic if the other CPU hasn't advanced its view of the
 *	   system uptime in a while.
 */
void
heartbeat(void)
{
	unsigned period_ticks, period_secs;
	unsigned count, uptime, cache, stamp, d;
	struct cpu_info *patient;

	KASSERT(curcpu_stable());

	period_ticks = atomic_load_relaxed(&heartbeat_max_period_ticks);
	period_secs = atomic_load_relaxed(&heartbeat_max_period_secs);
	if (__predict_false(period_ticks == 0) ||
	    __predict_false(period_secs == 0) ||
	    __predict_false(curcpu()->ci_schedstate.spc_flags & SPCF_OFFLINE))
		return;

	/*
	 * Count a heartbeat on this CPU.
	 */
	count = curcpu()->ci_heartbeat_count++;

	/*
	 * If the uptime hasn't changed, make sure that we haven't
	 * counted too many of our own heartbeats since the uptime last
	 * changed, and stop here -- we only do the cross-CPU work once
	 * per second.
	 */
	uptime = time_uptime;
	cache = atomic_load_relaxed(&curcpu()->ci_heartbeat_uptime_cache);
	if (__predict_true(cache == uptime)) {
		/*
		 * Timecounter hasn't advanced by more than a second.
		 * Make sure the timecounter isn't stuck according to
		 * our heartbeats.
		 *
		 * Our own heartbeat count can't roll back, and
		 * time_uptime should be updated before it wraps
		 * around, so d should never go negative; hence no
		 * check for d < UINT_MAX/2.
		 */
		stamp =
		    atomic_load_relaxed(&curcpu()->ci_heartbeat_uptime_stamp);
		d = count - stamp;
		if (__predict_false(d > period_ticks)) {
			panic("%s: time has not advanced in %u heartbeats",
			    cpu_name(curcpu()), d);
		}
		return;
	}

	/*
	 * If the uptime has changed, make sure that it hasn't changed
	 * so much that softints must be stuck on this CPU.  Since
	 * time_uptime is monotonic, this can't go negative, hence no
	 * check for d < UINT_MAX/2.
	 *
	 * This uses the hard timer interrupt handler on the current
	 * CPU to ensure soft interrupts at all priority levels have
	 * made progress.
	 */
	d = uptime - cache;
	if (__predict_false(d > period_secs)) {
		panic("%s: softints stuck for %u seconds",
		    cpu_name(curcpu()), d);
	}

	/*
	 * Schedule a softint to update our cache of the system uptime
	 * so the next call to heartbeat, on this or another CPU, can
	 * detect progress on this one.
	 */
	softint_schedule(heartbeat_sih);

	/*
	 * Select a patient to check the heartbeat of.  If there's no
	 * other online CPU, nothing to do.
	 */
	patient = select_patient();
	if (patient == NULL)
		return;

	/*
	 * Verify that time is advancing on the patient CPU.  If the
	 * delta exceeds UINT_MAX/2, that means it is already ahead by
	 * a little on the other CPU, and the subtraction went
	 * negative, which is OK.  If the CPU has been
	 * offlined since we selected it, no worries.
	 *
	 * This uses the current CPU to ensure the other CPU has made
	 * progress, even if the other CPU's hard timer interrupt
	 * handler is stuck for some reason.
	 *
	 * XXX Maybe confirm it hasn't gone negative by more than
	 * max_period?
	 */
	d = uptime - atomic_load_relaxed(&patient->ci_heartbeat_uptime_cache);
	if (__predict_false(d > period_secs) &&
	    __predict_false(d < UINT_MAX/2) &&
	    ((patient->ci_schedstate.spc_flags & SPCF_OFFLINE) == 0))
		defibrillate(patient, d);
}

/*
 * heartbeat_dump()
 *
 *	Print the heartbeat data of all CPUs.  Can be called from ddb.
 */
#ifdef DDB
static unsigned
db_read_unsigned(const unsigned *p)
{
	unsigned x;

	db_read_bytes((db_addr_t)p, sizeof(x), (char *)&x);

	return x;
}

void
heartbeat_dump(void)
{
	struct cpu_info *ci;

	db_printf("Heartbeats:\n");
	for (ci = db_cpu_first(); ci != NULL; ci = db_cpu_next(ci)) {
		db_printf("cpu%u: count %u uptime %u stamp %u\n",
		    db_read_unsigned(&ci->ci_index),
		    db_read_unsigned(&ci->ci_heartbeat_count),
		    db_read_unsigned(&ci->ci_heartbeat_uptime_cache),
		    db_read_unsigned(&ci->ci_heartbeat_uptime_stamp));
	}
}
#endif
