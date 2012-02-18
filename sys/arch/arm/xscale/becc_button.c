/*	$NetBSD: becc_button.c,v 1.3.118.1 2012/02/18 07:31:33 mrg Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for the reset button on the ADI Engineering, Inc. Big Endian
 * Companion Chip.
 *
 * When pressed for two seconds, the reset button performs a hard reset
 * of the system.  Shorter presses result in an interrupt being generated,
 * which the operating system can use to trigger a graceful reboot.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: becc_button.c,v 1.3.118.1 2012/02/18 07:31:33 mrg Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <arm/xscale/beccreg.h>
#include <arm/xscale/beccvar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

static int beccbut_attached;	/* there can be only one */

struct beccbut_softc {
	device_t sc_dev;
	struct sysmon_pswitch sc_smpsw;
	void *sc_ih;
};

static void
beccbut_pressed_event(void *arg)
{
	struct beccbut_softc *sc = arg;

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

static int
beccbut_intr(void *arg)
{
	struct beccbut_softc *sc = arg;
	int rv;

	rv = sysmon_task_queue_sched(0, beccbut_pressed_event, sc);
	if (rv != 0)
		printf("%s: WARNING: unable to queue button pressed "
		    "callback: %d\n", device_xname(sc->sc_dev), rv);

	return (1);
}

static int
beccbut_match(device_t parent, cfdata_t match, void *aux)
{

	return (beccbut_attached == 0);
}

static void
beccbut_attach(device_t parent, device_t self, void *aux)
{
	struct beccbut_softc *sc = device_private(self);

	aprint_normal(": Reset button\n");
	aprint_naive(": Reset button\n");

	beccbut_attached = 1;
	sc->sc_dev = self;

	sysmon_task_queue_init();

	sc->sc_smpsw.smpsw_name = device_xname(sc->sc_dev);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_RESET;

	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to register with sysmon\n");
		return;
	}

	sc->sc_ih = becc_intr_establish(ICU_PUSHBUTTON, IPL_TTY,
	    beccbut_intr, sc);
	if (sc->sc_ih == NULL)
		aprint_error_dev(sc->sc_dev,
		    "unable to establish interrupt handler\n");
}

CFATTACH_DECL_NEW(beccbut, sizeof(struct beccbut_softc),
    beccbut_match, beccbut_attach, NULL, NULL);
