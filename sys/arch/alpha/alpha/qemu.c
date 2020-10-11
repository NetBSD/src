/* $NetBSD: qemu.c,v 1.5 2020/10/11 18:39:48 thorpej Exp $ */

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/cpu.h>

#include <machine/autoconf.h>
#include <machine/cpuconf.h>
#include <machine/rpb.h>
#include <machine/alpha.h>

#include <alpha/alpha/clockvar.h>

extern struct cfdriver qemu_cd;

struct qemu_softc {
	device_t	sc_dev;

	struct timecounter sc_tc;
};

static unsigned long qemu_nsec_per_tick __read_mostly = (unsigned long)-1;

static inline unsigned long
qemu_get_vmtime(void)
{
	register unsigned long v0 __asm("$0");
	register unsigned long a0 __asm("$16") = 7;	/* Qemu get-time */

	__asm volatile ("call_pal %2"
		: "=r"(v0), "+r"(a0)
		: "i"(PAL_cserve)
		: "$17", "$18", "$19", "$20", "$21");

	return v0;
}

static void
qemu_delay(unsigned long usec)
{
	/* Get starting point. */
	const unsigned long base = qemu_get_vmtime();

	/* convert request from usec to nsec */
	const unsigned long nsec = usec * 1000;
	KASSERT(nsec > usec);

	/* Figure out finish line. */
	const unsigned long finished = base + nsec;
	KASSERT(finished > base);

	unsigned long now;

	/* Spin until we're finished. */
	while ((now = qemu_get_vmtime()) < finished) {
		/*
		 * If we have more than one clock tick worth of spinning
		 * to do, when use WTINT to wait at a low power state.
		 */
		if (finished - now > qemu_nsec_per_tick) {
			alpha_pal_wtint(0);
		}
	}
}

static u_int
qemu_get_timecount(struct timecounter * const tc __unused)
{
	return (u_int)qemu_get_vmtime();
}

static inline void
qemu_set_alarm_relative(unsigned long nsec)
{
	register unsigned long a0 __asm("$16") = 5;	/* Qemu set-alarm-rel */
	register unsigned long a1 __asm("$17") = nsec;

	__asm volatile ("call_pal %2"
		: "+r"(a0), "+r"(a1)
		: "i"(PAL_cserve)
		: "$0", "$18", "$19", "$20", "$21");
}

static void
qemu_hardclock(struct clockframe * const framep)
{
	if (__predict_false(qemu_nsec_per_tick == (unsigned long)-1)) {
		/* Spurious; qemu_clock_init() hasn't been called yet. */
		return;
	}

	/* Schedule the next tick before we process the current one. */
	qemu_set_alarm_relative(qemu_nsec_per_tick);

	hardclock(framep);
}

static void
qemu_clock_init(void * const v __unused)
{
	/* First-time initialization... */
	if (qemu_nsec_per_tick == (unsigned long)-1) {
		KASSERT(CPU_IS_PRIMARY(curcpu()));

		/*
		 * Override the clockintr routine; the Qemu alarm is
		 * one-shot, so we have to restart it for the next one.
		 */
		platform.clockintr = qemu_hardclock;

		/*
		 * hz=1024 is a little bananas for an emulated
		 * virtual machine.  Let MI code drive schedhz.
		 */
		hz = 50;
		schedhz = 0;

		qemu_nsec_per_tick = 1000000000UL / hz;

		printf("Using the Qemu CPU alarm for %d Hz hardclock.\n", hz);
	}

	/*
	 * Note: We need to do this on each CPU, as the Qemu
	 * alarm is implemented as a per-CPU register.
	 */
	qemu_set_alarm_relative(qemu_nsec_per_tick);
}

static int
qemu_match(device_t parent, cfdata_t cfdata, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (strcmp(ma->ma_name, qemu_cd.cd_name) != 0)
		return (0);

	return (1);
}

static void
qemu_attach(device_t parent, device_t self, void *aux)
{
	struct qemu_softc * const sc = device_private(self);
	struct timecounter * const tc = &sc->sc_tc;

	sc->sc_dev = self;

	aprint_normal(": Qemu virtual machine services\n");
	aprint_naive("\n");

	/*
	 * Use the Qemu "VM time" hypercall as the system timecounter.
	 */
	tc->tc_name = "Qemu";
	tc->tc_get_timecount = qemu_get_timecount;
	tc->tc_quality = 3000;
	tc->tc_counter_mask = __BITS(0,31);
	tc->tc_frequency = 1000000000UL;	/* nanosecond granularity */
	tc->tc_priv = sc;
	tc_init(tc);

	/*
	 * Use the Qemu alarm as the system clock.
	 */
	clockattach(qemu_clock_init, sc);

	/*
	 * Qemu's PALcode implements WTINT; use it to save host cycles.
	 */
	cpu_idle_fn = cpu_idle_wtint;

	/*
	 * Use Qemu's "VM time" hypercall to implement delay().
	 */
	alpha_delay_fn = qemu_delay;
}

CFATTACH_DECL_NEW(qemu, sizeof(struct qemu_softc),
    qemu_match, qemu_attach, NULL, NULL);
