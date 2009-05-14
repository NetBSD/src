/*	$NetBSD: panel.c,v 1.1 2009/05/14 01:10:19 macallan Exp $ */

/*-
 * Copyright (c) 2009 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: panel.c,v 1.1 2009/05/14 01:10:19 macallan Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kthread.h>

#include <sys/bus.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/autoconf.h>
#include <machine/machtype.h>

#include <sgimips/ioc/iocreg.h>
#include <sgimips/hpc/hpcvar.h>
#include <sgimips/hpc/hpcreg.h>

struct panel_softc {
	device_t sc_dev;
	struct sysmon_pswitch sc_pbutton;
	bus_space_tag_t sc_tag;
	bus_space_handle_t sc_hreg;
	int sc_last, sc_fired;
};

static int panel_match(device_t, cfdata_t, void *);
static void panel_attach(device_t, device_t, void *);

static int panel_intr(void *);
static void panel_powerbutton(void *);

CFATTACH_DECL_NEW(panel, sizeof(struct panel_softc), 
				panel_match, 
				panel_attach, 
				NULL, 
				NULL);

static int
panel_match(device_t parent, cfdata_t match, void *aux)
{
	if (mach_type == MACH_SGI_IP22)
		return 1;

	return 0;
}

static void
panel_attach(device_t parent, device_t self, void *aux)
{
	struct panel_softc *sc;
	struct hpc_attach_args *haa;

	sc = device_private(self);
	sc->sc_dev = self;
	haa = aux;
	sc->sc_tag = haa->ha_st;
	sc->sc_fired = 0;
	
	aprint_normal("\n");
	if (bus_space_subregion(haa->ha_st, haa->ha_sh, haa->ha_devoff,
			0x1, 		/* just a single register */
			&sc->sc_hreg)) {
		aprint_error(": unable to map panel register\n");
		return;
	}

	if ((cpu_intr_establish(haa->ha_irq, IPL_BIO,
	     panel_intr, sc)) == NULL) {
		printf(": unable to establish interrupt!\n");
		return;
	}

	sc->sc_last = 0;

	sysmon_task_queue_init();
	memset(&sc->sc_pbutton, 0, sizeof(struct sysmon_pswitch));
	sc->sc_pbutton.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_pbutton.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_pbutton) != 0)
		aprint_error_dev(sc->sc_dev,
		    "unable to register power button with sysmon\n");
	pmf_device_register(self, NULL, NULL);
}

static int
panel_intr(void *cookie)
{
	struct panel_softc *sc = cookie;
	uint8_t reg;
	
	reg = bus_space_read_1(sc->sc_tag, sc->sc_hreg, 0);
	bus_space_write_1(sc->sc_tag, sc->sc_hreg, 0,
	    IOC_PANEL_VDOWN_IRQ | IOC_PANEL_VUP_IRQ | IOC_PANEL_POWER_IRQ);
	if ((reg & IOC_PANEL_POWER_IRQ) == 0) {
		if (!sc->sc_fired)
			sysmon_task_queue_sched(0, panel_powerbutton, sc);
		sc->sc_fired = 1;
	}
	if (time_second == sc->sc_last)
		return 1;
	sc->sc_last = time_second;
	if ((reg & IOC_PANEL_VDOWN_HOLD) == 0)
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_DOWN);
	if ((reg & IOC_PANEL_VUP_HOLD) == 0)
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_UP);
	
	return 1;
}

static void
panel_powerbutton(void *cookie)
{
	struct panel_softc *sc = cookie;

	sysmon_pswitch_event(&sc->sc_pbutton, PSWITCH_EVENT_PRESSED);
}
