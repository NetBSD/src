/*	$NetBSD: ms_ap.c,v 1.10.38.1 2012/10/30 17:20:05 yamt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ms_ap.c,v 1.10.38.1 2012/10/30 17:20:05 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/adrsmap.h>
#include <newsmips/apbus/apbusvar.h>

struct msreg {
	volatile uint32_t ms_data;
	volatile uint32_t ms_stat;
	volatile uint32_t ms_intr_en;
	volatile uint32_t ms_reset;
	volatile uint32_t ms_speed;
};

struct ms_ap_softc {
	device_t sc_dev;
	struct msreg *sc_reg;
	device_t sc_wsmousedev;
	int sc_ndata;
	uint8_t sc_buf[3];
};

int ms_ap_match(device_t, cfdata_t, void *);
void ms_ap_attach(device_t, device_t, void *);
int ms_ap_intr(void *);

int ms_ap_enable(void *);
int ms_ap_ioctl(void *, u_long, void *, int, struct lwp *);
void ms_ap_disable(void *);

CFATTACH_DECL_NEW(ms_ap, sizeof(struct ms_ap_softc),
    ms_ap_match, ms_ap_attach, NULL, NULL);

struct wsmouse_accessops ms_ap_accessops = {
	ms_ap_enable,
	ms_ap_ioctl,
	ms_ap_disable
};

int
ms_ap_match(device_t parent, cfdata_t cf, void *aux)
{
	struct apbus_attach_args *apa = aux;

	if (strcmp(apa->apa_name, "ms") == 0)
		return 1;

	return 0;
}

void
ms_ap_attach(device_t parent, device_t self, void *aux)
{
	struct ms_ap_softc *sc = device_private(self);
	struct apbus_attach_args *apa = aux;
	struct msreg *reg = (void *)apa->apa_hwbase;
	struct wsmousedev_attach_args aa;

	sc->sc_dev = self;
	aprint_normal(" slot%d addr 0x%lx\n", apa->apa_slotno, apa->apa_hwbase);

	sc->sc_reg = reg;

	reg->ms_reset = 0x03;
	reg->ms_speed = 0x01;
	reg->ms_intr_en = 0;

	apbus_intr_establish(1, NEWS5000_INT1_KBD, 0, ms_ap_intr, sc,
	    "", apa->apa_ctlnum);

	aa.accessops = &ms_ap_accessops;
	aa.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &aa, wsmousedevprint);
}

int
ms_ap_intr(void *v)
{
	struct ms_ap_softc *sc = v;
	struct msreg *reg = sc->sc_reg;
	int code, index, byte0, byte1, byte2;
	int button, dx, dy;
	int rv = 0;

	reg->ms_intr_en = 0;

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

			wsmouse_input(sc->sc_wsmousedev,
					button,
					dx, -dy, 0, 0,
					WSMOUSE_INPUT_DELTA);
		}
	}

	reg->ms_intr_en = 1;
	return rv;
}

int
ms_ap_enable(void *v)
{
	struct ms_ap_softc *sc = v;
	struct msreg *reg = sc->sc_reg;

	reg->ms_intr_en = 1;
	return 0;
}

void
ms_ap_disable(void *v)
{
	struct ms_ap_softc *sc = v;
	struct msreg *reg = sc->sc_reg;

	reg->ms_intr_en = 0;
}

int
ms_ap_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	return EPASSTHROUGH;
}
