/*	$NetBSD: acpi_acad.c,v 1.1.2.2 2001/10/01 12:44:14 fvdl Exp $	*/

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
 * ACPI AC Adapter driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

struct acpiacad_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	int sc_status;			/* power status */
};

#define	AACAD_F_VERBOSE		0x01	/* verbose events */

int	acpiacad_match(struct device *, struct cfdata *, void *);
void	acpiacad_attach(struct device *, struct device *, void *);

struct cfattach acpiacad_ca = {
	sizeof(struct acpiacad_softc), acpiacad_match, acpiacad_attach,
};

void	acpiacad_get_status(void *);
void	acpiacad_notify_handler(ACPI_HANDLE, UINT32, void *context);

/*
 * acpiacad_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
acpiacad_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "ACPI0003") == 0)
		return (1);

	return (0);
}

/*
 * acpiacad_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
void
acpiacad_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpiacad_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	printf(": ACPI AC Adapter\n");

	sc->sc_node = aa->aa_node;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpiacad_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register DEVICE NOTIFY handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	/* XXX See acpiacad_notify_handler() */
	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_SYSTEM_NOTIFY, acpiacad_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register SYSTEM NOTIFY handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	/* Display the current state. */
	sc->sc_flags = AACAD_F_VERBOSE;
	acpiacad_get_status(sc);

	/*
	 * XXX Hook into sysmon here.
	 */
}

/*
 * acpiacad_get_status:
 *
 *	Get, and possibly display, the current AC line status.
 */
void
acpiacad_get_status(void *arg)
{
	struct acpiacad_softc *sc = arg;

	if (acpi_eval_integer(sc->sc_node->ad_handle, "_PSR",
	    &sc->sc_status) != AE_OK)
		return;

	if (sc->sc_flags & AACAD_F_VERBOSE)
		printf("%s: AC adapter %sconnected\n",
		    sc->sc_dev.dv_xname, sc->sc_status == 0 ? "not " : "");
}

/*
 * acpiacad_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
void
acpiacad_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	struct acpiacad_softc *sc = context;
	int rv;

	switch (notify) {
	/*
	 * XXX So, BusCheck is not exactly what I would expect,
	 * but at least my IBM T21 sends it on AC adapter status
	 * change.  --thorpej@wasabisystems.com
	 */
	case ACPI_NOTIFY_BusCheck:
	case ACPI_NOTIFY_PowerSourceStatusChanged:
#ifdef ACPI_ACAD_DEBUG
		printf("%s: received notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
#endif
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
		    acpiacad_get_status, sc);
		if (rv != AE_OK)
			printf("%s: unable to queue status check: %d\n",
			    sc->sc_dev.dv_xname, rv);
		break;

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}
