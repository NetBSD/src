/* $NetBSD: vchiq_kmod_netbsd.c,v 1.10 2017/12/10 21:38:27 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: vchiq_kmod_netbsd.c,v 1.10 2017/12/10 21:38:27 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/sysctl.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#include <dev/fdt/fdtvar.h>

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
	int sc_phandle;
};

static struct vchiq_softc *vchiq_softc = NULL;

static int vchiq_match(device_t, cfdata_t, void *);
static void vchiq_attach(device_t, device_t, void *);

static int vchiq_intr(void *);
static int vchiq_print(void *, const char *);
static void vchiq_defer(device_t);

/* External functions */
int vchiq_init(void);


#define VCHIQ_DOORBELL0		0x0
#define VCHIQ_DOORBELL1		0x4
#define VCHIQ_DOORBELL2		0x8
#define VCHIQ_DOORBELL3		0xC


CFATTACH_DECL_NEW(vchiq, sizeof(struct vchiq_softc),
    vchiq_match, vchiq_attach, NULL, NULL);

static int
vchiq_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-vchiq", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
vchiq_attach(device_t parent, device_t self, void *aux)
{
	struct vchiq_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal(": BCM2835 VCHIQ\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_phandle = phandle;

	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}

	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	vchiq_platform_attach(faa->faa_dmat);

	vchiq_softc = sc;

	config_mountroot(self, vchiq_defer);
}

static void
vchiq_defer(device_t self)
{
	struct vchiq_attach_args vaa;
	struct vchiq_softc *sc = device_private(self);
	const int phandle = sc->sc_phandle;

	vchiq_core_initialize();

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_VM, FDT_INTR_MPSAFE,
	    vchiq_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	vchiq_init();

	vaa.vaa_name = "AUDS";
	config_found_ia(self, "vchiqbus", &vaa, vchiq_print);
}

static int
vchiq_intr(void *priv)
{
	struct vchiq_softc *sc = priv;
	uint32_t status;

	bus_space_barrier(sc->sc_iot, sc->sc_ioh,
	    VCHIQ_DOORBELL0, 4, BUS_SPACE_BARRIER_READ);

	rmb();
	status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, VCHIQ_DOORBELL0);
	if (status & 0x4) {
		remote_event_pollall(&g_state);
		return 1;
	}

	return 0;
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
		bus_space_write_4(vchiq_softc->sc_iot, vchiq_softc->sc_ioh,
		    VCHIQ_DOORBELL2, 0);
		bus_space_barrier(vchiq_softc->sc_iot, vchiq_softc->sc_ioh,
		    VCHIQ_DOORBELL2, 4, BUS_SPACE_BARRIER_WRITE);
	}
}

SYSCTL_SETUP(sysctl_hw_vchiq_setup, "sysctl hw.vchiq setup")
{
	const struct sysctlnode *rnode = NULL;
	const struct sysctlnode *cnode = NULL;

	sysctl_createv(clog, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "vchiq", NULL,
	    NULL, 0, NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "loglevel", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &cnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "core", "VChiq Core Loglevel", NULL, 0,
	    &vchiq_core_log_level, 0, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &cnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "coremsg", "VChiq Core Message Loglevel", NULL, 0,
	    &vchiq_core_msg_log_level, 0, CTL_CREATE, CTL_EOL);

	sysctl_createv(clog, 0, &cnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "sync", "VChiq Sync Loglevel", NULL, 0,
	    &vchiq_sync_log_level, 0, CTL_CREATE, CTL_EOL);
}
