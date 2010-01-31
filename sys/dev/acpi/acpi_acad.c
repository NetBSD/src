/*	$NetBSD: acpi_acad.c,v 1.39 2010/01/31 11:16:18 jruoho Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_acad.c,v 1.39 2010/01/31 11:16:18 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#define _COMPONENT		 ACPI_ACAD_COMPONENT
ACPI_MODULE_NAME		 ("acpi_acad")

struct acpiacad_softc {
	struct acpi_devnode	*sc_node;
	struct sysmon_envsys	*sc_sme;
	struct sysmon_pswitch	 sc_smpsw;
	envsys_data_t		 sc_sensor;
	kmutex_t		 sc_mutex;
	int			 sc_status;
};

static const char * const acad_hid[] = {
	"ACPI0003",
	NULL
};

static int	acpiacad_match(device_t, cfdata_t, void *);
static void	acpiacad_attach(device_t, device_t, void *);
static int	acpiacad_detach(device_t, int);
static bool	acpiacad_resume(device_t, pmf_qual_t);
static void	acpiacad_get_status(void *);
static void	acpiacad_notify_handler(ACPI_HANDLE, uint32_t, void *);
static void	acpiacad_init_envsys(device_t);

CFATTACH_DECL_NEW(acpiacad, sizeof(struct acpiacad_softc),
    acpiacad_match, acpiacad_attach, acpiacad_detach, NULL);

/*
 * acpiacad_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpiacad_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, acad_hid);
}

/*
 * acpiacad_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
static void
acpiacad_attach(device_t parent, device_t self, void *aux)
{
	struct acpiacad_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": ACPI AC Adapter\n");
	aprint_normal(": ACPI AC Adapter\n");

	sc->sc_sme = NULL;
	sc->sc_status = -1;
	sc->sc_node = aa->aa_node;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_ACADAPTER;

	(void)sysmon_pswitch_register(&sc->sc_smpsw);
	(void)pmf_device_register(self, NULL, acpiacad_resume);

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpiacad_notify_handler, self);

	if (ACPI_SUCCESS(rv))
		acpiacad_init_envsys(self);
}

/*
 * acpiacad_detach:
 *
 *	Autoconfiguration `detach' routine.
 */
static int
acpiacad_detach(device_t self, int flags)
{
	struct acpiacad_softc *sc = device_private(self);
	ACPI_STATUS rv;

	rv = AcpiRemoveNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpiacad_notify_handler);

	if (ACPI_FAILURE(rv))
		return EBUSY;

	mutex_destroy(&sc->sc_mutex);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	pmf_device_deregister(self);
	sysmon_pswitch_unregister(&sc->sc_smpsw);

	return 0;
}

/*
 * acpiacad_resume:
 *
 * 	Queue a new status check.
 */
static bool
acpiacad_resume(device_t dv, pmf_qual_t qual)
{

	(void)AcpiOsExecute(OSL_NOTIFY_HANDLER, acpiacad_get_status, dv);

	return true;
}

/*
 * acpiacad_get_status:
 *
 *	Get, and possibly display, the current AC line status.
 */
static void
acpiacad_get_status(void *arg)
{
	device_t dv = arg;
	struct acpiacad_softc *sc = device_private(dv);
	ACPI_INTEGER status;
	ACPI_STATUS rv;

	mutex_enter(&sc->sc_mutex);

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_PSR", &status);

	if (ACPI_FAILURE(rv))
		goto fail;

	if (status != 0 && status != 1) {
		rv = AE_BAD_VALUE;
		goto fail;
	}

	if (sc->sc_status != status) {

		/*
		 * If status has changed, send the event:
		 *
		 * PSWITCH_EVENT_PRESSED  : _PSR = 1 : AC online.
		 * PSWITCH_EVENT_RELEASED : _PSR = 0 : AC offline.
		 */
		sysmon_pswitch_event(&sc->sc_smpsw, (status != 0) ?
	    	    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);

		aprint_debug_dev(dv, "AC adapter %sconnected\n",
		    status == 0 ? "not " : "");
	}

	sc->sc_status = status;
	sc->sc_sensor.state = ENVSYS_SVALID;
	sc->sc_sensor.value_cur = sc->sc_status;

	mutex_exit(&sc->sc_mutex);

	return;

fail:
	sc->sc_status = -1;
	sc->sc_sensor.state = ENVSYS_SINVALID;

	aprint_debug_dev(dv, "failed to evaluate _PSR: %s\n",
	    AcpiFormatException(rv));

	mutex_exit(&sc->sc_mutex);
}

/*
 * acpiacad_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpiacad_notify_handler(ACPI_HANDLE handle, uint32_t notify, void *context)
{
	static const int handler = OSL_NOTIFY_HANDLER;
	device_t dv = context;

	switch (notify) {
	/*
	 * XXX So, BusCheck is not exactly what I would expect,
	 * but at least my IBM T21 sends it on AC adapter status
	 * change.  --thorpej@wasabisystems.com
	 */
	/*
	 * XXX My Acer TravelMate 291 sends DeviceCheck on AC
	 * adapter status change.
	 *  --rpaulo@NetBSD.org
	 */
	/*
	 * XXX Sony VAIO VGN-N250E sends BatteryInformationChanged on AC
	 * adapter status change.
	 *  --jmcneill@NetBSD.org
	 */
	case ACPI_NOTIFY_BusCheck:
	case ACPI_NOTIFY_DeviceCheck:
	case ACPI_NOTIFY_PowerSourceStatusChanged:
	case ACPI_NOTIFY_BatteryInformationChanged:
		(void)AcpiOsExecute(handler, acpiacad_get_status, dv);
		break;

	default:
		aprint_error_dev(dv, "unknown notify 0x%02X\n", notify);
	}
}

static void
acpiacad_init_envsys(device_t dv)
{
	struct acpiacad_softc *sc = device_private(dv);

	sc->sc_sme = sysmon_envsys_create();

	sc->sc_sensor.state = ENVSYS_SINVALID;
	sc->sc_sensor.units = ENVSYS_INDICATOR;

 	(void)strlcpy(sc->sc_sensor.desc, "connected", ENVSYS_DESCLEN);

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor) != 0)
		goto fail;

	sc->sc_sme->sme_name = device_xname(dv);
	sc->sc_sme->sme_class = SME_CLASS_ACADAPTER;
	sc->sc_sme->sme_flags = SME_DISABLE_REFRESH;

	if (sysmon_envsys_register(sc->sc_sme) != 0)
		goto fail;

	(void)AcpiOsExecute(OSL_NOTIFY_HANDLER, acpiacad_get_status, dv);

	return;

fail:
	aprint_error_dev(dv, "failed to initialize sysmon\n");
	sysmon_envsys_destroy(sc->sc_sme);
	sc->sc_sme = NULL;
}
