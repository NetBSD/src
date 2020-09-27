/* $NetBSD: qemu.c,v 1.1 2020/09/27 23:59:37 thorpej Exp $ */

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
#include <sys/cpu.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>
#include <machine/alpha.h>

extern struct cfdriver qemu_cd;

struct qemu_softc {
	device_t	sc_dev;

	struct timecounter sc_tc;
};

static u_int
qemu_get_timecount(struct timecounter * const tc __unused)
{
	register unsigned long v0 __asm("$0");
	register unsigned long a0 __asm("$16") = 7;	/* Qemu get-time */
	
	__asm volatile ("call_pal %2"
		: "=r"(v0), "+r"(a0)
		: "i"(PAL_cserve)
		: "$17", "$18", "$19", "$20", "$21");

	return (u_int)v0;
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
}

CFATTACH_DECL_NEW(qemu, sizeof(struct qemu_softc),
    qemu_match, qemu_attach, NULL, NULL);
