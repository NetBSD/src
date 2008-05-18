/*	$NetBSD: kb_hb.c,v 1.11.2.1 2008/05/18 12:32:31 yamt Exp $	*/

/*-
 * Copyright (c) 2001 Izumi Tsutsui.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: kb_hb.c,v 1.11.2.1 2008/05/18 12:32:31 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/kb_hbreg.h>
#include <news68k/dev/kbvar.h>

#include <news68k/news68k/isr.h>

#include "ioconf.h"

#define KB_SIZE 0x10 /* XXX */
#define KB_PRI 5

static int kb_hb_match(device_t, cfdata_t, void *);
static void kb_hb_attach(device_t, device_t, void *);
static void kb_hb_init(struct kb_softc *);
int	kb_hb_intr(void *);
int	kb_hb_cnattach(void);

CFATTACH_DECL_NEW(kb_hb, sizeof(struct kb_softc),
    kb_hb_match, kb_hb_attach, NULL, NULL);

struct console_softc kb_hb_conssc;

static int
kb_hb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct hb_attach_args *ha = aux;
	u_int addr;

	if (strcmp(ha->ha_name, "kb"))
		return 0;

	/* XXX no default address */
	if (ha->ha_address == (u_int)-1)
		return 0;

	addr = IIOV(ha->ha_address); /* XXX */

	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

static void
kb_hb_attach(device_t parent, device_t self, void *aux)
{
	struct kb_softc *sc = device_private(self);
	struct hb_attach_args *ha = aux;
	bus_space_tag_t bt = ha->ha_bust;
	bus_space_handle_t bh;
	struct wskbddev_attach_args wsa;
	int ipl;

	sc->sc_dev = self;

	if (bus_space_map(bt, ha->ha_address, KB_SIZE, 0, &bh) != 0) {
		aprint_error(": can't map device space\n");
		return;
	}

	aprint_normal("\n");

	sc->sc_bt = bt;
	sc->sc_bh = bh;
	sc->sc_offset = KB_REG_DATA;
	sc->sc_conssc = &kb_hb_conssc;

	ipl = ha->ha_ipl;
	if (ipl == -1)
		ipl = KB_PRI;

	kb_hb_init(sc);

	isrlink_autovec(kb_hb_intr, (void *)sc, ipl, IPL_TTY);

	wsa.console = kb_hb_conssc.cs_isconsole;
	wsa.keymap = &kb_keymapdata;
	wsa.accessops = &kb_accessops;
	wsa.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &wsa, wskbddevprint);
}

static void
kb_hb_init(struct kb_softc *sc)
{
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, KB_REG_RESET, 0);
	bus_space_write_1(bt, bh, KB_REG_INTE, KB_INTE);
}

int
kb_hb_intr(void *arg)
{
	struct kb_softc *sc = (struct kb_softc *)arg;
	struct console_softc *kb_conssc = sc->sc_conssc;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;
	int stat, handled = 0;

	kb_conssc->cs_nintr++;

	stat = bus_space_read_1(bt, bh, KB_REG_STAT);
	if (stat & KBSTAT_RDY) {
		kb_intr(sc);
		handled = 1;
		bus_space_write_1(bt, bh, KB_REG_INTE, KB_INTE);
	}

	return handled;
}

int
kb_hb_cnattach(void)
{

	kb_hb_conssc.cs_isconsole = 1;
	return kb_cnattach(&kb_hb_conssc);
}
