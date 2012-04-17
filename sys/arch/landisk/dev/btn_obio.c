/*	$NetBSD: btn_obio.c,v 1.5.38.1 2012/04/17 00:06:34 yamt Exp $	*/

/*-
 * Copyright (C) 2005 NONAKA Kimihiro <nonaka@netbsd.org>
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

#include "pwrsw_obio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: btn_obio.c,v 1.5.38.1 2012/04/17 00:06:34 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/callout.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <sh3/devreg.h>

#include <machine/button.h>

#include <landisk/landisk/landiskreg.h>
#include <landisk/dev/obiovar.h>
#include <landisk/dev/buttonvar.h>

#define	BTN_SELECT	0
#define	BTN_COPY	1
#define	BTN_REMOVE	2
#define	NBUTTON		3

struct btn_obio_softc {
	device_t		sc_dev;
	void			*sc_ih;

	struct callout		sc_guard_ch;
	struct sysmon_pswitch	sc_smpsw;	/* reset */
	struct btn_event	sc_bev[NBUTTON];

	int			sc_mask;
};

#ifndef	BTN_TIMEOUT
#define	BTN_TIMEOUT	(hz / 10)	/* 100ms */
#endif

static int btn_obio_probe(device_t, cfdata_t, void *);
static void btn_obio_attach(device_t, device_t, void *);

static int btn_intr(void *aux);
static void btn_sysmon_pressed_event(void *arg);
static void btn_pressed_event(void *arg);
static void btn_guard_timeout(void *arg);

static struct btn_obio_softc *btn_softc;

static const struct {
	int idx;
	int mask;
	const char *name;
} btnlist[NBUTTON] = {
	{ BTN_SELECT,	BTN_SELECT_BIT, "select" },
	{ BTN_COPY,	BTN_COPY_BIT,	"copy"   },
	{ BTN_REMOVE,	BTN_REMOVE_BIT,	"remove" },
};

CFATTACH_DECL_NEW(btn_obio, sizeof(struct btn_obio_softc),
    btn_obio_probe, btn_obio_attach, NULL, NULL);

static int
btn_obio_probe(device_t parent, cfdata_t cfp, void *aux)
{
	struct obio_attach_args *oa = aux;

	if (btn_softc)
		return (0);

	oa->oa_nio = 0;
	oa->oa_niomem = 0;
	oa->oa_nirq = 1;
	oa->oa_irq[0].or_irq = LANDISK_INTR_BTN;

	return (1);
}

static void
btn_obio_attach(device_t parent, device_t self, void *aux)
{
	struct btn_obio_softc *sc;
	int i;

	aprint_naive("\n");
	aprint_normal(": USL-5P buttons\n");

	sc = device_private(self);
	sc->sc_dev = self;

	btn_softc = sc;

	callout_init(&sc->sc_guard_ch, 0);
	callout_setfunc(&sc->sc_guard_ch, btn_guard_timeout, sc);

	sc->sc_ih = extintr_establish(LANDISK_INTR_BTN, IPL_TTY, btn_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		panic("extintr_establish");
	}

	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_RESET;
	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		return;
	}
	sc->sc_mask |= BTN_RESET_BIT;

	for (i = 0; i < NBUTTON; i++) {
		int idx = btnlist[i].idx;
		sc->sc_bev[idx].bev_name = btnlist[i].name;
		if (btn_event_register(&sc->sc_bev[idx]) != 0) {
			aprint_error_dev(self,
					 "unable to register '%s' button\n",
					 btnlist[i].name);
		} else {
			sc->sc_mask |= btnlist[i].mask;
		}
	}
}

static int
btn_intr(void *arg)
{
	struct btn_obio_softc *sc = (void *)arg;
	device_t self = sc->sc_dev;
	int status;
	int i;

	status = (int8_t)_reg_read_1(LANDISK_BTNSTAT);
	if (status == -1) {
		return (0);
	}

	status = ~status;
	if (status & BTN_ALL_BIT) {
		if (status & BTN_RESET_BIT) {
			if (sc->sc_mask & BTN_RESET_BIT) {
				extintr_disable(sc->sc_ih);
#if NPWRSW_OBIO > 0
				extintr_disable_by_num(LANDISK_INTR_PWRSW);
#endif
				sysmon_task_queue_sched(0,
				    btn_sysmon_pressed_event, sc);
				return (1);
			} else {
				aprint_normal_dev(self,
					"reset button pressed\n");
			}
		}

		for (i = 0; i < NBUTTON; i++) {
			uint8_t mask = btnlist[i].mask;
			int rv = 0;
			if (status & mask) {
				if (sc->sc_mask & mask) {
					sysmon_task_queue_sched(1,
					    btn_pressed_event,
					    &sc->sc_bev[btnlist[i].idx]);
				} else {
					aprint_normal_dev(self,
						"%s button pressed\n",
						btnlist[i].name);
				}
				rv = 1;
			}
			if (rv != 0) {
				extintr_disable(sc->sc_ih);
				callout_schedule(&sc->sc_guard_ch, BTN_TIMEOUT);
			}
		}
		return (1);
	}
	return (0);
}

static void
btn_sysmon_pressed_event(void *arg)
{
	struct btn_obio_softc *sc = (void *)arg;

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

static void
btn_pressed_event(void *arg)
{
	struct btn_event *bev = (void *)arg;

	btn_event_send(bev, BUTTON_EVENT_PRESSED);
}

static void
btn_guard_timeout(void *arg)
{
	struct btn_obio_softc *sc = arg;

	callout_stop(&sc->sc_guard_ch);
	extintr_enable(sc->sc_ih);
}
