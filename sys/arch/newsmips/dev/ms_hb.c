/*	$NetBSD: ms_hb.c,v 1.7 2003/05/10 09:46:25 tsutsui Exp $	*/

/*-
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

#include <machine/adrsmap.h>

#include <newsmips/dev/hbvar.h>

struct msreg {
	u_char ms_data;
	u_char ms_stat;
	u_char ms_reset;
	u_char ms_init;
};

struct ms_hb_softc {
	struct device sc_dev;
	volatile struct msreg *sc_reg;
	struct device *sc_wsmousedev;
	int sc_ndata;
	u_char sc_buf[3];
};

int ms_hb_match(struct device *, struct cfdata *, void *);
void ms_hb_attach(struct device *, struct device *, void *);
int ms_hb_intr(void *);

int ms_hb_enable(void *);
int ms_hb_ioctl(void *, u_long, caddr_t, int, struct proc *);
void ms_hb_disable(void *);

CFATTACH_DECL(ms_hb, sizeof(struct ms_hb_softc),
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

	if (strcmp(ha->ha_name, "ms") == 0)
		return 1;

	return 0;
}

void
ms_hb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ms_hb_softc *sc = (void *)self;
	struct hb_attach_args *ha = aux;
	volatile struct msreg *reg;
	struct wsmousedev_attach_args aa;
	int intr;

	reg = (struct msreg *)ha->ha_addr;
	intr = ha->ha_level;

	if (intr == -1)
		intr = 2;

	sc->sc_reg = reg;

	reg->ms_reset = 0x01;
	reg->ms_init = 0x80;	/* 1200 bps */

	printf(" level %d\n", intr);

	hb_intr_establish(intr, INTEN0_MSINT, IPL_TTY, ms_hb_intr, sc);

	aa.accessops = &ms_hb_accessops;
	aa.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &aa, wsmousedevprint);
}

int
ms_hb_intr(v)
	void *v;
{
	struct ms_hb_softc *sc = v;
	volatile struct msreg *reg = sc->sc_reg;
	volatile u_char *ien = (void *)INTEN0;
	int code, index, byte0, byte1, byte2;
	int button, dx, dy;
	int rv = 0;

	*ien &= ~RX_MSINTE;

	while (reg->ms_stat & RX_MSRDY) {
		rv = 1;
		code = reg->ms_data;
		index = sc->sc_ndata;

		if (sc->sc_wsmousedev == NULL)
			continue;

		if (code & 0x80) {
			sc->sc_buf[0] = code;
			sc->sc_ndata = 1;
			continue;
		}

		if (index == 1) {
			sc->sc_buf[1] = code;
			sc->sc_ndata++;
			continue;
		}

		if (index == 2) {
			sc->sc_buf[2] = code;
			sc->sc_ndata++;

			byte0 = sc->sc_buf[0];
			byte1 = sc->sc_buf[1];
			byte2 = sc->sc_buf[2];

			button = 0;
			if (byte0 & 0x01)
				button |= 1;
			if (byte0 & 0x04)
				button |= 2;
			if (byte0 & 0x02)
				button |= 4;

			if (byte0 & 0x08)
				dx = byte1 - 0x80;
			else
				dx = byte1;
			if (byte0 & 0x10)
				dy = byte2 - 0x80;
			else
				dy = byte2;

			wsmouse_input(sc->sc_wsmousedev, button, dx, -dy, 0,
			    WSMOUSE_INPUT_DELTA);
		}
	}

	*ien |= RX_MSINTE;
	return rv;
}

int
ms_hb_enable(v)
	void *v;
{
	volatile u_char *ien = (void *)INTEN0;

	*ien |= RX_MSINTE;
	return 0;
}

void
ms_hb_disable(v)
	void *v;
{
	volatile u_char *ien = (void *)INTEN0;

	*ien &= ~RX_MSINTE;
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
