/*	$NetBSD: acpi_bat.c,v 1.1 2002/03/24 03:46:10 sommerfeld Exp $	*/

/*
 * Copyright 2001 Bill Sommerfeld.
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

#define ACPI_BAT_DEBUG

/*
 * ACPI Battery Driver.
 *
 * ACPI defines two different battery device interfaces: "Control
 * Method" batteries, in which AML methods are defined in order to get
 * battery status and set battery alarm thresholds, and a "Smart
 * Battery" device, which is an SMbus device accessed through the ACPI
 * Embedded Controller device.
 *
 * This driver is for the "Control Method"-style battery only.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_bat.c,v 1.1 2002/03/24 03:46:10 sommerfeld Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* for hz */
#include <sys/device.h>
#include <sys/callout.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

struct acpibat_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	struct callout sc_callout; 	/* XXX temporary polling */
	int sc_status;			/* power status */
	int sc_rate;			/* current drain rate */
	int sc_capacity;		/* current capacity */
	int sc_mv;			/* current potential in mV */
	int sc_design_capacity;		/* design capacity */
	int sc_pred_capacity;		/* estimated current max */
	int sc_warn_capacity;		/* warning level */
	int sc_low_capacity;		/* low level */
};

#define	ABAT_F_VERBOSE		0x01	/* verbose events */
#define ABAT_F_PWRUNIT_MA	0x02 	/* mA instead of mW */

#define ACM_RATEUNIT(sc) (((sc)->sc_flags & ABAT_F_PWRUNIT_MA)?"mA":"mW")
#define ACM_CAPUNIT(sc) (((sc)->sc_flags & ABAT_F_PWRUNIT_MA)?"mAh":"mWh")

int	acpibat_match(struct device *, struct cfdata *, void *);
void	acpibat_attach(struct device *, struct device *, void *);

struct cfattach acpibat_ca = {
	sizeof(struct acpibat_softc), acpibat_match, acpibat_attach,
};

static void acpibat_get_status(void *);
static void acpibat_get_info(void *);
void	acpibat_notify_handler(ACPI_HANDLE, UINT32, void *context);
static void acpibat_tick(void *);

/*
 * acpibat_match:
 *
 *	Autoconfiguration `match' routine.
 */
int
acpibat_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return (0);

	if (strcmp(aa->aa_node->ad_devinfo.HardwareId, "PNP0C0A") == 0)
		return (1);

	return (0);
}

/*
 * acpibat_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
void
acpibat_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpibat_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	printf(": ACPI Battery\n");

	sc->sc_node = aa->aa_node;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpibat_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register DEVICE NOTIFY handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}

	/* XXX See acpibat_notify_handler() */
	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_SYSTEM_NOTIFY, acpibat_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register SYSTEM NOTIFY handler: %d\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}
	/*
	 * XXX poll battery in the driver for now.
	 * in the future, when we have an API, let userland do this polling
	 */
	callout_init(&sc->sc_callout);
	callout_reset(&sc->sc_callout, 60*hz, acpibat_tick, sc);

	/* Display the current state. */
	sc->sc_flags = ABAT_F_VERBOSE;
	acpibat_get_info(sc);
	acpibat_get_status(sc);

	/*
	 * XXX Hook into sysmon here.
	 */
}

static void
acpibat_tick(void *arg)
{
	struct acpibat_softc *sc = arg;
	callout_reset(&sc->sc_callout, 60*hz, acpibat_tick, arg);
	AcpiOsQueueForExecution(OSD_PRIORITY_LO, acpibat_get_status, sc);
}


/*
 * acpibat_get_info
 *
 * 	Get, and possibly display, the battery info.
 */

static void
acpibat_get_info(void *arg)
{
	struct acpibat_softc *sc = arg;
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BIF", &buf);
	if (rv != AE_OK) {
		printf("%s: failed to evaluate _BIF: %x\n",
		    sc->sc_dev.dv_xname, rv);
		return;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;
	if (p1->Type != ACPI_TYPE_PACKAGE) {
		printf("%s: expected PACKAGE, got %d\n", sc->sc_dev.dv_xname,
		    p1->Type);
		goto out;
	}
	if (p1->Package.Count < 13)
		printf("%s: expected 13 elts, got %d\n",
		    sc->sc_dev.dv_xname, p1->Package.Count);

	p2 = p1->Package.Elements;
	if (p2[0].Integer.Value == 1)
		sc->sc_flags |= ABAT_F_PWRUNIT_MA;

	sc->sc_design_capacity = p2[1].Integer.Value;
	sc->sc_pred_capacity = p2[2].Integer.Value;
	sc->sc_warn_capacity = p2[6].Integer.Value;
	sc->sc_low_capacity = p2[7].Integer.Value;

	printf("%s: %s %s %s %s\n",
	    sc->sc_dev.dv_xname,
	    p2[12].String.Pointer, p2[11].String.Pointer,
	    p2[9].String.Pointer, p2[10].String.Pointer);

	printf("%s: Design %d%s, Predicted %d%s Warn %d%s Low %d%s\n",
	    sc->sc_dev.dv_xname,
	    sc->sc_design_capacity, ACM_CAPUNIT(sc),
	    sc->sc_pred_capacity, ACM_CAPUNIT(sc),
	    sc->sc_warn_capacity, ACM_CAPUNIT(sc),
	    sc->sc_low_capacity, ACM_CAPUNIT(sc));
out:
	AcpiOsFree(buf.Pointer);
}

/*
 * acpibat_get_status:
 *
 *	Get, and possibly display, the current battery line status.
 */
static void
acpibat_get_status(void *arg)
{
	struct acpibat_softc *sc = arg;
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BST", &buf);
	if (rv != AE_OK) {
		printf("bat: failed to evaluate _BIF: %x\n", rv);
		return;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		printf("bat: expected PACKAGE, got %d\n", p1->Type);
		goto out;
	}
	if (p1->Package.Count < 4)
		printf("bat: expected 4 elts, got %d\n", p1->Package.Count);
	p2 = p1->Package.Elements;

	sc->sc_status = p2[0].Integer.Value;
	sc->sc_rate = p2[1].Integer.Value;
	sc->sc_capacity = p2[2].Integer.Value;
	sc->sc_mv = p2[3].Integer.Value;

	if (sc->sc_flags & ABAT_F_VERBOSE) {
		printf("%s: %s%s: %dmV cap %d%s (%d%%) rate %d%s\n",
		    sc->sc_dev.dv_xname,
		    (sc->sc_status&4) ? "CRITICAL ":"",
		    (sc->sc_status&1) ? "discharging" : (
			    (sc->sc_status & 2) ? "charging" : "idle"),
		    sc->sc_mv,
		    sc->sc_capacity,
		    ACM_CAPUNIT(sc),
		    (sc->sc_capacity * 100) / sc->sc_design_capacity,
		    sc->sc_rate,
		    ACM_RATEUNIT(sc));
	}
out:
	AcpiOsFree(buf.Pointer);
}

/*
 * acpibat_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
void
acpibat_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	struct acpibat_softc *sc = context;
	int rv;

	switch (notify) {
	case ACPI_NOTIFY_BusCheck:
	case ACPI_NOTIFY_BatteryStatusChanged:
	case ACPI_NOTIFY_BatteryInformationChanged:
#ifdef ACPI_BAT_DEBUG
		printf("%s: received notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
#endif
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
		    acpibat_get_status, sc);
		if (rv != AE_OK)
			printf("%s: unable to queue status check: %d\n",
			    sc->sc_dev.dv_xname, rv);
		break;
	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}
