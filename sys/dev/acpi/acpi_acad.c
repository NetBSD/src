/*	$NetBSD: acpi_acad.c,v 1.35 2009/08/25 10:34:08 jmcneill Exp $	*/

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

#if 0
#define ACPI_ACAD_DEBUG
#endif

/*
 * ACPI AC Adapter driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_acad.c,v 1.35 2009/08/25 10:34:08 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

struct acpiacad_softc {
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	int sc_status;			/* status changed/not changed */
	int sc_notifysent;		/* notify message sent */

	struct sysmon_envsys *sc_sme;
	struct sysmon_pswitch sc_smpsw;	/* our sysmon glue */
	envsys_data_t sc_sensor;

	kmutex_t sc_mtx;
};

static const char * const acad_hid[] = {
	"ACPI0003",
	NULL
};

#define	AACAD_F_VERBOSE		0x01	/* verbose events */
#define AACAD_F_AVAILABLE	0x02	/* information is available */
#define AACAD_F_STCHANGED	0x04	/* status changed */

#define AACAD_SET(sc, f)	(void)((sc)->sc_flags |= (f))
#define AACAD_CLEAR(sc, f)	(void)((sc)->sc_flags &= ~(f))
#define AACAD_ISSET(sc, f)	((sc)->sc_flags & (f))

static int acpiacad_match(device_t, cfdata_t, void *);
static void acpiacad_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(acpiacad, sizeof(struct acpiacad_softc),
    acpiacad_match, acpiacad_attach, NULL, NULL);

static void acpiacad_get_status(void *);
static void acpiacad_clear_status(struct acpiacad_softc *);
static void acpiacad_notify_handler(ACPI_HANDLE, UINT32, void *);
static void acpiacad_init_envsys(device_t);
static void acpiacad_refresh(struct sysmon_envsys *, envsys_data_t *);
static bool acpiacad_resume(device_t PMF_FN_PROTO);

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

	sc->sc_node = aa->aa_node;
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NONE);

	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error_dev(self, "unable to register with sysmon\n");
		return;
	}

	sc->sc_status = -1;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpiacad_notify_handler, self);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "unable to register DEVICE and SYSTEM "
		    "NOTIFY handler: %s\n", AcpiFormatException(rv));
		return;
	}

#ifdef ACPI_ACAD_DEBUG
	/* Display the current state. */
	sc->sc_flags = AACAD_F_VERBOSE;
#endif

	if (!pmf_device_register(self, NULL, acpiacad_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	acpiacad_init_envsys(self);
}

/*
 * acpiacad_resume:
 *
 * 	Clear status after resuming to fetch new status.
 */
static bool
acpiacad_resume(device_t dv PMF_FN_ARGS)
{
	struct acpiacad_softc *sc = device_private(dv);

	acpiacad_clear_status(sc);
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

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_PSR", &status);
	if (ACPI_FAILURE(rv))
		return;

	mutex_enter(&sc->sc_mtx);
	sc->sc_notifysent = 0;
	if (sc->sc_status != status) {
		sc->sc_status = status;
		if (status)
			sc->sc_sensor.value_cur = 1;
		else
			sc->sc_sensor.value_cur = 0;
		AACAD_SET(sc, AACAD_F_STCHANGED);
	}

	sc->sc_sensor.state = ENVSYS_SVALID;
	AACAD_SET(sc, AACAD_F_AVAILABLE);
	/*
	 * If status has changed, send the event.
	 *
	 * PSWITCH_EVENT_RELEASED : AC offline
	 * PSWITCH_EVENT_PRESSED  : AC online
	 */
	if (AACAD_ISSET(sc, AACAD_F_STCHANGED)) {
		sysmon_pswitch_event(&sc->sc_smpsw, status ?
	    	    PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED);
		if (AACAD_ISSET(sc, AACAD_F_VERBOSE))
			aprint_verbose_dev(dv, "AC adapter %sconnected\n",
		    	    status == 0 ? "not " : "");
	}
	mutex_exit(&sc->sc_mtx);
}

/*
 * Clear status
 */
static void
acpiacad_clear_status(struct acpiacad_softc *sc)
{
	sc->sc_sensor.state = ENVSYS_SINVALID;
	AACAD_CLEAR(sc, AACAD_F_AVAILABLE);
	AACAD_CLEAR(sc, AACAD_F_STCHANGED);
}

/*
 * acpiacad_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpiacad_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	device_t dv = context;
	struct acpiacad_softc *sc = device_private(dv);
	ACPI_STATUS rv;

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
		mutex_enter(&sc->sc_mtx);
		acpiacad_clear_status(sc);
		mutex_exit(&sc->sc_mtx);
		if (sc->sc_status == -1 || !sc->sc_notifysent) {
			rv = AcpiOsExecute(OSL_NOTIFY_HANDLER,
			    acpiacad_get_status, dv);
			if (ACPI_FAILURE(rv))
				aprint_error_dev(dv,
				    "unable to queue status check: %s\n",
				    AcpiFormatException(rv));
			sc->sc_notifysent = 1;
#ifdef ACPI_ACAD_DEBUG
			aprint_debug_dev(dv, "received notify message: 0x%x\n",
		    	    notify);
#endif
		}
		break;

	default:
		aprint_error_dev(dv, "received unknown notify message: 0x%x\n",
		    notify);
	}
}

static void
acpiacad_init_envsys(device_t dv)
{
	struct acpiacad_softc *sc = device_private(dv);

	sc->sc_sme = sysmon_envsys_create();
	sc->sc_sensor.state = ENVSYS_SVALID;
	sc->sc_sensor.units = ENVSYS_INDICATOR;
 	strlcpy(sc->sc_sensor.desc, "connected", sizeof(sc->sc_sensor.desc));

	if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor)) {
		aprint_error_dev(dv, "unable to add sensor\n");
		sysmon_envsys_destroy(sc->sc_sme);
		return;
	}

	sc->sc_sme->sme_name = device_xname(dv);
	sc->sc_sme->sme_cookie = dv;
	sc->sc_sme->sme_refresh = acpiacad_refresh;
	sc->sc_sme->sme_class = SME_CLASS_ACADAPTER;
	sc->sc_sme->sme_flags = SME_INIT_REFRESH;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(dv, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
acpiacad_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	device_t dv = sme->sme_cookie;
	struct acpiacad_softc *sc = device_private(dv);

	if (!AACAD_ISSET(sc, AACAD_F_AVAILABLE))
		acpiacad_get_status(dv);
}
