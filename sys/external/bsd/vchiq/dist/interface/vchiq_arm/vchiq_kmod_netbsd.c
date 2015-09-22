/* $NetBSD: vchiq_kmod_netbsd.c,v 1.3.2.1 2015/09/22 12:06:06 skrll Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared D. McNeill
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
__KERNEL_RCSID(0, "$NetBSD: vchiq_kmod_netbsd.c,v 1.3.2.1 2015/09/22 12:06:06 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_netbsd.h"

extern VCHIQ_STATE_T g_state;

struct vchiq_softc {
	device_t sc_dev;
	device_t sc_audiodev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;

	int sc_intr;
};

static struct vchiq_softc *vchiq_softc = NULL;

static int vchiq_match(device_t, cfdata_t, void *);
static void vchiq_attach(device_t, device_t, void *);

static int vchiq_intr(void *);
static int vchiq_print(void *, const char *);
static void vchiq_defer(device_t);

/* External functions */
int vchiq_init(void);

CFATTACH_DECL_NEW(vchiq, sizeof(struct vchiq_softc),
    vchiq_match, vchiq_attach, NULL, NULL);

static int
vchiq_match(device_t parent, cfdata_t match, void *aux)
{
	struct amba_attach_args *aaa = aux;
	
	if (strcmp(aaa->aaa_name, "bcmvchiq") != 0)
		return 0;

	return 1;
}

static void
vchiq_attach(device_t parent, device_t self, void *aux)
{
        struct vchiq_softc *sc = device_private(self);
	struct amba_attach_args *aaa = aux;

	aprint_naive("\n");
	aprint_normal(": BCM2835 VCHIQ\n");

	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;
	sc->sc_intr = aaa->aaa_intr;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, aaa->aaa_size, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(self, "unable to map device\n");
		return;
	}

	vchiq_softc = sc;

	config_mountroot(self, vchiq_defer);
}

static void
vchiq_defer(device_t self)
{
	struct vchiq_attach_args vaa;
	struct vchiq_softc *sc = device_private(self);

	vchiq_core_initialize();

	sc->sc_ih = intr_establish(sc->sc_intr, IPL_SCHED, IST_LEVEL,
	    vchiq_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    sc->sc_intr);
		return;
	}

	vchiq_init();

	vaa.vaa_name = "AUDS";
	config_found_ia(self, "vchiqbus", &vaa, vchiq_print);
}

static int
vchiq_intr(void *priv)
{
	struct vchiq_softc *sc = priv;
	uint32_t status;

	status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, 0x40);
	if (status & 0x4)
		remote_event_pollall(&g_state);

	bus_space_barrier(vchiq_softc->sc_iot, vchiq_softc->sc_ioh,
	    0x40, 4, BUS_SPACE_BARRIER_READ);

	return 1;
}

static int
vchiq_print(void *aux, const char *pnp)
{
	struct vchiq_attach_args *vaa = aux;

	if (pnp)
		aprint_normal("%s at %s", vaa->vaa_name, pnp);

	return UNCONF;
}

void
remote_event_signal(REMOTE_EVENT_T *event)
{
	wmb();

	event->fired = 1;

	dsb();		/* data barrier operation */

	if (event->armed) {
		bus_space_barrier(vchiq_softc->sc_iot, vchiq_softc->sc_ioh,
		    0x48, 4, BUS_SPACE_BARRIER_WRITE);
		bus_space_write_4(vchiq_softc->sc_iot, vchiq_softc->sc_ioh,
		    0x48, 0);
	}
}
