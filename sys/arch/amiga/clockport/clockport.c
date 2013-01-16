/*      $NetBSD: clockport.c,v 1.2.2.4 2013/01/16 05:32:40 yamt Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/termios.h> 

#include <sys/bus.h>
#include <sys/intr.h>

#include <amiga/clockport/clockportvar.h>

static int	clockport_match(device_t, cfdata_t , void *);
static void	clockport_attach(device_t, device_t, void *);
static int	clockport_print(void *, const char *);
static int	clockport_submatch(device_t, cfdata_t, const int *, void *);

CFATTACH_DECL_NEW(clockport, sizeof(struct clockportbus_softc),
    clockport_match, clockport_attach, NULL, NULL);

static int
clockport_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
clockport_attach(device_t parent, device_t self, void *aux)
{
	struct clockportbus_softc *sc;

        aprint_normal("\n");

	sc = device_private(self);
	sc->sc_dev = self;
	sc->cpb_aa = (struct clockportbus_attach_args *) aux;

	config_search_ia(clockport_submatch, self, "clockport", 0);
}

static int
clockport_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct clockportbus_softc *sc;
	struct clockport_attach_args a; 

	sc = device_private(parent);

	/* XXX: copy bus_space_tag and intr routine for now... */
	a.cp_iot = sc->cpb_aa->cp_iot;
	a.cp_intr_establish = sc->cpb_aa->cp_intr_establish;

	if (config_match(parent, cf, &a)) {
		config_attach(parent, cf, &a, clockport_print);
		return 1;
	}

	return 0;
}

static int
clockport_print(void *aux, const char *str)
{

        if (str == NULL)
                return 0;

        printf("%s ", str);

        return 0;
}
