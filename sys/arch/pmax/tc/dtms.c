/*	$NetBSD: dtms.c,v 1.7.4.1 2007/03/12 05:49:52 rmind Exp $	*/

/*-
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
__KERNEL_RCSID(0, "$NetBSD: dtms.c,v 1.7.4.1 2007/03/12 05:49:52 rmind Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>

#include <machine/bus.h>

#include <arch/pmax/tc/dtreg.h>
#include <arch/pmax/tc/dtvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

struct dtms_softc {
	struct device	sc_dv;
	struct device	*sc_wsmousedev;
	int		sc_enabled;
};

int	dtms_match(struct device *, struct cfdata *, void *);
void	dtms_attach(struct device *, struct device *, void *);
int	dtms_input(void *, int);
int	dtms_enable(void *);
int	dtms_ioctl(void *, u_long, void *, int, struct lwp *);
void	dtms_disable(void *);
void	dtms_handler(void *, struct dt_msg *);

CFATTACH_DECL(dtms, sizeof(struct dtms_softc),
    dtms_match, dtms_attach, NULL, NULL);

const struct wsmouse_accessops dtms_accessops = {
	dtms_enable,
	dtms_ioctl,
	dtms_disable,
};

int
dtms_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct dt_attach_args *dta;

	dta = aux;
	return (dta->dta_addr == DT_ADDR_MOUSE);
}

void
dtms_attach(struct device *parent, struct device *self, void *aux)
{
	struct wsmousedev_attach_args a;
	struct dtms_softc *sc;
	struct dt_softc *dt;

	dt = (struct dt_softc *)parent;
	sc = (struct dtms_softc *)self;

	printf("\n");

	if (dt_establish_handler(dt, &dt_ms_dv, self, dtms_handler)) {
		printf("%s: unable to establish handler\n", self->dv_xname);
		return;
	}

	a.accessops = &dtms_accessops;
	a.accesscookie = sc;
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}

int
dtms_enable(void *cookie)
{
	struct dtms_softc *sc;

	sc = cookie;
	if (sc->sc_enabled)
		return (EBUSY);
	sc->sc_enabled = 1;

	return (0);
}

void
dtms_disable(void *cookie)
{
	struct dtms_softc *sc;

	sc = cookie;
	sc->sc_enabled = 0;
}

int
dtms_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = WSMOUSE_TYPE_VSXXX;
		return (0);
	}

	return (EPASSTHROUGH);
}

void
dtms_handler(void *cookie, struct dt_msg *msg)
{
	struct dtms_softc *sc;
	int buttons, dx, dy;
	short tmp;

	sc = cookie;

	if (!sc->sc_enabled)
		return;

	tmp = DT_GET_SHORT(msg->body[0], msg->body[1]);
	buttons = tmp & 1;
	if (tmp & 2)
		buttons |= 4;
	if (tmp & 4)
		buttons |= 2;

	tmp = DT_GET_SHORT(msg->body[2], msg->body[3]);
	if (tmp < 0)
		dx = -(-tmp & 0x1f);
	else
		dx = tmp & 0x1f;

	tmp = DT_GET_SHORT(msg->body[4], msg->body[5]);
	if (tmp < 0)
		dy = -(-tmp & 0x1f);
	else
		dy = tmp & 0x1f;

	wsmouse_input(sc->sc_wsmousedev,
			buttons,
			dx, dy, 0, 0,
			WSMOUSE_INPUT_DELTA);
}
