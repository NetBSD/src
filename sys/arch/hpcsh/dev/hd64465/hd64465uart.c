/*	$NetBSD: hd64465uart.c,v 1.3.4.3 2002/06/23 17:37:00 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "opt_kgdb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/kgdb.h>

#include <sys/termios.h>
#include <dev/cons.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/console.h>

#include <dev/ic/comvar.h>
#include <dev/ic/comreg.h>

#include <machine/debug.h>

#include <hpcsh/dev/hd64465/hd64465var.h>
#include <hpcsh/dev/hd64465/hd64465intcreg.h>
#include <hpcsh/dev/hd64465/hd64465uartvar.h>
#include <hpcsh/dev/hd64465/hd64465uartreg.h>

STATIC struct hd64465uart_chip {
	struct hpcsh_bus_space __tag_body;
	bus_space_tag_t io_tag;
	int console;
} hd64465uart_chip;

struct hd64465uart_softc {
	struct com_softc sc_com;

	struct hd64465uart_chip *sc_chip;
	enum hd64465_module_id sc_module_id;
};

/* boot console */
cdev_decl(com);
void hd64465uartcnprobe(struct consdev *);
void hd64465uartcninit(struct consdev *);

STATIC int hd64465uart_match(struct device *, struct cfdata *, void *);
STATIC void hd64465uart_attach(struct device *, struct device *, void *);

struct cfattach hd64465uart_ca = {
	sizeof(struct hd64465uart_softc), hd64465uart_match,
	hd64465uart_attach
};

STATIC void hd64465uart_init(void);
STATIC u_int8_t hd64465uart_read_1(void *, bus_space_handle_t, bus_size_t);
STATIC void hd64465uart_write_1(void *, bus_space_handle_t, bus_size_t,
    u_int8_t);

#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#ifndef COMCN_SPEED
#define COMCN_SPEED	19200
#endif

void
hd64465uartcnprobe(struct consdev *cp)
{
	int maj;

	/* locate the major number */
	for (maj = 0; maj < nchrdev; maj++)
		if (cdevsw[maj].d_open == comopen)
			break;

	/* Initialize required fields. */
	cp->cn_dev = makedev(maj, 0);
	cp->cn_pri = CN_NORMAL;
}

void
hd64465uartcninit(struct consdev *cp)
{

	hd64465uart_init();

	comcnattach(hd64465uart_chip.io_tag, 0x0, COMCN_SPEED, COM_FREQ,
	    CONMODE);	

	hd64465uart_chip.console = 1;
}

#ifdef KGDB
int
hd64465uart_kgdb_init()
{

	if (strcmp(kgdb_devname, "hd64465uart") != 0)
		return (1);

	if (hd64465uart_chip.console)
		return (1);	/* can't share with console */

	hd64465uart_init();

	if (com_kgdb_attach(hd64465uart_chip.io_tag, 0x0, kgdb_rate,
	    COM_FREQ, CONMODE) != 0) {
		printf("%s: KGDB console open failed.\n", __FUNCTION__);
		return (1);
	}

	return (0);
}
#endif /* KGDB */

int
hd64465uart_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hd64465_attach_args *ha = aux;

	return (ha->ha_module_id == HD64465_MODULE_UART);
}

void
hd64465uart_attach(struct device *parent, struct device *self, void *aux)
{
	struct hd64465_attach_args *ha = aux;
	struct hd64465uart_softc *sc = (struct hd64465uart_softc *)self;
	struct com_softc *csc = &sc->sc_com;

	sc->sc_chip = &hd64465uart_chip;

	sc->sc_module_id = ha->ha_module_id;

	if (!sc->sc_chip->console)
		hd64465uart_init();

	csc->sc_iot = sc->sc_chip->io_tag;
	bus_space_map(csc->sc_iot, 0, 8, 0, &csc->sc_ioh);
	csc->sc_iobase = 0;
	csc->sc_frequency = COM_FREQ;

	/* supply clock XXX notyet */

	/* sanity check */
	if (!comprobe1(csc->sc_iot, csc->sc_ioh)) {
		printf(": device problem. don't attach.\n");

		/* stop clock XXX notyet */
		return;
	}

	com_attach_subr(csc);

	/* register interrupt handler */
	hd64465_intr_establish(HD64465_UART, IST_LEVEL, IPL_TTY,
	    comintr, self);
}

void
hd64465uart_init()
{

	if (hd64465uart_chip.io_tag)
		return;

	hd64465uart_chip.io_tag = bus_space_create(
		&hd64465uart_chip.__tag_body, "HD64465 UART I/O",
		0xb0008000, 0); /* no extent */

	/* override bus_space_read_1, bus_space_write_1 */
	hd64465uart_chip.io_tag->hbs_r_1 = hd64465uart_read_1;
	hd64465uart_chip.io_tag->hbs_w_1 = hd64465uart_write_1;
}

u_int8_t
hd64465uart_read_1(void *t, bus_space_handle_t h, bus_size_t ofs)
{

	return *(__volatile__ u_int8_t *)(h + (ofs << 1));
}

void
hd64465uart_write_1(void *t, bus_space_handle_t h, bus_size_t ofs,
    u_int8_t val)
{

	*(__volatile__ u_int8_t *)(h + (ofs << 1)) = val;	
}
