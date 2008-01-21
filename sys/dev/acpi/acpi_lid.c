/*	$NetBSD: acpi_lid.c,v 1.16.2.3 2008/01/21 09:42:30 yamt Exp $	*/

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
 * ACPI Lid Switch driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_lid.c,v 1.16.2.3 2008/01/21 09:42:30 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

struct acpilid_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	struct sysmon_pswitch sc_smpsw;	/* our sysmon glue */
};

static const char * const lid_hid[] = {
	"PNP0C0D",
	NULL
};

static int	acpilid_match(struct device *, struct cfdata *, void *);
static void	acpilid_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpilid, sizeof(struct acpilid_softc),
    acpilid_match, acpilid_attach, NULL, NULL);

static void	acpilid_status_changed(void *);
static void	acpilid_notify_handler(ACPI_HANDLE, UINT32, void *);

static bool	acpilid_suspend(device_t);

/*
 * acpilid_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpilid_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, lid_hid);
}

/*
 * acpilid_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
static void
acpilid_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpilid_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": ACPI Lid Switch\n");
	aprint_normal(": ACPI Lid Switch\n");

	sc->sc_node = aa->aa_node;

	sc->sc_smpsw.smpsw_name = sc->sc_dev.dv_xname;
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_LID;
	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, acpilid_notify_handler, sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to register DEVICE NOTIFY handler: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}

	acpi_set_wake_gpe(sc->sc_node->ad_handle);

	if (!pmf_device_register(self, acpilid_suspend, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static void
acpilid_wake_event(struct acpilid_softc *sc, bool enable)
{
	ACPI_STATUS rv;
        ACPI_OBJECT_LIST ArgList;
        ACPI_OBJECT Arg;

        ArgList.Count = 1;
	ArgList.Pointer = &Arg;

	Arg.Type = ACPI_TYPE_INTEGER;
	Arg.Integer.Value = enable ? 1 : 0;

	rv = AcpiEvaluateObject (sc->sc_node->ad_handle, "_PSW",
	    &ArgList, NULL);
	if (ACPI_FAILURE(rv) && rv != AE_NOT_FOUND)
		aprint_error_dev(&sc->sc_dev,
		    "unable to evaluate _PSW handler: %s\n",
		    AcpiFormatException(rv));
}

/*
 * acpilid_status_changed:
 *
 *	Get, and possibly display, the lid status, and take action.
 */
static void
acpilid_status_changed(void *arg)
{
	struct acpilid_softc *sc = arg;
	ACPI_INTEGER status;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_LID", &status);
	if (ACPI_FAILURE(rv))
		return;

	sysmon_pswitch_event(&sc->sc_smpsw, status == 0 ?
	    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
}

/*
 * acpilid_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpilid_notify_handler(ACPI_HANDLE handle, UINT32 notify,
    void *context)
{
	struct acpilid_softc *sc = context;
	int rv;

	switch (notify) {
	case ACPI_NOTIFY_LidStatusChanged:
#ifdef ACPI_LID_DEBUG
		printf("%s: received LidStatusChanged message\n",
		    sc->sc_dev.dv_xname);
#endif
		rv = AcpiOsExecute(OSL_NOTIFY_HANDLER,
		    acpilid_status_changed, sc);
		if (ACPI_FAILURE(rv))
			printf("%s: WARNING: unable to queue lid change "
			    "callback: %s\n", sc->sc_dev.dv_xname,
			    AcpiFormatException(rv));
		break;

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}

static bool
acpilid_suspend(device_t dv)
{
	struct acpilid_softc *sc = device_private(dv);
	ACPI_INTEGER status;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_LID", &status);
	if (ACPI_FAILURE(rv))
		return true;

	acpilid_wake_event(sc, status == 0);
	return true;
}
