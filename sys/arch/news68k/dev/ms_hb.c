/*	$NetBSD: ms_hb.c,v 1.6 2003/07/15 02:59:26 lukem Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ms_hb.c,v 1.6 2003/07/15 02:59:26 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <news68k/dev/hbvar.h>
#include <news68k/dev/ms_hbreg.h>
#include <news68k/dev/msvar.h>

#include <news68k/news68k/isr.h>

#define MS_SIZE 0x10 /* XXX */
#define MS_IPL 5

int ms_hb_match(struct device *, struct cfdata *, void *);
void ms_hb_attach(struct device *, struct device *, void *);
void ms_hb_init(struct ms_softc *);
int ms_hb_intr(void *);

int ms_hb_enable(void *);
int ms_hb_ioctl(void *, u_long, caddr_t, int, struct proc *);
void ms_hb_disable(void *);

CFATTACH_DECL(ms_hb, sizeof(struct ms_softc),
    ms_hb_match, ms_hb_attach, NULL, NULL);

struct wsmouse_accessops ms_hb_accessops = {
	ms_hb_enable,
	ms_hb_ioctl,
	ms_hb_disable
};

int
ms_hb_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct hb_attach_args *ha = aux;
	u_int addr;

	if (strcmp(ha->ha_name, "ms"))
		return 0;

	/* XXX no default address */
	if (ha->ha_address == (u_int)-1)
		return 0;

	addr = IIOV(ha->ha_address); /* XXX */

	if (badaddr((void *)addr, 1))
		return 0;

	return 1;
}

void
ms_hb_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct ms_softc *sc = (void *)self;
	struct hb_attach_args *ha = aux;
	bus_space_tag_t bt = ha->ha_bust;
	bus_space_handle_t bh;
	struct wsmousedev_attach_args wsa;
	int ipl;

	if (bus_space_map(bt, ha->ha_address, MS_SIZE, 0, &bh) != 0) {
		printf("can't map device space\n");
		return;
	}

	printf("\n");

	sc->sc_bt = bt;
	sc->sc_bh = bh;
	sc->sc_offset = MS_REG_DATA;
	ipl = ha->ha_ipl;
	if (ipl == -1)
		ipl = MS_IPL;

	ms_hb_init(sc);

	isrlink_autovec(ms_hb_intr, (void *)sc, ipl, ISRPRI_TTY);

	wsa.accessops = &ms_hb_accessops;
	wsa.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &wsa, wsmousedevprint);
}

void
ms_hb_init(sc)
	struct ms_softc *sc;
{
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, MS_REG_RESET, 0);
	bus_space_write_1(bt, bh, MS_REG_INTE, MS_INTE);
}

int
ms_hb_intr(v)
	void *v;
{
	struct ms_softc *sc = v;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;
	int handled = 0;

	while ((bus_space_read_1(bt, bh, MS_REG_STAT) & MSSTAT_RDY) != 0) {
		ms_intr(sc);
		handled = 1;
	}
	if (handled)
		bus_space_write_1(bt, bh, MS_REG_INTE, MS_INTE);

	return handled;
}

int
ms_hb_enable(v)
	void *v;
{
	struct ms_softc *sc = v;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, MS_REG_INTE, MS_INTE);

	return 0;
}

void
ms_hb_disable(v)
	void *v;
{
	struct ms_softc *sc = v;
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;

	bus_space_write_1(bt, bh, MS_REG_INTE, 0);
}

int
ms_hb_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	return EPASSTHROUGH;
}
