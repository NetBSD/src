/*	$NetBSD: acpi_button.c,v 1.1.4.2 2001/10/08 21:18:05 nathanw Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define	ACPI_BUT_DEBUG

struct acpibut_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_buttype;			/* button type */
	int sc_flags;			/* see below */
};

#define	ACPIBUT_TYPE_POWER		0
#define	ACPIBUT_TYPE_SLEEP		1

#define	ACPIBUT_F_VERBOSE		0x01	/* verbose events */

int	acpibut_match(struct device *, struct cfdata *, void *);
void	acpibut_attach(struct device *, struct device *, void *);

struct cfattach acpibut_ca = {
	sizeof(struct acpibut_softc), acpibut_match, acpibut_attach,
};

void	acpibut_pressed_for_sleep(void *);
void	acpibut_pressed_for_wakeup(void *);
void	acpibut_notify_handler(ACPI_HANDLE, UINT32, void *context);

/*
 * acpibut_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
acpibut_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "PNP0C0C") == 0 ||
	    strcmp(aa->aa_node->ad_devinfo.HardwareId, "PNP0C0E") == 0)
		return (1);

	return (0);
}

/*
 * acpibut_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
void
acpibut_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpibut_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	const char *desc;
	ACPI_STATUS rv;

	if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "PNP0C0C") == 0) {
		sc->sc_buttype = ACPIBUT_TYPE_POWER;
		desc = "Power";
	} else if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "PNP0C0E") == 0) {
		sc->sc_buttype = ACPIBUT_TYPE_SLEEP;
		desc = "Sleep";
	} else {
		printf("\n");
		panic("acpibut_attach: impossible");
	}

	printf(": ACPI %s Button\n", desc);

	sc->sc_node = aa->aa_node;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpibut_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register device notify handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

#ifdef ACPI_BUT_DEBUG
	/* Display the current state when it changes. */
	sc->sc_flags = ACPIBUT_F_VERBOSE;
#endif
}

/*
 * acpibut_pressed_for_sleep:
 *
 *	Deal with a button being pressed for sleep.
 */
void
acpibut_pressed_for_sleep(void *arg)
{
	struct acpibut_softc *sc = arg;

	if (sc->sc_flags & ACPIBUT_F_VERBOSE)
		printf("%s: button pressed for sleep\n",
		    sc->sc_dev.dv_xname);

	/*
	 * XXX Perform the appropriate action.
	 */
}

/*
 * acpibut_pressed_for_wakeup:
 *
 *	Deal with a button being pressed for wakeup.
 */
void
acpibut_pressed_for_wakeup(void *arg)
{
	struct acpibut_softc *sc = arg;

	if (sc->sc_flags & ACPIBUT_F_VERBOSE)
		printf("%s: button pressed for wakeup\n",
		    sc->sc_dev.dv_xname);

	/*
	 * XXX Perform the appropriate action.
	 */
}

/*
 * acpibut_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
void
acpibut_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
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
		    acpibut_pressed_for_sleep, sc);
		if (rv != AE_OK)
			printf("%s: WARNING: unable to queue button pressed "
			    "event: %d\n", sc->sc_dev.dv_xname, rv);
		break;

	/* XXX ACPI_NOTIFY_DeviceWake?? */

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}
