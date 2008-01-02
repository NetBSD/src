/* $NetBSD: mainbus.c,v 1.1.2.2 2008/01/02 21:50:44 bouyer Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
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
__KERNEL_RCSID(0, "$NetBSD: mainbus.c,v 1.1.2.2 2008/01/02 21:50:44 bouyer Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/cons.h>

#include <machine/mainbus.h>

static int	mainbus_match(device_t, cfdata_t, void *);
static void	mainbus_attach(device_t, device_t, void *);

static int	mainbus_print(void *, const char *);

typedef struct mainbus_softc {
	device_t	sc_dev;
} mainbus_softc_t;

CFATTACH_DECL_NEW(mainbus, sizeof(mainbus_softc_t),
    mainbus_match, mainbus_attach, NULL, NULL);

static int
mainbus_match(device_t parent, cfdata_t match, void *opaque)
{

	return 1;
}

static void
mainbus_attach(device_t parent, device_t self, void *opaque)
{
	mainbus_softc_t *sc = device_private(self);
	struct thunkbus_attach_args taa;

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_dev = self;

	taa.taa_type = THUNKBUS_TYPE_CPU;
	config_found_ia(self, "thunkbus", &taa, mainbus_print);
	taa.taa_type = THUNKBUS_TYPE_CLOCK;
	config_found_ia(self, "thunkbus", &taa, mainbus_print);
	taa.taa_type = THUNKBUS_TYPE_TTYCONS;
	config_found_ia(self, "thunkbus", &taa, mainbus_print);
}

static int
mainbus_print(void *opaque, const char *pnp)
{
	if (pnp)
		aprint_normal("%s", pnp);
	return UNCONF;
}
