/*	$NetBSD: com_frodo.c,v 1.8.44.1 2014/08/20 00:03:00 tls Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)com.c	7.5 (Berkeley) 5/16/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_frodo.c,v 1.8.44.1 2014/08/20 00:03:00 tls Exp $");

#include "sti_sgc.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>
#include <sys/ttydefaults.h>

#include <machine/bus.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <hp300/dev/intiovar.h>
#include <hp300/dev/frodoreg.h>
#include <hp300/dev/frodovar.h>
#include <hp300/dev/com_frodovar.h>

#include "ioconf.h"

struct com_frodo_softc {
	struct	com_softc sc_com;	/* real "com" softc */
};

static int	com_frodo_match(device_t, cfdata_t , void *);
static void	com_frodo_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_frodo, sizeof(struct com_frodo_softc),
    com_frodo_match, com_frodo_attach, NULL, NULL);

static int com_frodo_speed = TTYDEF_SPEED;
static struct bus_space_tag comcntag;

#define CONMODE	((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#define COM_FRODO_FREQ	8006400

static int
com_frodo_match(device_t parent, cfdata_t match, void *aux)
{
	struct frodo_attach_args *fa = aux;

	if (strcmp(fa->fa_name, com_cd.cd_name) != 0)
		return 0;

	switch (fa->fa_offset) {
	case FRODO_APCI_OFFSET(1):
	case FRODO_APCI_OFFSET(2):
	case FRODO_APCI_OFFSET(3):
		return 1;
	}

	return 0;
}

static void
com_frodo_attach(device_t parent, device_t self, void *aux)
{
	struct com_frodo_softc *fsc = device_private(self);
	struct com_softc *sc = &fsc->sc_com;
	struct frodo_attach_args *fa = aux;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	int isconsole;

	sc->sc_dev = self;
	isconsole = com_is_console(&comcntag, fa->fa_base + fa->fa_offset,
	    &ioh);

	if (isconsole)
		iot = &comcntag;
	else
		iot = fa->fa_bst;

	if (!isconsole &&
	    bus_space_map(iot, fa->fa_base + fa->fa_offset, COM_NPORTS << 2,
	     0, &ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, iot, ioh, fa->fa_base + fa->fa_offset);

	sc->sc_frequency = COM_FRODO_FREQ;
	SET(sc->sc_hwflags, COM_HW_NOIEN);

	com_attach_subr(sc);

	frodo_intr_establish(parent, comintr, sc, fa->fa_line, IPL_VM);
}

int
com_frodo_cnattach(bus_space_tag_t bst, bus_addr_t addr, int scode)
{
	bus_space_tag_t iot = &comcntag;
	bus_space_handle_t ioh;
	volatile uint8_t *frodoregs;

	if (machineid != HP_425 || mmuid != MMUID_425_E)
		return 1;

	/*
	 * Check the service switch. On the 425e, this is a physical
	 * switch, unlike other frodo-based machines, so we can use it
	 * as a serial vs internal video selector, since the PROM can not
	 * be configured for serial console.
	 */
	frodoregs = (volatile uint8_t *)IIOV(INTIOBASE + FRODO_BASE);
	if (badaddr(__UNVOLATILE(frodoregs)) != 0) {
		/* 425e but no frodo chip found? */
		return 1;
	}

	/*
	 * if sti(4) is not configured, we need serial console anyway
	 * and no need to check the service switch.
	 */
#if NSTI_SGC > 0
	if (ISSET(frodoregs[FRODO_IISR], FRODO_IISR_SERVICE))
		return 1;
#endif

	frodo_init_bus_space(iot);

	if (bus_space_map(iot, addr, INTIO_DEVSIZE, BUS_SPACE_MAP_LINEAR, &ioh))
		return 1;

	comcnattach(iot, addr, com_frodo_speed, COM_FRODO_FREQ,
	    COM_TYPE_NORMAL, CONMODE);

	return 0;
}
