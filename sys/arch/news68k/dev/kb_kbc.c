/*	$NetBSD: kb_kbc.c,v 1.7.44.2 2008/06/02 13:22:28 mjf Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: kb_kbc.c,v 1.7.44.2 2008/06/02 13:22:28 mjf Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>


#include <news68k/dev/kbcreg.h>
#include <news68k/dev/kbcvar.h>
#include <news68k/dev/kbvar.h>

#include <news68k/news68k/isr.h>

static int kb_kbc_match(device_t, cfdata_t, void *);
static void kb_kbc_attach(device_t, device_t, void *);
static void kb_kbc_init(struct kb_softc *);
int	kb_kbc_intr(void *);
int	kb_kbc_cnattach(void);

CFATTACH_DECL_NEW(kb_kbc, sizeof(struct kb_softc),
    kb_kbc_match, kb_kbc_attach, NULL, NULL);

struct console_softc kb_kbc_conssc;

static int
kb_kbc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct kbc_attach_args *ka = aux;

	if (strcmp(ka->ka_name, "kb"))
		return 0;

	return 1;
}

static void
kb_kbc_attach(device_t parent, device_t self, void *aux)
{
	struct kb_softc *sc = device_private(self);
	struct kbc_attach_args *ka = aux;
	struct wskbddev_attach_args wsa;
	int ipl;

	sc->sc_dev = self;
	aprint_normal("\n");

	sc->sc_bt = ka->ka_bt;
	sc->sc_bh = ka->ka_bh;
	sc->sc_offset = KBC_KBREG_DATA;
	sc->sc_conssc = &kb_kbc_conssc;
	ipl = ka->ka_ipl;

	kb_kbc_init(sc);

	isrlink_autovec(kb_kbc_intr, (void *)sc, ipl, IPL_TTY);

	wsa.console = kb_kbc_conssc.cs_isconsole;
	wsa.keymap = &kb_keymapdata;
	wsa.accessops = &kb_accessops;
	wsa.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &wsa, wskbddevprint);
}

static void
kb_kbc_init(struct kb_softc *sc)
{
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, KBC_KBREG_RESET, 0);
	bus_space_write_1(bt, bh, KBC_KBREG_INTE, KBC_INTE);
}

int
kb_kbc_intr(void *arg)
{
	struct kb_softc *sc = (struct kb_softc *)arg;
	struct console_softc *kb_conssc = sc->sc_conssc;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;
	int stat, handled = 0;

	kb_conssc->cs_nintr++;

	stat = bus_space_read_1(bt, bh, KBC_KBREG_STAT);
	if (stat & KBCSTAT_KBRDY) {
		kb_intr(sc);
		handled = 1;
		bus_space_write_1(bt, bh, KBC_KBREG_INTE, KBC_INTE);
	}

	return handled;
}

int
kb_kbc_cnattach(void)
{

	kb_kbc_conssc.cs_isconsole = 1;
	return kb_cnattach(&kb_kbc_conssc);
}
