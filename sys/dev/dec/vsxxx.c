/* $NetBSD: vsxxx.c,v 1.2.8.2 2002/04/01 07:45:11 nathanw Exp $ */

/*
 * Copyright (c) 1999 Tohru Nishimura.  All rights reserved.
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
 *      This product includes software developed by Tohru Nishimura.
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
__KERNEL_RCSID(0, "$NetBSD: vsxxx.c,v 1.2.8.2 2002/04/01 07:45:11 nathanw Exp $");

/*
 * Common machinary for VSXXX mice and tablet
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/dec/vsxxxvar.h>

/*
 * XXX XXX XXX
 *
 * collect various mice and tablet parameters/protocol design
 * and establish vsxxx.h
 *
 * XXX XXX XXX
 */
#define	VS_SELFTEST	'T'
#define	VS_INCREMENTAL	'R'
#define	VS_PROMPTING	'D'
#define	VS_REQ_POSITION	'P'

#define	VS_MOUSE_REPSZ	3
#define	VS_TABLET_REPSZ	5

/* first octet */
#define	VS_START_FRAME	0x80
#define	VS_R_BUTTON	0x01
#define	VS_M_BUTTON	0x02
#define	VS_L_BUTTON	0x04
#define	VS_Y_SIGN	0x08
#define	VS_X_SIGN	0x10

/* low order 4 bit of the second octet in a SELFTEST reply */
#define	VS_MOUSE	0x2
#define	VS_TABLET	0x4

extern struct cfdriver vsms_cd;

static int  vsxxx_enable __P((void *));
static int  vsxxx_ioctl __P((void *, u_long, caddr_t, int, struct proc *));
static void vsxxx_disable __P((void *));

struct wsmouse_accessops vsxxx_accessops = {	/* EXPORT */
	vsxxx_enable,				/* MD? */
	vsxxx_ioctl,
	vsxxx_disable,				/* MD? */
};

static int
vsxxx_enable(v)
	void *v;
{
	/* turn on the hardware? */
	((struct vsxxx_softc *)v)->sc_nbyte = 0;
	return 0;
}

static void
vsxxx_disable(v)
	void *v;
{
	/* turn off the hardware? */
}

/*ARGUSED*/
static int
vsxxx_ioctl(v, cmd, data, flag, p)
	void *v;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = WSMOUSE_TYPE_VSXXX;
		return 0;
	}
	return EPASSTHROUGH;
}

/* EXPORT */ void
vsxxx_input(data)
	int data;
{
	struct vsxxx_softc *sc = (void *)vsms_cd.cd_devs[0];
	int x, y;

	if (data & VS_START_FRAME)
		sc->sc_nbyte = 0;

	sc->sc_report.raw[sc->sc_nbyte++] = data;

	if (sc->sc_nbyte < VS_MOUSE_REPSZ)
		return;
	sc->sc_nbyte = 0;

	x = sc->sc_report.ms.xpos;
	y = sc->sc_report.ms.ypos;
	if ((sc->sc_report.raw[0] & VS_X_SIGN) == 0)
		x = -x;
	if ((sc->sc_report.raw[0] & VS_Y_SIGN) != 0)
		y = -y;					/* Eeeh? */
	wsmouse_input(sc->sc_wsmousedev, sc->sc_report.raw[0] & 07, x, y, 0,
		      WSMOUSE_INPUT_DELTA);
}
