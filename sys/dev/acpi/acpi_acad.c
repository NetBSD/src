/*	$NetBSD: acpi_acad.c,v 1.23 2007/07/01 07:37:12 xtraeme Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_acad.c,v 1.23 2007/07/01 07:37:12 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#define ACPIACAD_NSENSORS	2

/* sensor indexes */
#define ACPIACAD_CONNECTED	0
#define ACPIACAD_DISCONNECTED	1

struct acpiacad_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	int sc_status;			/* status changed/not changed */
	int sc_notifysent;		/* notify message sent */

	struct sysmon_envsys sc_sysmon;
	struct sysmon_pswitch sc_smpsw;	/* our sysmon glue */
	struct envsys_data sc_data[ACPIACAD_NSENSORS];

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

static int acpiacad_match(struct device *, struct cfdata *, void *);
static void acpiacad_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpiacad, sizeof(struct acpiacad_softc),
    acpiacad_match, acpiacad_attach, NULL, NULL);

static void acpiacad_get_status(void *);
static void acpiacad_clear_status(struct acpiacad_softc *);
static void acpiacad_notify_handler(ACPI_HANDLE, UINT32, void *);
static void acpiacad_init_envsys(struct acpiacad_softc *);
static int acpiacad_gtredata(struct sysmon_envsys *, envsys_data_t *);

/*
 * acpiacad_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpiacad_match(struct device *parent, struct cfdata *match, void *aux)
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
acpiacad_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpiacad_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": ACPI AC Adapter\n");
	aprint_normal(": ACPI AC Adapter\n");

	sc->sc_node = aa->aa_node;
	mutex_init(&sc->sc_mtx, MUTEX_DRIVER, IPL_NONE);

	sc->sc_smpsw.smpsw_name = sc->sc_dev.dv_xname;
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_ACADAPTER;
	if (sysmon_pswitch_register(&sc->sc_smpsw) != 0) {
		aprint_error("%s: unable to register with sysmon\n",
		       sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_status = -1;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpiacad_notify_handler, sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to register DEVICE and SYSTEM "
		    "NOTIFY handler: %s\n", sc->sc_dev.dv_xname,
		    AcpiFormatException(rv));
		return;
	}

#ifdef ACPI_ACAD_DEBUG
	/* Display the current state. */
	sc->sc_flags = AACAD_F_VERBOSE;
#endif

	acpiacad_init_envsys(sc);
}

/*
 * acpiacad_get_status:
 *
 *	Get, and possibly display, the current AC line status.
 */
static void
acpiacad_get_status(void *arg)
{
	struct acpiacad_softc *sc = arg;
	ACPI_INTEGER status;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_PSR", &status);
	if (ACPI_FAILURE(rv))
		return;

	mutex_enter(&sc->sc_mtx);
	if (sc->sc_status != status) {
		sc->sc_status = status;
		if (status) {
			sc->sc_data[ACPIACAD_CONNECTED].state = ENVSYS_SVALID;
			sc->sc_data[ACPIACAD_CONNECTED].value_cur = 1;
		} else {
			sc->sc_data[ACPIACAD_DISCONNECTED].state =
			    ENVSYS_SVALID;
			sc->sc_data[ACPIACAD_DISCONNECTED].value_cur = 1;
		}
		AACAD_SET(sc, AACAD_F_STCHANGED);
		sc->sc_notifysent = 0;
	}

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
			printf("%s: AC adapter %sconnected\n",
		    	    sc->sc_dev.dv_xname, status == 0 ? "not " : "");
	}
	mutex_exit(&sc->sc_mtx);
}

/*
 * Clear status
 */
static void
acpiacad_clear_status(struct acpiacad_softc *sc)
{
	sc->sc_data[ACPIACAD_CONNECTED].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIACAD_DISCONNECTED].state = ENVSYS_SINVALID;
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
	struct acpiacad_softc *sc = context;
	int rv;

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
			rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
			    acpiacad_get_status, sc);
			if (ACPI_FAILURE(rv))
				printf("%s: unable to queue status check: %s\n",
				    sc->sc_dev.dv_xname,
				    AcpiFormatException(rv));
			sc->sc_notifysent = 1;
#ifdef ACPI_ACAD_DEBUG
			printf("%s: received notify message: 0x%x\n",
		    	    sc->sc_dev.dv_xname, notify);
#endif
		}
		break;

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
	}
}

static void
acpiacad_init_envsys(struct acpiacad_softc *sc)
{
	int i;

	for (i = 0; i < ACPIACAD_NSENSORS; i++) {
		sc->sc_data[i].sensor = i;
		sc->sc_data[i].state = ENVSYS_SVALID;
	}

#define INITDATA(index, unit, string) \
	sc->sc_data[index].units = unit;     				\
	snprintf(sc->sc_data[index].desc, sizeof(sc->sc_data->desc),	\
	    "%s %s", sc->sc_dev.dv_xname, string);			\

	INITDATA(ACPIACAD_CONNECTED, ENVSYS_INDICATOR, "connected");
	INITDATA(ACPIACAD_DISCONNECTED, ENVSYS_INDICATOR, "disconnected");

	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_name = sc->sc_dev.dv_xname;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = acpiacad_gtredata;
	sc->sc_sysmon.sme_nsensors = ACPIACAD_NSENSORS;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

static int
acpiacad_gtredata(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct acpiacad_softc *sc = sme->sme_cookie;

	if (!AACAD_ISSET(sc, AACAD_F_AVAILABLE))
		acpiacad_get_status(sc);

	return 0;
}
