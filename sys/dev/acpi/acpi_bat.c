/*	$NetBSD: acpi_bat.c,v 1.49 2007/07/05 12:14:18 xtraeme Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__KERNEL_RCSID(0, "$NetBSD: acpi_bat.c,v 1.49 2007/07/05 12:14:18 xtraeme Exp $");

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
#define ACPIBAT_NSENSORS	12  /* number of sensors */

struct acpibat_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	int sc_available;		/* available information level */

	struct sysmon_envsys sc_sysmon;
	struct envsys_data sc_data[ACPIBAT_NSENSORS];
	kmutex_t sc_mtx;

	struct timeval sc_lastupdate, sc_updateinterval;
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

static int	acpibat_match(struct device *, struct cfdata *, void *);
static void	acpibat_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpibat, sizeof(struct acpibat_softc),
    acpibat_match, acpibat_attach, NULL, NULL);

static void acpibat_clear_presence(struct acpibat_softc *);
static void acpibat_clear_info(struct acpibat_softc *);
static void acpibat_clear_stat(struct acpibat_softc *);
static int acpibat_battery_present(struct acpibat_softc *);
static ACPI_STATUS acpibat_get_status(struct acpibat_softc *);
static ACPI_STATUS acpibat_get_info(struct acpibat_softc *);
static void acpibat_print_info(struct acpibat_softc *);
static void acpibat_print_stat(struct acpibat_softc *);
static void acpibat_update(void *);

static void acpibat_init_envsys(struct acpibat_softc *);
static void acpibat_notify_handler(ACPI_HANDLE, UINT32, void *);
static int acpibat_gtredata(struct sysmon_envsys *, envsys_data_t *);

/*
 * acpibat_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpibat_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, bat_hid);
}

/*
 * acpibat_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
static void
acpibat_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpibat_softc *sc = (void *) self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;

	aprint_naive(": ACPI Battery (Control Method)\n");
	aprint_normal(": ACPI Battery (Control Method)\n");

	sc->sc_node = aa->aa_node;
	mutex_init(&sc->sc_mtx, MUTEX_DRIVER, IPL_NONE);

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
				      ACPI_ALL_NOTIFY,
				      acpibat_notify_handler, sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to register DEVICE/SYSTEM NOTIFY "
		    "handler: %s\n", sc->sc_dev.dv_xname,
		    AcpiFormatException(rv));
		return;
	}

#ifdef ACPI_BAT_DEBUG
	ABAT_SET(sc, ABAT_F_VERBOSE);
#endif

	acpibat_init_envsys(sc);
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

	sc->sc_data[ACPIBAT_DCAPACITY].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_LFCCAPACITY].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_CAPACITY].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_TECHNOLOGY].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_DVOLTAGE].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_WCAPACITY].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_LCAPACITY].state = ENVSYS_SINVALID;
}

static void
acpibat_clear_stat(struct acpibat_softc *sc)
{
	if (sc->sc_available > ABAT_ALV_INFO)
		sc->sc_available = ABAT_ALV_INFO;

	sc->sc_data[ACPIBAT_CHARGERATE].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_CAPACITY].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_VOLTAGE].state = ENVSYS_SINVALID;
	sc->sc_data[ACPIBAT_CHARGING].state = ENVSYS_SINVALID;
}


/*
 * returns 0 for no battery, 1 for present, and -1 on error
 */
static int
acpibat_battery_present(struct acpibat_softc *sc)
{
	uint32_t sta;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_STA", &val);
	if (ACPI_FAILURE(rv)) {
		printf("%s: failed to evaluate _STA: %s\n",
		       sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return -1;
	}

	sta = (uint32_t)val;

	mutex_enter(&sc->sc_mtx);
	sc->sc_available = ABAT_ALV_PRESENCE;
	if (sta & ACPIBAT_STA_PRESENT) {
		ABAT_SET(sc, ABAT_F_PRESENT);
		sc->sc_data[ACPIBAT_PRESENT].state = ENVSYS_SVALID;
		sc->sc_data[ACPIBAT_PRESENT].value_cur = 1;
	} else
		sc->sc_data[ACPIBAT_PRESENT].state = ENVSYS_SINVALID;

	mutex_exit(&sc->sc_mtx);

	return (sta & ACPIBAT_STA_PRESENT) ? 1 : 0;
}

/*
 * acpibat_get_info
 *
 * 	Get, and possibly display, the battery info.
 */

static ACPI_STATUS
acpibat_get_info(struct acpibat_softc *sc)
{
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	int capunit, rateunit;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BIF", &buf);
	if (ACPI_FAILURE(rv)) {
		printf("%s: failed to evaluate _BIF: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return rv;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		printf("%s: expected PACKAGE, got %d\n", sc->sc_dev.dv_xname,
		    p1->Type);
		goto out;
	}
	if (p1->Package.Count < 13) {
		printf("%s: expected 13 elts, got %d\n",
		    sc->sc_dev.dv_xname, p1->Package.Count);
		goto out;
	}
	p2 = p1->Package.Elements;

	mutex_enter(&sc->sc_mtx);
	if ((p2[0].Integer.Value & ACPIBAT_PWRUNIT_MA) != 0) {
		ABAT_SET(sc, ABAT_F_PWRUNIT_MA);
		capunit = ENVSYS_SAMPHOUR;
		rateunit = ENVSYS_SAMPS;
	} else {
		ABAT_CLEAR(sc, ABAT_F_PWRUNIT_MA);
		capunit = ENVSYS_SWATTHOUR;
		rateunit = ENVSYS_SWATTS;
	}

	sc->sc_data[ACPIBAT_DCAPACITY].units = capunit;
	sc->sc_data[ACPIBAT_LFCCAPACITY].units = capunit;
	sc->sc_data[ACPIBAT_WCAPACITY].units = capunit;
	sc->sc_data[ACPIBAT_LCAPACITY].units = capunit;
	sc->sc_data[ACPIBAT_CHARGERATE].units = rateunit;
	sc->sc_data[ACPIBAT_CHARGERATE].flags |= ENVSYS_FMONNOTSUPP;
	sc->sc_data[ACPIBAT_DISCHARGERATE].units = rateunit;
	sc->sc_data[ACPIBAT_DISCHARGERATE].flags |= ENVSYS_FMONNOTSUPP;
	sc->sc_data[ACPIBAT_CAPACITY].units = capunit;

	sc->sc_data[ACPIBAT_DCAPACITY].value_cur = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_DCAPACITY].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_DCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_data[ACPIBAT_LFCCAPACITY].value_cur = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LFCCAPACITY].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_LFCCAPACITY].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_data[ACPIBAT_CAPACITY].value_max = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_TECHNOLOGY].value_cur = p2[3].Integer.Value;
	sc->sc_data[ACPIBAT_TECHNOLOGY].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_DVOLTAGE].value_cur = p2[4].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_DVOLTAGE].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_DVOLTAGE].flags = ENVSYS_FMONNOTSUPP;
	sc->sc_data[ACPIBAT_WCAPACITY].value_cur = p2[5].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_WCAPACITY].value_max = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_WCAPACITY].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_WCAPACITY].flags |=
	    (ENVSYS_FPERCENT|ENVSYS_FVALID_MAX|ENVSYS_FMONNOTSUPP);
	sc->sc_data[ACPIBAT_LCAPACITY].value_cur = p2[6].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LCAPACITY].value_max = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LCAPACITY].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_LCAPACITY].flags |=
	    (ENVSYS_FPERCENT|ENVSYS_FVALID_MAX|ENVSYS_FMONNOTSUPP);
	sc->sc_available = ABAT_ALV_INFO;

	mutex_exit(&sc->sc_mtx);

	aprint_normal("%s: battery info: %s, %s, %s", sc->sc_dev.dv_xname,
	    p2[12].String.Pointer, p2[11].String.Pointer, p2[9].String.Pointer);
	if (p2[10].String.Pointer)
		aprint_normal(" %s", p2[10].String.Pointer);

	aprint_normal("\n");

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
acpibat_get_status(struct acpibat_softc *sc)
{
	int status, battrate;
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BST", &buf);
	if (ACPI_FAILURE(rv)) {
		printf("%s: failed to evaluate _BST: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return rv;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		printf("bat: expected PACKAGE, got %d\n", p1->Type);
		rv = AE_ERROR;
		goto out;
	}
	if (p1->Package.Count < 4) {
		printf("bat: expected 4 elts, got %d\n", p1->Package.Count);
		rv = AE_ERROR;
		goto out;
	}
	p2 = p1->Package.Elements;

	mutex_enter(&sc->sc_mtx);

	status = p2[0].Integer.Value;
	battrate = p2[1].Integer.Value;

	if (status & ACPIBAT_ST_CHARGING) {
		sc->sc_data[ACPIBAT_CHARGERATE].state = ENVSYS_SVALID;
		sc->sc_data[ACPIBAT_CHARGERATE].value_cur = battrate * 1000;
		sc->sc_data[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_data[ACPIBAT_CHARGING].value_cur = 1;
	} else if (status & ACPIBAT_ST_DISCHARGING) {
		sc->sc_data[ACPIBAT_DISCHARGERATE].state = ENVSYS_SVALID;
		sc->sc_data[ACPIBAT_DISCHARGERATE].value_cur = battrate * 1000;
		sc->sc_data[ACPIBAT_CHARGING].state = ENVSYS_SINVALID;
	}

	sc->sc_data[ACPIBAT_CAPACITY].value_cur = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_CAPACITY].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_CAPACITY].flags |=
	    (ENVSYS_FPERCENT|ENVSYS_FVALID_MAX);
	sc->sc_data[ACPIBAT_CAPACITY].monitor = true;
	sc->sc_data[ACPIBAT_CAPACITY].flags |=
	    (ENVSYS_FMONCRITICAL|ENVSYS_FMONCRITUNDER|ENVSYS_FMONWARNUNDER);
	sc->sc_data[ACPIBAT_VOLTAGE].value_cur = p2[3].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_VOLTAGE].state = ENVSYS_SVALID;
	sc->sc_data[ACPIBAT_VOLTAGE].flags = ENVSYS_FMONNOTSUPP;

	if (sc->sc_data[ACPIBAT_CAPACITY].value_cur <
	    sc->sc_data[ACPIBAT_WCAPACITY].value_cur)
		sc->sc_data[ACPIBAT_CAPACITY].state = ENVSYS_SWARNUNDER;

	if (sc->sc_data[ACPIBAT_CAPACITY].value_cur <
	    sc->sc_data[ACPIBAT_LCAPACITY].value_cur)
		sc->sc_data[ACPIBAT_CAPACITY].state = ENVSYS_SCRITUNDER;

	if (status & ACPIBAT_ST_CRITICAL)
		sc->sc_data[ACPIBAT_CAPACITY].state = ENVSYS_SCRITICAL;

	mutex_exit(&sc->sc_mtx);

	rv = AE_OK;

out:
	AcpiOsFree(buf.Pointer);
	return rv;
}

#define SCALE(x)	((x)/1000000), (((x)%1000000)/1000)
#define CAPUNITS(sc)	(ABAT_ISSET((sc), ABAT_F_PWRUNIT_MA)?"Ah":"Wh")
#define RATEUNITS(sc)	(ABAT_ISSET((sc), ABAT_F_PWRUNIT_MA)?"A":"W")
static void
acpibat_print_info(struct acpibat_softc *sc)
{
	const char *tech;

	if (sc->sc_data[ACPIBAT_TECHNOLOGY].value_cur)
		tech = "secondary";
	else
		tech = "primary";

	aprint_normal("%s: %s battery, Design %d.%03d%s, Last full %d.%03d%s "
	       "Warn %d.%03d%s Low %d.%03d%s\n",
	       sc->sc_dev.dv_xname, tech,
	       SCALE(sc->sc_data[ACPIBAT_DCAPACITY].value_cur), CAPUNITS(sc),
	       SCALE(sc->sc_data[ACPIBAT_LFCCAPACITY].value_cur),CAPUNITS(sc),
	       SCALE(sc->sc_data[ACPIBAT_WCAPACITY].value_cur), CAPUNITS(sc),
	       SCALE(sc->sc_data[ACPIBAT_LCAPACITY].value_cur), CAPUNITS(sc));
}

static void
acpibat_print_stat(struct acpibat_softc *sc)
{
	const char *capstat, *chargestat;
	int percent, denom;

	percent = 0;

	if (sc->sc_data[ACPIBAT_CAPACITY].state == ENVSYS_SCRITUNDER)
		capstat = "CRITICAL UNDER ";
	else if (sc->sc_data[ACPIBAT_CAPACITY].state == ENVSYS_SCRITOVER)
		capstat = "CRITICAL OVER ";
	else
		capstat = "";

	if (sc->sc_data[ACPIBAT_CHARGING].state == ENVSYS_SVALID)
		chargestat = "charging";
	else if (!sc->sc_data[ACPIBAT_CHARGING].state == ENVSYS_SINVALID)
		chargestat = "discharging";
	else
		chargestat = "idling";

	denom = sc->sc_data[ACPIBAT_DCAPACITY].value_cur / 100;
	if (denom > 0)
		percent = (sc->sc_data[ACPIBAT_CAPACITY].value_cur) / denom;

	aprint_normal("%s: %s%s: %d.%03dV cap %d.%03d%s (%d%%) "
	    "rate %d.%03d%s\n", sc->sc_dev.dv_xname, capstat, chargestat,
	    SCALE(sc->sc_data[ACPIBAT_VOLTAGE].value_cur),
	    SCALE(sc->sc_data[ACPIBAT_CAPACITY].value_cur), CAPUNITS(sc),
	    percent,
	    SCALE(sc->sc_data[ACPIBAT_CHARGING].value_cur ?
	        sc->sc_data[ACPIBAT_CHARGERATE].value_cur :
	        sc->sc_data[ACPIBAT_CHARGING].value_cur ?
	        sc->sc_data[ACPIBAT_DISCHARGERATE].value_cur : 0),
	        RATEUNITS(sc));
}

static void
acpibat_update(void *arg)
{
	struct acpibat_softc *sc = arg;

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
			if (acpibat_battery_present(sc) < 0) {
				/* error */
				aprint_debug("%s: cannot get battery "
				    "presence.\n", sc->sc_dev.dv_xname);
				return;
			}

		if (ABAT_ISSET(sc, ABAT_F_PRESENT)) {
			/* the battery is present. */
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				aprint_debug("%s: battery is present.\n",
				    sc->sc_dev.dv_xname);
			if (ACPI_FAILURE(acpibat_get_info(sc)))
				return;
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				acpibat_print_info(sc);
		} else {
			/* the battery is not present. */
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				aprint_debug("%s: battery is not present.\n",
				    sc->sc_dev.dv_xname);
			return;
		}
	} else {
		/* current information is valid */
		if (!ABAT_ISSET(sc, ABAT_F_PRESENT)) {
			/* the battery is not present. */
			return;
		}
 	}

	if (ACPI_FAILURE(acpibat_get_status(sc)))
		return;

	if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
		acpibat_print_stat(sc);
}

/*
 * acpibat_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpibat_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	struct acpibat_softc *sc = context;
	int rv;

#ifdef ACPI_BAT_DEBUG
	printf("%s: received notify message: 0x%x\n",
	       sc->sc_dev.dv_xname, notify);
#endif

	switch (notify) {
	case ACPI_NOTIFY_BusCheck:
		break;

	case ACPI_NOTIFY_DeviceCheck:
	case ACPI_NOTIFY_BatteryInformationChanged:
		mutex_enter(&sc->sc_mtx);
		acpibat_clear_presence(sc);
		mutex_exit(&sc->sc_mtx);
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
					     acpibat_update, sc);
		if (ACPI_FAILURE(rv))
			printf("%s: unable to queue status check: %s\n",
			       sc->sc_dev.dv_xname, AcpiFormatException(rv));
		break;

	case ACPI_NOTIFY_BatteryStatusChanged:
		mutex_enter(&sc->sc_mtx);
		acpibat_clear_stat(sc);
		mutex_exit(&sc->sc_mtx);
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
					     acpibat_update, sc);
		if (ACPI_FAILURE(rv))
			printf("%s: unable to queue status check: %s\n",
			       sc->sc_dev.dv_xname, AcpiFormatException(rv));
		break;

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		       sc->sc_dev.dv_xname, notify);
	}
}

static void
acpibat_init_envsys(struct acpibat_softc *sc)
{
	int capunit, rateunit;

	if (sc->sc_flags & ABAT_F_PWRUNIT_MA) {
		capunit = ENVSYS_SAMPHOUR;
		rateunit = ENVSYS_SAMPS;
	} else {
		capunit = ENVSYS_SWATTHOUR;
		rateunit = ENVSYS_SWATTS;
	}

#define INITDATA(index, unit, string) \
	sc->sc_data[index].sensor = index;				\
	sc->sc_data[index].units = unit;     				\
	sc->sc_data[index].state = ENVSYS_SVALID;			\
	snprintf(sc->sc_data[index].desc, sizeof(sc->sc_data->desc),	\
	    "%s %s", sc->sc_dev.dv_xname, string);			\

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
	INITDATA(ACPIBAT_CHARGING, ENVSYS_INDICATOR, "charging");

#undef INITDATA

	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_name = sc->sc_dev.dv_xname;	
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = acpibat_gtredata;
	sc->sc_sysmon.sme_nsensors = ACPIBAT_NSENSORS;

	sc->sc_updateinterval.tv_sec = 1;
	sc->sc_updateinterval.tv_usec = 0;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		aprint_error("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

static int
acpibat_gtredata(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct acpibat_softc *sc = sme->sme_cookie;

	if (ratecheck(&sc->sc_lastupdate, &sc->sc_updateinterval))
		acpibat_update(sc);

	return 0;
}
