/*	$NetBSD: com_mainbus.c,v 1.19.12.1 2017/12/03 11:36:00 jdolecek Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: com_mainbus.c,v 1.19.12.1 2017/12/03 11:36:00 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/termios.h>

#include <machine/autoconf.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <cobalt/cobalt/console.h>
#include <cobalt/dev/com_mainbusvar.h>

struct com_mainbus_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

#define COM_MAINBUS_FREQ	(COM_FREQ * 10)
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

static int	com_mainbus_probe(device_t, cfdata_t , void *);
static void	com_mainbus_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(com_mainbus, sizeof(struct com_mainbus_softc),
    com_mainbus_probe, com_mainbus_attach, NULL, NULL);

int
com_mainbus_probe(device_t parent, cfdata_t match, void *aux)
{

	switch (cobalt_id) {
	case COBALT_ID_RAQ:
	case COBALT_ID_QUBE2:
	case COBALT_ID_RAQ2:
		return 1;
	}

	return 0;
}

void
com_mainbus_attach(device_t parent, device_t self, void *aux)
{
	struct com_mainbus_softc *msc = device_private(self);
	struct com_softc *sc = &msc->sc_com;
	struct mainbus_attach_args *maa = aux;
	bus_space_handle_t	ioh;

	sc->sc_dev = self;
	if (!com_is_console(maa->ma_iot, maa->ma_addr, &ioh) &&
	    bus_space_map(maa->ma_iot, maa->ma_addr, COM_NPORTS, 0, &ioh)) {
		aprint_error(": can't map i/o space\n");
		return;
	}
	COM_INIT_REGS(sc->sc_regs, maa->ma_iot, ioh, maa->ma_addr);

	sc->sc_frequency = COM_MAINBUS_FREQ;

	com_attach_subr(sc);

	cpu_intr_establish(maa->ma_level, IPL_SERIAL, comintr, sc);

	return;
}

void
com_mainbus_cnprobe(struct consdev *cn)
{

	cn->cn_pri = (console_present != 0 && cobalt_id != COBALT_ID_QUBE2700)
	    ? CN_NORMAL : CN_DEAD;
}

void
com_mainbus_cninit(struct consdev *cn)
{

	comcnattach(0, COM_BASE, 115200, COM_MAINBUS_FREQ, COM_TYPE_NORMAL,
	    CONMODE);
}
