/*	$NetBSD: ds1553rtc.c,v 1.1.4.1 2011/06/06 09:05:32 jruoho Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: ds1553rtc.c,v 1.1.4.1 2011/06/06 09:05:32 jruoho Exp $");

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/tty.h>

#include "ioconf.h"

#include <sys/intr.h>
#include <sys/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mk48txxvar.h>

#include <powerpc/booke/cpuvar.h>

static int ds1553rtc_match(device_t, cfdata_t, void *);
static void ds1553rtc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ds1553rtc, sizeof(struct mk48txx_softc),
    ds1553rtc_match, ds1553rtc_attach, NULL, NULL);

static int
ds1553rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct generic_attach_args * const ga = aux;
	bus_space_handle_t bsh;
	int error;
	int rv;
	uint8_t saved_data, new_data;
	bus_size_t size = ga->ga_size;

	if (ga->ga_addr == OBIOCF_ADDR_DEFAULT)
		return 0;
	if (size == OBIOCF_SIZE_DEFAULT)
		size = 8192;

	/*
	 * If this is NVRAM+RTC, make sure we can modify the NVRAM.
	 */
	error = bus_space_map(ga->ga_bst, ga->ga_addr, size, 0, &bsh);
	if (error)
		return 0;

	bus_size_t target = size - 17;
	saved_data = bus_space_read_1(ga->ga_bst, bsh, target);
	bus_space_write_1(ga->ga_bst, bsh, target, ~saved_data);
	new_data = bus_space_read_1(ga->ga_bst, bsh, target);
	rv = (new_data == (uint8_t) ~saved_data);
	bus_space_write_1(ga->ga_bst, bsh, target, saved_data);

	bus_space_unmap(ga->ga_bst, bsh, size);

	return rv;
}

static void
ds1553rtc_attach(device_t parent, device_t self, void *aux)
{
	struct mk48txx_softc * const sc = device_private(self);
	struct generic_attach_args * const ga = aux;
	int error;
	bus_size_t size = ga->ga_size;

	if (size == OBIOCF_SIZE_DEFAULT)
		size = 8192;

	sc->sc_dev = self;
	sc->sc_bst = ga->ga_bst;
	sc->sc_model = "ds1553";
	sc->sc_flag = MK48TXX_HAVE_CENT_REG;
	error = bus_space_map(sc->sc_bst, ga->ga_addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	mk48txx_attach(sc);
	aprint_normal("\n");
}
