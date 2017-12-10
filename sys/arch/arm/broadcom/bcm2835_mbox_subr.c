/*	$NetBSD: bcm2835_mbox_subr.c,v 1.5 2017/12/10 21:38:26 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_mbox_subr.c,v 1.5 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_mboxreg.h>
#include <arm/broadcom/bcm2835reg.h>

void
bcm2835_mbox_read(bus_space_tag_t iot, bus_space_handle_t ioh, uint8_t chan,
    uint32_t *data)
{
	uint32_t mbox;

	KASSERT((chan & 0xf) == chan);

	for (;;) {
		uint8_t rchan;
		uint32_t rdata;

		bus_space_barrier(iot, ioh, 0, BCM2835_MBOX_SIZE,
		    BUS_SPACE_BARRIER_READ);

		if ((bus_space_read_4(iot, ioh,
		    BCM2835_MBOX0_STATUS) & BCM2835_MBOX_STATUS_EMPTY) != 0)
			continue;

		mbox = bus_space_read_4(iot, ioh, BCM2835_MBOX0_READ);

		rchan = BCM2835_MBOX_CHAN(mbox);
		rdata = BCM2835_MBOX_DATA(mbox);

		if (rchan == chan) {
			*data = rdata;
			return;
		}
	}
}

void
bcm2835_mbox_write(bus_space_tag_t iot, bus_space_handle_t ioh, uint8_t chan,
    uint32_t data)
{
	uint32_t rdata;

	KASSERT((chan & 0xf) == chan);
	KASSERT((data & 0xf) == 0);
	for (;;) {

		bus_space_barrier(iot, ioh, 0, BCM2835_MBOX_SIZE,
		    BUS_SPACE_BARRIER_READ);

		if ((rdata = bus_space_read_4(iot, ioh,
		    BCM2835_MBOX0_STATUS) & BCM2835_MBOX_STATUS_FULL) == 0)
			break;
	}

	bus_space_write_4(iot, ioh, BCM2835_MBOX1_WRITE,
	    BCM2835_MBOX_MSG(chan, data));

	bus_space_barrier(iot, ioh, 0, BCM2835_MBOX_SIZE,
	    BUS_SPACE_BARRIER_WRITE);
}
