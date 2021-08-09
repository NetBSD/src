/*	$NetBSD: pcfiic_ebus.c,v 1.7.16.1 2021/08/09 00:30:08 thorpej Exp $	*/
/*	$OpenBSD: pcfiic_ebus.c,v 1.13 2008/06/08 03:07:40 deraadt Exp $ */

/*
 * Copyright (c) 2021 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 2006 David Gwynne <dlg@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcfiic_ebus.c,v 1.7.16.1 2021/08/09 00:30:08 thorpej Exp $");

/*
 * Device specific driver for the EBus i2c devices found on some sun4u
 * systems. On systems not having a boot-bus controller the i2c devices
 * are PCF8584.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <sys/bus.h>
#include <machine/openfirm.h>
#include <machine/autoconf.h>

#include <dev/ebus/ebusreg.h>
#include <dev/ebus/ebusvar.h>

#include <dev/i2c/i2cvar.h>

#include <dev/ic/pcf8584var.h>
#include <dev/ic/pcf8584reg.h>

struct pcfiic_ebus_softc {
	struct pcfiic_softc	esc_sc;

	kmutex_t		esc_ctrl_lock;
	bus_space_handle_t	esc_ioh;	/* for channel selection */

	void			*esc_ih;
};

static void
bbc_select_channel(struct pcfiic_ebus_softc *esc, uint8_t channel)
{
	bus_space_write_1(esc->esc_sc.sc_iot, esc->esc_ioh, 0, channel);
	bus_space_barrier(esc->esc_sc.sc_iot, esc->esc_ioh, 0, 1,
	    BUS_SPACE_BARRIER_WRITE);
}

static int
bbc_acquire_bus(void *v, int flags)
{
	struct pcfiic_channel *ch = v;
	struct pcfiic_ebus_softc *esc = container_of(ch->ch_sc,
	    struct pcfiic_ebus_softc, esc_sc);

	if (flags & I2C_F_POLL) {
		if (! mutex_tryenter(&esc->esc_ctrl_lock)) {
			return EBUSY;
		}
	} else {
		mutex_enter(&esc->esc_ctrl_lock);
	}

	bbc_select_channel(esc, (uint8_t)ch->ch_channel);
	return 0;
}

static void
bbc_release_bus(void *v, int flags)
{
	struct pcfiic_channel *ch = v;
	struct pcfiic_ebus_softc *esc = container_of(ch->ch_sc,
	    struct pcfiic_ebus_softc, esc_sc);

	mutex_exit(&esc->esc_ctrl_lock);
}

static void
bbc_initialize_channels(struct pcfiic_ebus_softc *esc)
{
	struct pcfiic_softc *sc = &esc->esc_sc;
	struct pcfiic_channel *ch;
	devhandle_t devhandle = device_handle(sc->sc_dev);
	unsigned int busmap = 0;
	int node = devhandle_to_of(devhandle);
	uint32_t reg[2];
	uint32_t channel;
	int i, nchannels;

	/*
	 * Two physical I2C busses share a single controller.  The
	 * devices are not distinct, so it's not easy to treat it
	 * it as a mux.
	 *
	 * The locking order is:
	 *
	 *	iic bus mutex -> ctrl_lock
	 *
	 * ctrl_lock is taken in bbc_acquire_bus.
	 */
	mutex_init(&esc->esc_ctrl_lock, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_acquire_bus = bbc_acquire_bus;
	sc->sc_release_bus = bbc_release_bus;

	/*
	 * The Sun device tree has all devices, no matter the
	 * channel, as direct children of this node.  Figure
	 * out which channel numbers are listed, count them,
	 * and then populate the channel structures.
	 */
	for (node = OF_child(node); node != 0; node = OF_peer(node)) {
		if (OF_getprop(node, "reg", reg, sizeof(reg)) != sizeof(reg)) {
			continue;
		}

		/* Channel is in the first cell. */
		channel = be32toh(reg[0]);
		KASSERT(channel < 32);

		busmap |= __BIT(channel);
	}

	nchannels = popcount(busmap);
	if (nchannels == 0) {
		/* No child devices. */
		return;
	}

	ch = kmem_alloc(nchannels * sizeof(*ch), KM_SLEEP);
	for (i = 0; i < nchannels; i++) {
		channel = ffs(busmap);
		KASSERT(channel != 0);
		channel--;	/* ffs() returns 0 if no bits set. */
		busmap &= ~__BIT(channel);

		ch[i].ch_channel = channel;
		ch[i].ch_devhandle = devhandle;
	}

	sc->sc_channels = ch;
	sc->sc_nchannels = nchannels;
}

static int
pcfiic_ebus_match(device_t parent, struct cfdata *match, void *aux)
{
	struct ebus_attach_args		*ea = aux;
	char				compat[32];

	if (strcmp(ea->ea_name, "SUNW,envctrl") == 0 ||
	    strcmp(ea->ea_name, "SUNW,envctrltwo") == 0)
		return (1);

	if (strcmp(ea->ea_name, "i2c") != 0)
		return (0);

	if (OF_getprop(ea->ea_node, "compatible", compat, sizeof(compat)) == -1)
		return (0);

	if (strcmp(compat, "pcf8584") == 0 ||
	    strcmp(compat, "i2cpcf,8584") == 0 ||
	    strcmp(compat, "SUNW,i2c-pic16f747") == 0 ||
	    strcmp(compat, "SUNW,bbc-i2c") == 0)
		return (1);

	return (0);
}

static void
pcfiic_ebus_attach(device_t parent, device_t self, void *aux)
{
	struct pcfiic_ebus_softc	*esc = device_private(self);
	struct pcfiic_softc		*sc = &esc->esc_sc;
	struct ebus_attach_args		*ea = aux;
	char				compat[32];
	uint32_t			addr[2];
	uint8_t				clock = PCF8584_CLK_12 | PCF8584_SCL_90;
	int				swapregs = 0;

	if (ea->ea_nreg < 1 || ea->ea_nreg > 2) {
		printf(": expected 1 or 2 registers, got %d\n", ea->ea_nreg);
		return;
	}

	/* E450 and E250 have a different clock */
	if ((strcmp(ea->ea_name, "SUNW,envctrl") == 0) ||
	    (strcmp(ea->ea_name, "SUNW,envctrltwo") == 0))
		clock = PCF8584_CLK_12 | PCF8584_SCL_45;

	sc->sc_dev = self;
	if (OF_getprop(ea->ea_node, "compatible", compat, sizeof(compat)) > 0 &&
	    strcmp(compat, "SUNW,bbc-i2c") == 0) {
		/*
		 * On BBC-based machines, Sun swapped the order of
		 * the registers on their clone pcf, plus they feed
		 * it a non-standard clock.
		 */
		int clk = prom_getpropint(findroot(), "clock-frequency", 0);

		if (clk < 105000000)
			clock = PCF8584_CLK_3 | PCF8584_SCL_90;
		else if (clk < 160000000)
			clock = PCF8584_CLK_4_43 | PCF8584_SCL_90;
		swapregs = 1;
	}

	if (OF_getprop(ea->ea_node, "own-address", &addr, sizeof(addr)) == -1) {
		addr[0] = 0;
		addr[1] = 0x55 << 1;
	} else if (addr[1] == 0x00 || addr[1] > 0xff) {
		printf(": invalid address on I2C bus");
		return;
	}

	if (bus_space_map(ea->ea_bustag, EBUS_ADDR_FROM_REG(&ea->ea_reg[0]),
			  ea->ea_reg[0].size, 0, &sc->sc_ioh) == 0) {
		sc->sc_iot = ea->ea_bustag;
	} else {
		printf(": can't map register space\n");
               	return;
	}

	if (ea->ea_nreg == 2) {
		/*
		 * Second register only occurs on BBC-based machines,
		 * and is likely not prom mapped
		 */
		if (bus_space_map(sc->sc_iot,
				  EBUS_ADDR_FROM_REG(&ea->ea_reg[1]),
				  ea->ea_reg[1].size, 0, &esc->esc_ioh) != 0) {
			printf(": can't map 2nd register space\n");
			return;
		}
		bbc_initialize_channels(esc);
	}

	if (ea->ea_nintr >= 1)
		esc->esc_ih = bus_intr_establish(sc->sc_iot, ea->ea_intr[0],
		    IPL_BIO, pcfiic_intr, sc);
	else
		esc->esc_ih = NULL;

	if (esc->esc_ih == NULL)
		sc->sc_poll = 1;

	pcfiic_attach(sc, (i2c_addr_t)(addr[1] >> 1), clock, swapregs);
}

CFATTACH_DECL_NEW(pcfiic, sizeof(struct pcfiic_ebus_softc),
    pcfiic_ebus_match, pcfiic_ebus_attach, NULL, NULL);
