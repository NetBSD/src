/*	$NetBSD: virtctrl.c,v 1.1 2024/01/02 18:11:44 thorpej Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: virtctrl.c,v 1.1 2024/01/02 18:11:44 thorpej Exp $");

#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/reboot.h>
#include <sys/systm.h>

#include <machine/cpu.h>

#include <virt68k/dev/mainbusvar.h>

struct virtctrl_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "netbsd,qemu-virt-ctrl" },
	DEVICE_COMPAT_EOL
};

#define	VIRTCTRL_REG_FEATURES	0x00
#define	VIRTCTRL_REG_CMD	0x04

#define	CMD_NOP			0
#define	CMD_RESET		1
#define	CMD_HALT		2
#define	CMD_PANIC		3

#define	REG_READ(sc, r)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (r))
#define	REG_WRITE(sc, r, v)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (r))

static void
virtctrl_reset_handler(void *v, int howto)
{
	struct virtctrl_softc * const sc = v;

	if (panicstr != NULL) {
		/*
		 * XXX N.B. This forces a shutdown, rather than
		 * XXX a reboot.
		 */
		REG_WRITE(sc, VIRTCTRL_REG_CMD, CMD_PANIC);
	} else if (howto & RB_HALT) {
		REG_WRITE(sc, VIRTCTRL_REG_CMD, CMD_HALT);
	} else {
		REG_WRITE(sc, VIRTCTRL_REG_CMD, CMD_RESET);
	}
}

static int
virtctrl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args * const ma = aux;

	return mainbus_compatible_match(ma, compat_data);
}

static void
virtctrl_attach(device_t parent, device_t self, void *aux)
{
	struct virtctrl_softc * const sc = device_private(self);
	struct mainbus_attach_args * const ma = aux;
	uint32_t feat;

	sc->sc_dev = self;
	sc->sc_bst = ma->ma_st;
	if (bus_space_map(sc->sc_bst, ma->ma_addr, ma->ma_size, 0,
			  &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Qemu Virtual System Controller\n");

	feat = REG_READ(sc, VIRTCTRL_REG_FEATURES);
	aprint_normal_dev(self, "features=0x%08x\n", feat);

	cpu_set_reset_func(virtctrl_reset_handler, sc);
}

CFATTACH_DECL_NEW(virtctrl, sizeof(struct virtctrl_softc),
	virtctrl_match, virtctrl_attach, NULL, NULL);
