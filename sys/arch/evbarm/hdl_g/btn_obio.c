/*	$NetBSD: btn_obio.c,v 1.1.10.2 2006/05/24 15:47:54 tron Exp $	*/

/*-
 * Copyright (c) 2005, 2006 NONAKA Kimihiro
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: btn_obio.c,v 1.1.10.2 2006/05/24 15:47:54 tron Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/callout.h>

#include <arm/xscale/i80321var.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <evbarm/hdl_g/hdlgreg.h>
#include <evbarm/hdl_g/obiovar.h>

struct btn_obio_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	void			*sc_ih;

	struct sysmon_pswitch	sc_smpsw[2];

	int			sc_mask;
};

static int btn_obio_match(struct device *, struct cfdata *, void *);
static void btn_obio_attach(struct device *, struct device *, void *);

static int btn_intr(void *aux);
static void btn_sysmon_pressed_event(void *arg);

CFATTACH_DECL(btn_obio, sizeof(struct btn_obio_softc),
    btn_obio_match, btn_obio_attach, NULL, NULL);

static int
btn_obio_match(struct device *parent, struct cfdata *cfp, void *aux)
{

	/* We take it on faith that the device is there. */
	return 1;
}

static void
btn_obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_attach_args *oba = aux;
	struct btn_obio_softc *sc = (void *)self;
	int error;

	sc->sc_iot = oba->oba_st;
	error = bus_space_map(sc->sc_iot, oba->oba_addr, 1, 0, &sc->sc_ioh);
	if (error) {
		aprint_error(": failed to map registers: %d\n", error);
		return;
	}

	sysmon_power_settype("landisk");
	sysmon_task_queue_init();

	/* power switch */
	sc->sc_smpsw[0].smpsw_name = device_xname(&sc->sc_dev);
	sc->sc_smpsw[0].smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->sc_smpsw[0]) != 0) {
		aprint_error(": unable to register power button with sysmon\n");
		return;
	}
	sc->sc_mask |= BTNSTAT_POWER;
	hdlg_enable_pldintr(INTEN_PWRSW);

	/* reset button */
	sc->sc_smpsw[1].smpsw_name = device_xname(&sc->sc_dev);
	sc->sc_smpsw[1].smpsw_type = PSWITCH_TYPE_RESET;
	if (sysmon_pswitch_register(&sc->sc_smpsw[1]) != 0) {
		aprint_error(": unable to register reset button with sysmon\n");
		return;
	}
	sc->sc_mask |= BTNSTAT_RESET;

	sc->sc_ih = i80321_intr_establish(oba->oba_irq, IPL_TTY, btn_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error(": couldn't establish intr handler");
		return;
	}
	hdlg_enable_pldintr(INTEN_BUTTON);

	aprint_normal("\n");
}

static int
btn_intr(void *arg)
{
	struct btn_obio_softc *sc = (void *)arg;
	int status;
	int rv;

	status = (int8_t)bus_space_read_1(sc->sc_iot, sc->sc_ioh, 0);
	if (status == -1) {
		return 0;
	}

	rv = 0;
	status = ~status;
	if (status & BTNSTAT_POWER) {
		if (sc->sc_mask & BTNSTAT_POWER) {
			hdlg_disable_pldintr(INTEN_PWRSW|INTEN_BUTTON);
			i80321_intr_disestablish(sc->sc_ih);
			sc->sc_ih = NULL;
			sysmon_task_queue_sched(0, btn_sysmon_pressed_event,
			    &sc->sc_smpsw[0]);
		} else {
			aprint_error("%s: power button pressed\n",
			    device_xname(&sc->sc_dev));
		}
		rv = 1;
	} else if (status & BTNSTAT_RESET) {
		if (sc->sc_mask & BTNSTAT_RESET) {
			hdlg_disable_pldintr(INTEN_PWRSW|INTEN_BUTTON);
			i80321_intr_disestablish(sc->sc_ih);
			sc->sc_ih = NULL;
			sysmon_task_queue_sched(0, btn_sysmon_pressed_event,
			    &sc->sc_smpsw[1]);
		} else {
			aprint_error("%s: reset button pressed\n",
			    device_xname(&sc->sc_dev));
		}
		rv = 1;
	}

	return rv;
}

static void
btn_sysmon_pressed_event(void *arg)
{
	struct sysmon_pswitch *smpsw = (void *)arg;

	sysmon_pswitch_event(smpsw, PSWITCH_EVENT_PRESSED);
}
