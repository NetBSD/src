/*	$NetBSD: pwrsw_obio.c,v 1.3 2012/01/21 19:44:29 nonaka Exp $	*/

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

#include "btn_obio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pwrsw_obio.c,v 1.3 2012/01/21 19:44:29 nonaka Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/conf.h>
#include <sys/ioctl.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <sh3/devreg.h>

#include <landisk/landisk/landiskreg.h>
#include <landisk/dev/obiovar.h>

struct pwrsw_obio_softc {
	device_t		sc_dev;
	void			*sc_ih;

	struct sysmon_pswitch	sc_smpsw; /* our sysmon glue */

	int			sc_flags;
#define	SYSMON_ATTACHED	1
};

static int pwrsw_obio_probe(device_t, cfdata_t, void *);
static void pwrsw_obio_attach(device_t, device_t, void *);

static int pwrsw_intr(void *aux);
static void pwrsw_pressed_event(void *arg);

CFATTACH_DECL_NEW(pwrsw_obio, sizeof(struct pwrsw_obio_softc),
    pwrsw_obio_probe, pwrsw_obio_attach, NULL, NULL);

static struct pwrsw_obio_softc *pwrsw_softc;

static int
pwrsw_obio_probe(device_t parent, cfdata_t cfp, void *aux)
{
	struct obio_attach_args *oa = aux;

	if (pwrsw_softc)
		return (0);

	oa->oa_nio = 0;
	oa->oa_niomem = 0;
	oa->oa_nirq = 1;
	oa->oa_irq[0].or_irq = LANDISK_INTR_PWRSW;

	return (1);
}

static void
pwrsw_obio_attach(device_t parent, device_t self, void *aux)
{
	struct pwrsw_obio_softc *sc;

	aprint_naive("\n");
	aprint_normal(": Power Switch\n");

	sc = device_private(self);
	sc->sc_dev = self;

	pwrsw_softc = sc;

	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_POWER;

	sc->sc_ih = extintr_establish(LANDISK_INTR_PWRSW, IPL_TTY,
	    pwrsw_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		panic("extintr_establish");
	}

	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		return;
	}
	sc->sc_flags |= SYSMON_ATTACHED;
}

static int
pwrsw_intr(void *arg)
{
	struct pwrsw_obio_softc *sc = arg;
	int status;

	status = (int8_t)_reg_read_1(LANDISK_BTNSTAT);
	if (status == -1) {
		return (0);
	}

	status = ~status;
	if (status & BTN_POWER_BIT) {
		if (sc->sc_flags & SYSMON_ATTACHED) {
			sysmon_task_queue_sched(0, pwrsw_pressed_event, sc);
			extintr_disable(sc->sc_ih);
#if NBTN_OBIO > 0
			extintr_disable_by_num(LANDISK_INTR_BTN);
#endif
		} else {
			aprint_normal_dev(sc->sc_dev, "pressed\n");
		}
		_reg_write_1(LANDISK_PWRSW_INTCLR, 1);
		return (1);
	}
	return (0);
}

static void
pwrsw_pressed_event(void *arg)
{
	struct pwrsw_obio_softc *sc = arg;

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}
