/* $NetBSD: vchiq_kmod_netbsd.c,v 1.12 2020/09/26 12:58:23 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: vchiq_kmod_netbsd.c,v 1.12 2020/09/26 12:58:23 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/sysctl.h>

#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#include "vchiq_arm.h"
#include "vchiq_2835.h"
#include "vchiq_netbsd.h"

extern VCHIQ_STATE_T g_state;

static struct vchiq_softc *vchiq_softc = NULL;

#define VCHIQ_DOORBELL0		0x0
#define VCHIQ_DOORBELL1		0x4
#define VCHIQ_DOORBELL2		0x8
#define VCHIQ_DOORBELL3		0xC

void
vchiq_set_softc(struct vchiq_softc *sc)
{
	vchiq_softc = sc;
}

int
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

int
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

	dsb(sy);		/* data barrier operation */

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
