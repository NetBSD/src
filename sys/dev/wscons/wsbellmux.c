/* $NetBSD: wsbellmux.c,v 1.1.6.2 2017/12/03 11:37:37 jdolecek Exp $ */
/*-
 * Copyright (c) 2017 Nathanial Sloss <nathanialsloss@yahoo.com.au>
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
__KERNEL_RCSID(0, "$NetBSD: wsbellmux.c,v 1.1.6.2 2017/12/03 11:37:37 jdolecek Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/types.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/selinfo.h>

#include "wsmux.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsbellvar.h>

/*
 * Print function (for parent devices).
 */
int
wsbelldevprint(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("wsbell at %s", pnp);
	return (UNCONF);
}

#if NWSMUX > 0
int
wsbell_add_mux(int unit, struct wsmux_softc *muxsc)
{
	struct wsbell_softc *sc;
	device_t wsbelldev;
	cfdriver_t wsbellcd;

	wsbelldev = device_find_by_driver_unit("wsbell", unit);
	if (wsbelldev == NULL)
		return ENXIO;
	wsbellcd = device_cfdriver(wsbelldev);
	if (wsbellcd == NULL)
		return ENXIO;

	sc = device_lookup_private(wsbellcd, unit);
	if (sc == NULL)
		return ENXIO;

	if (sc->sc_base.me_parent != NULL || sc->sc_base.me_evp != NULL)
		return (EBUSY);

	return (wsmux_attach_sc(muxsc, &sc->sc_base));
}
#endif
