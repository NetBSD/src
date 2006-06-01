/*	$NetBSD: com_mainbus.c,v 1.10.6.2 2006/06/01 22:34:18 kardel Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: com_mainbus.c,v 1.10.6.2 2006/06/01 22:34:18 kardel Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/termios.h>

#include <machine/autoconf.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/nvram.h>
#include <machine/bootinfo.h>

#include <dev/ic/comreg.h>
#include <dev/ic/comvar.h>

#include <cobalt/dev/com_mainbusvar.h>

extern int console_present;

struct com_mainbus_softc {
	struct com_softc sc_com;
	void *sc_ih;
};

#define COM_MAINBUS_FREQ	(COM_FREQ * 10)
#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */

static int	com_mainbus_probe(struct device *, struct cfdata *, void *);
static void	com_mainbus_attach(struct device *, struct device *, void *);

CFATTACH_DECL(com_mainbus, sizeof(struct com_mainbus_softc),
    com_mainbus_probe, com_mainbus_attach, NULL, NULL);

int
com_mainbus_probe(struct device *parent, struct cfdata *match, void *aux)
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
com_mainbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct com_mainbus_softc *msc = (void *)self;
	struct com_softc *sc = &msc->sc_com;
	struct mainbus_attach_args *maa = aux;

	sc->sc_iot = maa->ma_iot;
	sc->sc_iobase = maa->ma_addr;

	if (!com_is_console(sc->sc_iot, sc->sc_iobase, &sc->sc_ioh) &&
	    bus_space_map(sc->sc_iot, sc->sc_iobase, COM_NPORTS, 0,
	    &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	sc->sc_frequency = COM_MAINBUS_FREQ;

	com_attach_subr(sc);

	cpu_intr_establish(maa->ma_level, IPL_SERIAL, comintr, sc);

	return;
}

void
com_mainbus_cnprobe(struct consdev *cn)
{
	struct btinfo_flags *bi_flags;

	/*
	 * Linux code has a comment that serial console must be probed
	 * early, otherwise the value which allows to detect serial port
	 * could be overwritten. Okay, probe here and record the result
	 * for the future use.
	 *
	 * Note that if the kernel was booted with a boot loader,
	 * the latter *has* to provide a flag indicating whether console
	 * is present or not because the value might be overwritten by
	 * the loaded kernel.
	 */
	if ((bi_flags = lookup_bootinfo(BTINFO_FLAGS)) == NULL) {
		/* No boot information, probe console now */
		console_present = *(volatile uint32_t *)
					MIPS_PHYS_TO_KSEG1(0x0020001c);
	} else {
		/* Get the value determined by the boot loader. */
		console_present = bi_flags->bi_flags & BI_SERIAL_CONSOLE;
	}

	cn->cn_pri = (console_present != 0) ? CN_NORMAL : CN_DEAD;
}

void
com_mainbus_cninit(struct consdev *cn)
{

	comcnattach(0, 0x1c800000, 115200, COM_MAINBUS_FREQ, COM_TYPE_NORMAL,
	    CONMODE);
}
