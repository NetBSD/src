/*	$NetBSD: ms.c,v 1.2 2003/07/15 02:59:26 lukem Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: ms.c,v 1.2 2003/07/15 02:59:26 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <news68k/dev/msvar.h>

void
ms_intr(sc)
	struct ms_softc *sc;
{
	bus_space_tag_t bt = sc->sc_bt;
	bus_space_handle_t bh = sc->sc_bh;
	bus_size_t offset = sc->sc_offset;
	int code, index, byte0, byte1, byte2;
	int button, dx, dy;

	if (sc->sc_wsmousedev == NULL)
		return;

	code = bus_space_read_1(bt, bh, offset);
	index = sc->sc_ndata;

	if (code & MS_S_MARK) {
		sc->sc_buf[MS_S_BYTE] = code;
		sc->sc_ndata = MS_X_BYTE;
		return;
	}

	if (index == MS_X_BYTE) {
		sc->sc_buf[MS_X_BYTE] = code;
		sc->sc_ndata = MS_Y_BYTE;
		return;
	}

	if (index == MS_Y_BYTE) {
		sc->sc_buf[MS_Y_BYTE] = code;
		sc->sc_ndata = 0;

		byte0 = sc->sc_buf[MS_S_BYTE];
		byte1 = sc->sc_buf[MS_X_BYTE];
		byte2 = sc->sc_buf[MS_Y_BYTE];

		button = 0;
		if (byte0 & MS_S_SW1)
			button |= 0x01;
		if (byte0 & MS_S_SW3)
			button |= 0x02;
		if (byte0 & MS_S_SW2)
			button |= 0x04;

		dx = byte1 & MS_X_DATA;
		if (byte0 & MS_S_XSIGN)
			dx = -dx;
		dy = byte2 & MS_Y_DATA;
		if (byte0 & MS_S_YSIGN)
			dy = -dy;

		wsmouse_input(sc->sc_wsmousedev, button, dx, -dy, 0,
		    WSMOUSE_INPUT_DELTA);
	}
}
