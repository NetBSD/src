/*	$NetBSD: hd64461uart.c,v 1.20 2007/12/15 00:39:19 perry Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hd64461uart.c,v 1.20 2007/12/15 00:39:19 perry Exp $");

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

#include <hpcsh/dev/hd64461/hd64461var.h>
#include <hpcsh/dev/hd64461/hd64461reg.h>
#include <hpcsh/dev/hd64461/hd64461intcreg.h>
#include <hpcsh/dev/hd64461/hd64461uartvar.h>
#include <hpcsh/dev/hd64461/hd64461uartreg.h>

STATIC struct hd64461uart_chip {
	struct hpcsh_bus_space __tag_body;
	bus_space_tag_t io_tag;
	int console;
} hd64461uart_chip;

struct hd64461uart_softc {
	struct com_softc sc_com;

	struct hd64461uart_chip *sc_chip;
	enum hd64461_module_id sc_module_id;
};

/* boot console */
void hd64461uartcnprobe(struct consdev *);
void hd64461uartcninit(struct consdev *);

STATIC int hd64461uart_match(struct device *, struct cfdata *, void *);
STATIC void hd64461uart_attach(struct device *, struct device *, void *);

CFATTACH_DECL(hd64461uart, sizeof(struct hd64461uart_softc),
    hd64461uart_match, hd64461uart_attach, NULL, NULL);

STATIC void hd64461uart_init(void);
STATIC uint8_t hd64461uart_read_1(void *, bus_space_handle_t, bus_size_t);
STATIC void hd64461uart_write_1(void *, bus_space_handle_t, bus_size_t,
    uint8_t);

#define CONMODE ((TTYDEF_CFLAG & ~(CSIZE | CSTOPB | PARENB)) | CS8) /* 8N1 */
#ifndef COMCN_SPEED
#define COMCN_SPEED	19200
#endif

void
hd64461uartcnprobe(struct consdev *cp)
{
	extern const struct cdevsw com_cdevsw;
	int maj;

	/* locate the major number */
	maj = cdevsw_lookup_major(&com_cdevsw);

	/* Initialize required fields. */
	cp->cn_dev = makedev(maj, 0);
	cp->cn_pri = CN_NORMAL;
}

void
hd64461uartcninit(struct consdev *cp)
{

	hd64461uart_init();

	comcnattach(hd64461uart_chip.io_tag, 0x0, COMCN_SPEED, COM_FREQ,
	    COM_TYPE_NORMAL, CONMODE);

	hd64461uart_chip.console = 1;
}

#ifdef KGDB
int
hd64461uart_kgdb_init(void)
{

	if (strcmp(kgdb_devname, "hd64461uart") != 0)
		return (1);

	if (hd64461uart_chip.console)
		return (1);	/* can't share with console */

	hd64461uart_init();

	if (com_kgdb_attach(hd64461uart_chip.io_tag, 0x0, kgdb_rate,
	    COM_FREQ, COM_TYPE_NORMAL, CONMODE) != 0) {
		printf("%s: KGDB console open failed.\n", __func__);
		return (1);
	}

	return (0);
}
#endif /* KGDB */

STATIC int
hd64461uart_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hd64461_attach_args *ha = aux;

	return (ha->ha_module_id == HD64461_MODULE_UART);
}

STATIC void
hd64461uart_attach(struct device *parent, struct device *self, void *aux)
{
	struct hd64461_attach_args *ha = aux;
	struct hd64461uart_softc *sc = (struct hd64461uart_softc *)self;
	struct com_softc *csc = &sc->sc_com;
	uint16_t r16;
	bus_space_handle_t ioh;

	sc->sc_chip = &hd64461uart_chip;

	sc->sc_module_id = ha->ha_module_id;

	if (!sc->sc_chip->console)
		hd64461uart_init();

	bus_space_map(sc->sc_chip->io_tag, 0, 8, 0, &ioh);
	csc->sc_frequency = COM_FREQ;
	COM_INIT_REGS(csc->sc_regs, sc->sc_chip->io_tag, ioh, 0);

	/* switch port to UART */

	/* supply clock */
	r16 = hd64461_reg_read_2(HD64461_SYSSTBCR_REG16);
	r16 &= ~HD64461_SYSSTBCR_SURTSD;
	hd64461_reg_write_2(HD64461_SYSSTBCR_REG16, r16);

	/* sanity check */
	if (!com_probe_subr(&csc->sc_regs)) {
		printf(": device problem. don't attach.\n");

		/* stop clock */
		r16 |= HD64461_SYSSTBCR_SURTSD;
		hd64461_reg_write_2(HD64461_SYSSTBCR_REG16, r16);
		return;
	}

	com_attach_subr(csc);

	hd6446x_intr_establish(HD64461_INTC_UART, IST_LEVEL, IPL_TTY,
	    comintr, self);
}

STATIC void
hd64461uart_init(void)
{

	if (hd64461uart_chip.io_tag)
		return;

	hd64461uart_chip.io_tag = bus_space_create(
		&hd64461uart_chip.__tag_body, "HD64461 UART I/O",
		HD64461_UART_REGBASE, 0); /* no extent */

	/* override bus_space_read_1, bus_space_write_1 */
	hd64461uart_chip.io_tag->hbs_r_1 = hd64461uart_read_1;
	hd64461uart_chip.io_tag->hbs_w_1 = hd64461uart_write_1;
}

STATIC uint8_t
hd64461uart_read_1(void *t, bus_space_handle_t h, bus_size_t ofs)
{

	return *(volatile uint8_t *)(h + (ofs << 1));
}

STATIC void
hd64461uart_write_1(void *t, bus_space_handle_t h, bus_size_t ofs,
    uint8_t val)
{

	*(volatile uint8_t *)(h + (ofs << 1)) = val;
}
