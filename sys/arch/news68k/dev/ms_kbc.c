/*	$NetBSD: ms_kbc.c,v 1.1.10.1 2002/04/01 07:41:36 nathanw Exp $	*/

/*-
 * Copyright (c) 2001 Izumi Tsutsui.  All rights reserved.
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <news68k/dev/kbcreg.h>
#include <news68k/dev/kbcvar.h>
#include <news68k/dev/msvar.h>

#include <news68k/news68k/isr.h>

int ms_kbc_match(struct device *, struct cfdata *, void *);
void ms_kbc_attach(struct device *, struct device *, void *);
void ms_kbc_init(struct ms_softc *);
int ms_kbc_intr(void *);

int ms_kbc_enable(void *);
void ms_kbc_disable(void *);
int ms_kbc_ioctl(void *, u_long, caddr_t, int, struct proc *);

struct cfattach ms_kbc_ca = {
	sizeof(struct ms_softc), ms_kbc_match, ms_kbc_attach
};

struct wsmouse_accessops ms_kbc_accessops = {
	ms_kbc_enable,
	ms_kbc_ioctl,
	ms_kbc_disable
};

int
ms_kbc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct kbc_attach_args *ka = aux;

	if (strcmp(ka->ka_name, "ms"))
		return 0;

	return 1;
}

void
ms_kbc_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ms_softc *sc = (void *)self;
	struct kbc_attach_args *ka = aux;
	struct wsmousedev_attach_args wsa;
	int ipl;

	printf("\n");

	sc->sc_bt = ka->ka_bt;
	sc->sc_bh = ka->ka_bh;
	sc->sc_offset = KBC_MSREG_DATA;
	ipl = ka->ka_ipl;

	ms_kbc_init(sc);

	isrlink_autovec(ms_kbc_intr, (void *)sc, ipl, ISRPRI_TTY);

	wsa.accessops = &ms_kbc_accessops;
	wsa.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &wsa, wsmousedevprint);
}

void
ms_kbc_init(sc)
	struct ms_softc *sc;
{
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, KBC_MSREG_RESET, 0);
	bus_space_write_1(bt, bh, KBC_MSREG_INTE, KBC_INTE);
}

int
ms_kbc_intr(v)
	void *v;
{
	struct ms_softc *sc = v;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;
	int handled = 0;

	while (bus_space_read_1(bt, bh, KBC_MSREG_STAT) & KBCSTAT_MSRDY) {
		ms_intr(sc);
		handled = 1;
	}
	if (handled)
		bus_space_write_1(bt, bh, KBC_MSREG_INTE, KBC_INTE);

	return handled;
}

int
ms_kbc_enable(v)
	void *v;
{
	struct ms_softc *sc = v;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, KBC_MSREG_INTE, KBC_INTE);

	return 0;
}

void
ms_kbc_disable(v)
	void *v;
{
	struct ms_softc *sc = v;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, KBC_MSREG_INTE, 0);
}

int
ms_kbc_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return EPASSTHROUGH;
}
