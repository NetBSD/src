/*	$NetBSD: acpi_lid.c,v 1.2 2001/09/29 05:36:49 thorpej Exp $	*/

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
 * ACPI Lid Switch driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

struct acpilid_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
};

#define	ACPILID_F_VERBOSE		0x01	/* verbose events */

int	acpilid_match(struct device *, struct cfdata *, void *);
void	acpilid_attach(struct device *, struct device *, void *);

struct cfattach acpilid_ca = {
	sizeof(struct acpilid_softc), acpilid_match, acpilid_attach,
};

void	acpilid_status_changed(void *);
void	acpilid_notify_handler(ACPI_HANDLE, UINT32, void *context);

/*
 * acpilid_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
acpilid_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "PNP0C0D") == 0)
		return (1);

	return (0);
}

/*
 * acpilid_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
void
acpilid_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpilid_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	printf(": ACPI Lid Switch\n");

	sc->sc_node = aa->aa_node;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpilid_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register device notify handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	/* Display the current state when it changes. */
	sc->sc_flags = ACPILID_F_VERBOSE;
}

/*
 * acpilid_status_changed:
 *
 *	Get, and possibly display, the lid status, and take action.
 */
void
acpilid_status_changed(void *arg)
{
	struct acpilid_softc *sc = arg;
	int status;

	if (acpi_eval_integer(sc->sc_node->ad_handle, "_LID",
	    &status) != AE_OK)
		return;

	if (sc->sc_flags & ACPILID_F_VERBOSE)
		printf("%s: Lid %s\n",
		    sc->sc_dev.dv_xname,
		    status == 0 ? "closed " : "open");

	/*
	 * XXX Perform the appropriate action for the new lid status.
	 */
}

/*
 * acpilid_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
void
acpilid_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	struct acpilid_softc *sc = context;
	int rv;

	switch (notify) {
	case ACPI_NOTIFY_LidStatusChanged:
#ifdef ACPI_LID_DEBUG
		printf("%s: received LidStatusChanged message\n",
		    sc->sc_dev.dv_xname);
#endif
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
		    acpilid_status_changed, sc);
		if (rv != AE_OK)
			printf("%s: WARNING: unable to queue lid change "
			    "event: %d\n", sc->sc_dev.dv_xname, rv);
		break;

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}
