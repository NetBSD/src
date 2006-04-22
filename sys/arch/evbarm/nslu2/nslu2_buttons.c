/*	$NetBSD: nslu2_buttons.c,v 1.2.8.2 2006/04/22 11:37:24 simonb Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: nslu2_buttons.c,v 1.2.8.2 2006/04/22 11:37:24 simonb Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/nslu2/nslu2reg.h>

struct slugbutt_softc {
	struct device sc_dev;
	struct sysmon_pswitch sc_smpwr;
	struct sysmon_pswitch sc_smrst;
};

static int slugbutt_attached;

#define	SLUGBUTT_PWR_BIT	(1u << GPIO_BUTTON_PWR)
#define	SLUGBUTT_RST_BIT	(1u << GPIO_BUTTON_RST)

static void
power_event(void *arg)
{
	struct slugbutt_softc *sc = arg;

	sysmon_pswitch_event(&sc->sc_smpwr, PSWITCH_EVENT_PRESSED);
}

static int
power_intr(void *arg)
{
	struct slugbutt_softc *sc = arg;
	int rv;

	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPISR, SLUGBUTT_PWR_BIT);

	rv = sysmon_task_queue_sched(0, power_event, sc);
	if (rv) {
		printf("%s: WARNING: unable to queue power button "
		    "callback: %d\n", sc->sc_dev.dv_xname, rv);
	}

	return (1);
}

static void
reset_event(void *arg)
{
	struct slugbutt_softc *sc = arg;

	sysmon_pswitch_event(&sc->sc_smrst, PSWITCH_EVENT_PRESSED);
}

static int
reset_intr(void *arg)
{
	struct slugbutt_softc *sc = arg;
	int rv;

	GPIO_CONF_WRITE_4(ixp425_softc, IXP425_GPIO_GPISR, SLUGBUTT_RST_BIT);

	rv = sysmon_task_queue_sched(0, reset_event, sc);
	if (rv) {
		printf("%s: WARNING: unable to queue reset button "
		    "callback: %d\n", sc->sc_dev.dv_xname, rv);
	}

	return (1);
}

static void
slugbutt_deferred(struct device *self)
{
	struct slugbutt_softc *sc = (struct slugbutt_softc *) self;
	struct ixp425_softc *ixsc = ixp425_softc;
	uint32_t reg;

	/* Configure the GPIO pins as inputs */
	reg = GPIO_CONF_READ_4(ixsc, IXP425_GPIO_GPOER);
	reg |= SLUGBUTT_PWR_BIT | SLUGBUTT_RST_BIT;
	GPIO_CONF_WRITE_4(ixsc, IXP425_GPIO_GPOER, reg);

	/* Configure the input type: Falling edge */
	reg = GPIO_CONF_READ_4(ixsc, GPIO_TYPE_REG(GPIO_BUTTON_PWR));
	reg &= ~GPIO_TYPE(GPIO_BUTTON_PWR, GPIO_TYPE_MASK);
	reg |= GPIO_TYPE(GPIO_BUTTON_PWR, GPIO_TYPE_EDG_FALLING);
	GPIO_CONF_WRITE_4(ixsc, GPIO_TYPE_REG(GPIO_BUTTON_PWR), reg);

	reg = GPIO_CONF_READ_4(ixsc, GPIO_TYPE_REG(GPIO_BUTTON_RST));
	reg &= ~GPIO_TYPE(GPIO_BUTTON_RST, GPIO_TYPE_MASK);
	reg |= GPIO_TYPE(GPIO_BUTTON_RST, GPIO_TYPE_EDG_FALLING);
	GPIO_CONF_WRITE_4(ixsc, GPIO_TYPE_REG(GPIO_BUTTON_RST), reg);

	/* Clear any existing interrupt */
	GPIO_CONF_WRITE_4(ixsc, IXP425_GPIO_GPISR, SLUGBUTT_PWR_BIT |
	    SLUGBUTT_RST_BIT);

	sysmon_task_queue_init();

	sc->sc_smpwr.smpsw_name = sc->sc_dev.dv_xname;
	sc->sc_smpwr.smpsw_type = PSWITCH_TYPE_POWER;

	if (sysmon_pswitch_register(&sc->sc_smpwr) != 0) {
		printf("%s: unable to register power button with sysmon\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_smrst.smpsw_name = sc->sc_dev.dv_xname;
	sc->sc_smrst.smpsw_type = PSWITCH_TYPE_RESET;

	if (sysmon_pswitch_register(&sc->sc_smrst) != 0) {
		printf("%s: unable to register reset button with sysmon\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Hook the interrupts */
	ixp425_intr_establish(BUTTON_PWR_INT, IPL_TTY, power_intr, sc);
	ixp425_intr_establish(BUTTON_RST_INT, IPL_TTY, reset_intr, sc);
}


static int
slugbutt_match(struct device *parent, struct cfdata *cf, void *arg)
{

	return (slugbutt_attached == 0);
}

static void
slugbutt_attach(struct device *parent, struct device *self, void *arg)
{

	slugbutt_attached = 1;

	aprint_normal(": Power and Reset buttons\n");

	/* Defer, to ensure ixp425_softc has been initialised */
	config_interrupts(self, slugbutt_deferred);
}

CFATTACH_DECL(slugbutt, sizeof(struct slugbutt_softc),
    slugbutt_match, slugbutt_attach, NULL, NULL);
