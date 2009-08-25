/*	$NetBSD: acpi_bat.c,v 1.72 2009/08/25 10:34:08 jmcneill Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum of By Noon Software, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

#if 0
#define ACPI_BAT_DEBUG
#endif

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
__KERNEL_RCSID(0, "$NetBSD: acpi_bat.c,v 1.72 2009/08/25 10:34:08 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* for hz */
#include <sys/device.h>
#include <sys/mutex.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

/* sensor indexes */
#define ACPIBAT_PRESENT		0
#define ACPIBAT_DCAPACITY	1
#define ACPIBAT_LFCCAPACITY	2
#define ACPIBAT_TECHNOLOGY	3
#define ACPIBAT_DVOLTAGE	4
#define ACPIBAT_WCAPACITY	5
#define ACPIBAT_LCAPACITY	6
#define ACPIBAT_VOLTAGE		7
#define ACPIBAT_CHARGERATE	8
#define ACPIBAT_DISCHARGERATE	9
#define ACPIBAT_CAPACITY	10
#define ACPIBAT_CHARGING	11
#define ACPIBAT_CHARGE_STATE	12
#define ACPIBAT_NSENSORS	13  /* number of sensors */

struct acpibat_softc {
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	int sc_available;		/* available information level */

	struct sysmon_envsys *sc_sme;
	envsys_data_t sc_sensor[ACPIBAT_NSENSORS];
	struct timeval sc_lastupdate;

	kmutex_t sc_mutex;
	kcondvar_t sc_condvar;
};

static const char * const bat_hid[] = {
	"PNP0C0A",
	NULL
};

/*
 * These flags are used to examine the battery device data returned from
 * the ACPI interface, specifically the "battery status"
 */
#define ACPIBAT_PWRUNIT_MA	0x00000001  /* mA not mW */

/*
 * These flags are used to examine the battery charge/discharge/critical
 * state returned from a get-status command.
 */
#define ACPIBAT_ST_DISCHARGING	0x00000001  /* battery is discharging */
#define ACPIBAT_ST_CHARGING	0x00000002  /* battery is charging */
#define ACPIBAT_ST_CRITICAL	0x00000004  /* battery is critical */

/*
 * Flags for battery status from _STA return
 */
#define ACPIBAT_STA_PRESENT	0x00000010  /* battery present */

/*
 * These flags are used to set internal state in our softc.
 */
#define	ABAT_F_VERBOSE		0x01	/* verbose events */
#define ABAT_F_PWRUNIT_MA	0x02 	/* mA instead of mW */
#define ABAT_F_PRESENT		0x04	/* is the battery present? */

#define ABAT_SET(sc, f)		(void)((sc)->sc_flags |= (f))
#define ABAT_CLEAR(sc, f)	(void)((sc)->sc_flags &= ~(f))
#define ABAT_ISSET(sc, f)	((sc)->sc_flags & (f))

/*
 * Available info level
 */

#define ABAT_ALV_NONE		0	/* none is available */
#define ABAT_ALV_PRESENCE	1	/* presence info is available */
#define ABAT_ALV_INFO		2	/* battery info is available */
#define ABAT_ALV_STAT		3	/* battery status is available */

static int	acpibat_match(device_t, cfdata_t, void *);
static void	acpibat_attach(device_t, device_t, void *);
static bool	acpibat_resume(device_t PMF_FN_PROTO);

CFATTACH_DECL_NEW(acpibat, sizeof(struct acpibat_softc),
    acpibat_match, acpibat_attach, NULL, NULL);

static void acpibat_clear_presence(struct acpibat_softc *);
static void acpibat_clear_info(struct acpibat_softc *);
static void acpibat_clear_stat(struct acpibat_softc *);
static int acpibat_battery_present(device_t);
static ACPI_STATUS acpibat_get_status(device_t);
static ACPI_STATUS acpibat_get_info(device_t);
static void acpibat_print_info(device_t);
static void acpibat_print_stat(device_t);
static void acpibat_update(void *);
static void acpibat_update_info(void *);
static void acpibat_update_stat(void *);

static void acpibat_init_envsys(device_t);
static void acpibat_notify_handler(ACPI_HANDLE, UINT32, void *);
static void acpibat_refresh(struct sysmon_envsys *, envsys_data_t *);

/*
 * acpibat_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpibat_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, bat_hid);
}

static bool
acpibat_resume(device_t dv PMF_FN_ARGS)
{
	ACPI_STATUS rv;

	rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_stat, dv);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(dv, "unable to queue status check: %s\n",
		    AcpiFormatException(rv));
	rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_info, dv);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(dv, "unable to queue info check: %s\n",
		    AcpiFormatException(rv));

	return true;
}

/*
 * acpibat_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
static void
acpibat_attach(device_t parent, device_t self, void *aux)
{
	struct acpibat_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": ACPI Battery (Control Method)\n");
	aprint_normal(": ACPI Battery (Control Method)\n");

	sc->sc_node = aa->aa_node;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condvar, device_xname(self));

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
				      ACPI_ALL_NOTIFY,
				      acpibat_notify_handler, self);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self,
		    "unable to register DEVICE/SYSTEM NOTIFY handler: %s\n",
		    AcpiFormatException(rv));
		return;
	}

#ifdef ACPI_BAT_DEBUG
	ABAT_SET(sc, ABAT_F_VERBOSE);
#endif

	if (!pmf_device_register(self, NULL, acpibat_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	acpibat_init_envsys(self);
}

/*
 * clear informations
 */

static void
acpibat_clear_presence(struct acpibat_softc *sc)
{
	acpibat_clear_info(sc);
	sc->sc_available = ABAT_ALV_NONE;
	ABAT_CLEAR(sc, ABAT_F_PRESENT);
}

static void
acpibat_clear_info(struct acpibat_softc *sc)
{
	acpibat_clear_stat(sc);
	if (sc->sc_available > ABAT_ALV_PRESENCE)
		sc->sc_available = ABAT_ALV_PRESENCE;

	sc->sc_sensor[ACPIBAT_DCAPACITY].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_TECHNOLOGY].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_DVOLTAGE].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_WCAPACITY].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_LCAPACITY].state = ENVSYS_SINVALID;
}

static void
acpibat_clear_stat(struct acpibat_softc *sc)
{
	if (sc->sc_available > ABAT_ALV_INFO)
		sc->sc_available = ABAT_ALV_INFO;

	sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_VOLTAGE].state = ENVSYS_SINVALID;
	sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SINVALID;
}


/*
 * returns 0 for no battery, 1 for present, and -1 on error
 */
static int
acpibat_battery_present(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	uint32_t sta;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_STA", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(dv, "failed to evaluate _STA: %s\n",
		    AcpiFormatException(rv));
		return -1;
	}

	sta = (uint32_t)val;

	sc->sc_available = ABAT_ALV_PRESENCE;
	if (sta & ACPIBAT_STA_PRESENT) {
		ABAT_SET(sc, ABAT_F_PRESENT);
		sc->sc_sensor[ACPIBAT_PRESENT].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_PRESENT].value_cur = 1;
	} else
		sc->sc_sensor[ACPIBAT_PRESENT].value_cur = 0;

	return (sta & ACPIBAT_STA_PRESENT) ? 1 : 0;
}

/*
 * acpibat_get_info
 *
 * 	Get, and possibly display, the battery info.
 */

static ACPI_STATUS
acpibat_get_info(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	int capunit, rateunit;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BIF", &buf);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(dv, "failed to evaluate _BIF: %s\n",
		    AcpiFormatException(rv));
		return rv;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		aprint_error_dev(dv, "expected PACKAGE, got %d\n", p1->Type);
		goto out;
	}
	if (p1->Package.Count < 13) {
		aprint_error_dev(dv, "expected 13 elements, got %d\n",
		    p1->Package.Count);
		goto out;
	}
	p2 = p1->Package.Elements;

	if ((p2[0].Integer.Value & ACPIBAT_PWRUNIT_MA) != 0) {
		ABAT_SET(sc, ABAT_F_PWRUNIT_MA);
		capunit = ENVSYS_SAMPHOUR;
		rateunit = ENVSYS_SAMPS;
	} else {
		ABAT_CLEAR(sc, ABAT_F_PWRUNIT_MA);
		capunit = ENVSYS_SWATTHOUR;
		rateunit = ENVSYS_SWATTS;
	}

	sc->sc_sensor[ACPIBAT_DCAPACITY].units = capunit;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].units = capunit;
	sc->sc_sensor[ACPIBAT_WCAPACITY].units = capunit;
	sc->sc_sensor[ACPIBAT_LCAPACITY].units = capunit;
	sc->sc_sensor[ACPIBAT_CHARGERATE].units = rateunit;
	sc->sc_sensor[ACPIBAT_DISCHARGERATE].units = rateunit;
	sc->sc_sensor[ACPIBAT_CAPACITY].units = capunit;

	sc->sc_sensor[ACPIBAT_DCAPACITY].value_cur = p2[1].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_DCAPACITY].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].value_cur = p2[2].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_CAPACITY].value_max = p2[2].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_TECHNOLOGY].value_cur = p2[3].Integer.Value;
	sc->sc_sensor[ACPIBAT_TECHNOLOGY].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_DVOLTAGE].value_cur = p2[4].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_DVOLTAGE].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_WCAPACITY].value_cur = p2[5].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_WCAPACITY].value_max = p2[2].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_WCAPACITY].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_WCAPACITY].flags |=
	    (ENVSYS_FPERCENT|ENVSYS_FVALID_MAX);
	sc->sc_sensor[ACPIBAT_LCAPACITY].value_cur = p2[6].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_LCAPACITY].value_max = p2[2].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_LCAPACITY].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_LCAPACITY].flags |=
	    (ENVSYS_FPERCENT|ENVSYS_FVALID_MAX);
	sc->sc_available = ABAT_ALV_INFO;

	aprint_verbose_dev(dv, "battery info: %s, %s, %s",
	    p2[12].String.Pointer, p2[11].String.Pointer, p2[9].String.Pointer);
	if (p2[10].String.Pointer)
		aprint_verbose(" %s", p2[10].String.Pointer);

	aprint_verbose("\n");

	rv = AE_OK;

out:
	AcpiOsFree(buf.Pointer);
	return rv;
}

/*
 * acpibat_get_status:
 *
 *	Get, and possibly display, the current battery line status.
 */
static ACPI_STATUS
acpibat_get_status(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	int status, battrate;
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BST", &buf);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(dv, "failed to evaluate _BST: %s\n",
		    AcpiFormatException(rv));
		return rv;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		aprint_error_dev(dv, "expected PACKAGE, got %d\n",
		    p1->Type);
		rv = AE_ERROR;
		goto out;
	}
	if (p1->Package.Count < 4) {
		aprint_error_dev(dv, "expected 4 elts, got %d\n",
		    p1->Package.Count);
		rv = AE_ERROR;
		goto out;
	}
	p2 = p1->Package.Elements;

	status = p2[0].Integer.Value;
	battrate = p2[1].Integer.Value;

	if (status & ACPIBAT_ST_CHARGING) {
		sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGERATE].value_cur = battrate * 1000;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].value_cur = 1;
	} else if (status & ACPIBAT_ST_DISCHARGING) {
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].value_cur = battrate * 1000;
		sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].value_cur = 0;
	} else if (!(status & (ACPIBAT_ST_CHARGING|ACPIBAT_ST_DISCHARGING))) {
		sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].value_cur = 0;
		sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
	}

	sc->sc_sensor[ACPIBAT_CHARGE_STATE].value_cur =
	    ENVSYS_BATTERY_CAPACITY_NORMAL;

	sc->sc_sensor[ACPIBAT_CAPACITY].value_cur = p2[2].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_CAPACITY].flags |=
	    (ENVSYS_FPERCENT|ENVSYS_FVALID_MAX);
	sc->sc_sensor[ACPIBAT_VOLTAGE].value_cur = p2[3].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_VOLTAGE].state = ENVSYS_SVALID;

	if (sc->sc_sensor[ACPIBAT_CAPACITY].value_cur <
	    sc->sc_sensor[ACPIBAT_WCAPACITY].value_cur) {
		sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SWARNUNDER;
		sc->sc_sensor[ACPIBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_WARNING;
	}

	if (sc->sc_sensor[ACPIBAT_CAPACITY].value_cur <
	    sc->sc_sensor[ACPIBAT_LCAPACITY].value_cur) {
		sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SCRITUNDER;
		sc->sc_sensor[ACPIBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_LOW;
	}

	if (status & ACPIBAT_ST_CRITICAL) {
		sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SCRITICAL;
		sc->sc_sensor[ACPIBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_CRITICAL;
	}

	rv = AE_OK;

out:
	AcpiOsFree(buf.Pointer);
	return rv;
}

#define SCALE(x)	((x)/1000000), (((x)%1000000)/1000)
#define CAPUNITS(sc)	(ABAT_ISSET((sc), ABAT_F_PWRUNIT_MA)?"Ah":"Wh")
#define RATEUNITS(sc)	(ABAT_ISSET((sc), ABAT_F_PWRUNIT_MA)?"A":"W")
static void
acpibat_print_info(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	const char *tech;

	if (sc->sc_sensor[ACPIBAT_TECHNOLOGY].value_cur)
		tech = "secondary";
	else
		tech = "primary";

	aprint_debug_dev(dv, "%s battery, Design %d.%03d%s "
	    "Last full %d.%03d%s Warn %d.%03d%s Low %d.%03d%s\n",
	    tech, SCALE(sc->sc_sensor[ACPIBAT_DCAPACITY].value_cur), CAPUNITS(sc),
	    SCALE(sc->sc_sensor[ACPIBAT_LFCCAPACITY].value_cur),CAPUNITS(sc),
	    SCALE(sc->sc_sensor[ACPIBAT_WCAPACITY].value_cur), CAPUNITS(sc),
	    SCALE(sc->sc_sensor[ACPIBAT_LCAPACITY].value_cur), CAPUNITS(sc));
}

static void
acpibat_print_stat(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	const char *capstat, *chargestat;
	int percent, denom;
	int32_t value;

	percent = 0;

	if (sc->sc_sensor[ACPIBAT_CAPACITY].state == ENVSYS_SCRITUNDER)
		capstat = "CRITICAL UNDER ";
	else if (sc->sc_sensor[ACPIBAT_CAPACITY].state == ENVSYS_SCRITOVER)
		capstat = "CRITICAL OVER ";
	else
		capstat = "";

	if (sc->sc_sensor[ACPIBAT_CHARGING].state != ENVSYS_SVALID) {
		chargestat = "idling";
		value = 0;
	} else if (sc->sc_sensor[ACPIBAT_CHARGING].value_cur == 0) {
		chargestat = "discharging";
		value = sc->sc_sensor[ACPIBAT_DISCHARGERATE].value_cur;
	} else {
		chargestat = "charging";
		value = sc->sc_sensor[ACPIBAT_CHARGERATE].value_cur;
	}

	denom = sc->sc_sensor[ACPIBAT_LFCCAPACITY].value_cur / 100;
	if (denom > 0)
		percent = (sc->sc_sensor[ACPIBAT_CAPACITY].value_cur) / denom;

	aprint_debug_dev(dv, "%s%s: %d.%03dV cap %d.%03d%s (%d%%) "
	    "rate %d.%03d%s\n", capstat, chargestat,
	    SCALE(sc->sc_sensor[ACPIBAT_VOLTAGE].value_cur),
	    SCALE(sc->sc_sensor[ACPIBAT_CAPACITY].value_cur), CAPUNITS(sc),
	    percent, SCALE(value), RATEUNITS(sc));
}

static void
acpibat_update(void *arg)
{
	device_t dv = arg;
	struct acpibat_softc *sc = device_private(dv);

	if (sc->sc_available < ABAT_ALV_INFO) {
		/* current information is invalid */
#if 0
		/*
		 * XXX: The driver sometimes unaware that the battery exist.
		 * (i.e. just after the boot or resuming)
		 * Thus, the driver should always check it here.
		 */
		if (sc->sc_available < ABAT_ALV_PRESENCE)
#endif
			/* presence is invalid */
			if (acpibat_battery_present(dv) < 0) {
				/* error */
				aprint_debug_dev(dv,
				    "cannot get battery presence.\n");
				return;
			}

		if (ABAT_ISSET(sc, ABAT_F_PRESENT)) {
			/* the battery is present. */
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				aprint_debug_dev(dv,
				    "battery is present.\n");
			if (ACPI_FAILURE(acpibat_get_info(dv)))
				return;
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				acpibat_print_info(dv);
		} else {
			/* the battery is not present. */
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				aprint_debug_dev(dv,
				    "battery is not present.\n");
			return;
		}
	} else {
		/* current information is valid */
		if (!ABAT_ISSET(sc, ABAT_F_PRESENT)) {
			/* the battery is not present. */
			return;
		}
 	}

	if (ACPI_FAILURE(acpibat_get_status(dv)))
		return;

	if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
		acpibat_print_stat(dv);
}

static void
acpibat_update_info(void *arg)
{
	device_t dev = arg;
	struct acpibat_softc *sc = device_private(dev);

	mutex_enter(&sc->sc_mutex);
	acpibat_clear_presence(sc);
	acpibat_update(arg);
	mutex_exit(&sc->sc_mutex);
}

static void
acpibat_update_stat(void *arg)
{
	device_t dev = arg;
	struct acpibat_softc *sc = device_private(dev);

	mutex_enter(&sc->sc_mutex);
	acpibat_clear_stat(sc);
	acpibat_update(arg);
	microtime(&sc->sc_lastupdate);
	cv_broadcast(&sc->sc_condvar);
	mutex_exit(&sc->sc_mutex);
}

/*
 * acpibat_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpibat_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	device_t dv = context;
	ACPI_STATUS rv;

#ifdef ACPI_BAT_DEBUG
	aprint_debug_dev(dv, "received notify message: 0x%x\n", notify);
#endif

	switch (notify) {
	case ACPI_NOTIFY_BusCheck:
		break;
	case ACPI_NOTIFY_DeviceCheck:
	case ACPI_NOTIFY_BatteryInformationChanged:
		rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_info, dv);
		if (ACPI_FAILURE(rv))
			aprint_error_dev(dv,
			    "unable to queue info check: %s\n",
			    AcpiFormatException(rv));
		break;

	case ACPI_NOTIFY_BatteryStatusChanged:
		rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_stat, dv);
		if (ACPI_FAILURE(rv))
			aprint_error_dev(dv,
			    "unable to queue status check: %s\n",
			    AcpiFormatException(rv));
		break;

	default:
		aprint_error_dev(dv,
		    "received unknown notify message: 0x%x\n", notify);
	}
}

static void
acpibat_init_envsys(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	int i, capunit, rateunit;

	if (sc->sc_flags & ABAT_F_PWRUNIT_MA) {
		capunit = ENVSYS_SAMPHOUR;
		rateunit = ENVSYS_SAMPS;
	} else {
		capunit = ENVSYS_SWATTHOUR;
		rateunit = ENVSYS_SWATTS;
	}

#define INITDATA(index, unit, string)					\
	sc->sc_sensor[index].state = ENVSYS_SVALID;			\
	sc->sc_sensor[index].units = unit;     				\
 	strlcpy(sc->sc_sensor[index].desc, string,			\
 	    sizeof(sc->sc_sensor[index].desc));

	INITDATA(ACPIBAT_PRESENT, ENVSYS_INDICATOR, "present");
	INITDATA(ACPIBAT_DCAPACITY, capunit, "design cap");
	INITDATA(ACPIBAT_LFCCAPACITY, capunit, "last full cap");
	INITDATA(ACPIBAT_TECHNOLOGY, ENVSYS_INTEGER, "technology");
	INITDATA(ACPIBAT_DVOLTAGE, ENVSYS_SVOLTS_DC, "design voltage");
	INITDATA(ACPIBAT_WCAPACITY, capunit, "warn cap");
	INITDATA(ACPIBAT_LCAPACITY, capunit, "low cap");
	INITDATA(ACPIBAT_VOLTAGE, ENVSYS_SVOLTS_DC, "voltage");
	INITDATA(ACPIBAT_CHARGERATE, rateunit, "charge rate");
	INITDATA(ACPIBAT_DISCHARGERATE, rateunit, "discharge rate");
	INITDATA(ACPIBAT_CAPACITY, capunit, "charge");
	INITDATA(ACPIBAT_CHARGING, ENVSYS_BATTERY_CHARGE, "charging");
	INITDATA(ACPIBAT_CHARGE_STATE, ENVSYS_BATTERY_CAPACITY, "charge state");

#undef INITDATA

	/* Enable monitoring for the charge state sensor */
	sc->sc_sensor[ACPIBAT_CHARGE_STATE].monitor = true;
	sc->sc_sensor[ACPIBAT_CHARGE_STATE].flags |= ENVSYS_FMONSTCHANGED;

	/* Disable userland monitoring on these sensors */
	sc->sc_sensor[ACPIBAT_VOLTAGE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_CHARGERATE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_DISCHARGERATE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_DCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_TECHNOLOGY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_DVOLTAGE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_WCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_sensor[ACPIBAT_LCAPACITY].flags = ENVSYS_FMONNOTSUPP;

	sc->sc_sme = sysmon_envsys_create();
	for (i = 0; i < ACPIBAT_NSENSORS; i++) {
		if (sysmon_envsys_sensor_attach(sc->sc_sme,
						&sc->sc_sensor[i])) {
			aprint_error_dev(dv, "unable to add sensor%d\n", i);
			sysmon_envsys_destroy(sc->sc_sme);
			return;
		}
	}

	sc->sc_sme->sme_name = device_xname(dv);
	sc->sc_sme->sme_cookie = dv;
	sc->sc_sme->sme_refresh = acpibat_refresh;
	sc->sc_sme->sme_class = SME_CLASS_BATTERY;
	sc->sc_sme->sme_flags = SME_POLL_ONLY;

	acpibat_update(dv);

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(dv, "unable to register with sysmon\n");
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
acpibat_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	device_t dv = sme->sme_cookie;
	struct acpibat_softc *sc = device_private(dv);
	ACPI_STATUS rv;
	struct timeval tv, tmp;

	if (ABAT_ISSET(sc, ABAT_F_PRESENT)) {
		tmp.tv_sec = 5;
		tmp.tv_usec = 0;
		microtime(&tv);
		timersub(&tv, &tmp, &tv);
		if (timercmp(&tv, &sc->sc_lastupdate, <))
			return;

		if (!mutex_tryenter(&sc->sc_mutex))
			return;
		rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_stat, dv);
		if (!ACPI_FAILURE(rv))
			cv_timedwait(&sc->sc_condvar, &sc->sc_mutex, hz);
		mutex_exit(&sc->sc_mutex);
	}
}
