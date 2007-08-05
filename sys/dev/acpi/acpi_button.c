/*	$NetBSD: acpi_button.c,v 1.22.22.1 2007/08/05 19:01:00 jmcneill Exp $	*/

/*
 * Copyright 2001, 2003 Wasabi Systems, Inc.
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
 * ACPI Button driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_button.c,v 1.22.22.1 2007/08/05 19:01:00 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

struct acpibut_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	struct sysmon_pswitch sc_smpsw;	/* our sysmon glue */
	int sc_flags;			/* see below */
};

static const char * const power_button_hid[] = {
	"PNP0C0C",
	NULL
};

static const char * const sleep_button_hid[] = {
	"PNP0C0E",
	NULL
};

#define	ACPIBUT_F_VERBOSE		0x01	/* verbose events */

static int	acpibut_match(struct device *, struct cfdata *, void *);
static void	acpibut_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpibut, sizeof(struct acpibut_softc),
    acpibut_match, acpibut_attach, NULL, NULL);

static void	acpibut_pressed_event(void *);
static void	acpibut_notify_handler(ACPI_HANDLE, UINT32, void *);

/*
 * acpibut_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpibut_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, power_button_hid))
		return 1;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, sleep_button_hid))
		return 1;

	return 0;
}

/*
 * acpibut_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
static void
acpibut_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpibut_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	const char *desc;
	ACPI_STATUS rv;

	sc->sc_smpsw.smpsw_name = sc->sc_dev.dv_xname;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, power_button_hid)) {
		sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_POWER;
		desc = "Power";
	} else if (acpi_match_hid(aa->aa_node->ad_devinfo, sleep_button_hid)) {
		sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_SLEEP;
		desc = "Sleep";
	} else {
		printf("\n");
		panic("acpibut_attach: impossible");
	}

	aprint_naive(": ACPI %s Button\n", desc);
	aprint_normal(": ACPI %s Button\n", desc);

	sc->sc_node = aa->aa_node;

	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpibut_notify_handler, sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to register DEVICE NOTIFY handler: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}

	acpi_set_wake_gpe(sc->sc_node->ad_handle);

#ifdef ACPI_BUT_DEBUG
	/* Display the current state when it changes. */
	sc->sc_flags = ACPIBUT_F_VERBOSE;
#endif

	(void)pnp_register(self, pnp_generic_power);
}

/*
 * acpibut_pressed_event:
 *
 *	Deal with a button being pressed.
 */
static void
acpibut_pressed_event(void *arg)
{
	struct acpibut_softc *sc = arg;

	if (sc->sc_flags & ACPIBUT_F_VERBOSE)
		printf("%s: button pressed\n", sc->sc_dev.dv_xname);

	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

/*
 * acpibut_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpibut_notify_handler(ACPI_HANDLE handle, UINT32 notify,
    void *context)
{
	struct acpibut_softc *sc = context;
	int rv;

	switch (notify) {
	case ACPI_NOTIFY_S0PowerButtonPressed:
#if 0
	case ACPI_NOTIFY_S0SleepButtonPressed: /* same as above */
#endif
#ifdef ACPI_BUT_DEBUG
		printf("%s: received ButtonPressed message\n",
		    sc->sc_dev.dv_xname);
#endif
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
		    acpibut_pressed_event, sc);
		if (ACPI_FAILURE(rv))
			printf("%s: WARNING: unable to queue button pressed "
			    "callback: %s\n", sc->sc_dev.dv_xname,
			    AcpiFormatException(rv));
		break;

	/* XXX ACPI_NOTIFY_DeviceWake?? */

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}
