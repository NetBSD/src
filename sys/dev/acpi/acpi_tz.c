/* $NetBSD: acpi_tz.c,v 1.19 2006/10/12 01:30:54 christos Exp $ */

/*
 * Copyright (c) 2003 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * ACPI Thermal Zone driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_tz.c,v 1.19 2006/10/12 01:30:54 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/lock.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

/* flags */
#define ATZ_F_VERBOSE		0x01	/* show events to console */
#define ATZ_F_CRITICAL		0x02	/* zone critical */
#define ATZ_F_HOT		0x04	/* zone hot */
#define ATZ_F_PASSIVE		0x08	/* zone passive cooling */
#define ATZ_F_PASSIVEONLY	0x10	/* zone is passive cooling only */

/* no active cooling level */
#define ATZ_ACTIVE_NONE -1

/* constants */
#define ATZ_TZP_RATE	300	/* default if no _TZP CM present (30 secs) */
#define ATZ_NLEVELS	10	/* number of cooling levels, from ACPI spec */
#define ATZ_ZEROC	2732	/* 0C in tenths degrees Kelvin */
#define ATZ_TMP_INVALID	0xffffffff	/* invalid temperature */
#define ATZ_ZONE_EXPIRE	9000	/* zone info refetch interval (15min) */

/* sensor indexes */
#define ATZ_SENSOR_TEMP	0	/* thermal zone temperature */
#define ATZ_NUMSENSORS	1	/* number of sensors */

static const struct envsys_range acpitz_ranges[] = {
	{ 0, 1,	ATZ_SENSOR_TEMP },
};

static int	acpitz_match(struct device *, struct cfdata *, void *);
static void	acpitz_attach(struct device *, struct device *, void *);

/*
 * ACPI Temperature Zone information. Note all temperatures are reported
 * in tenths of degrees Kelvin
 */
struct acpitz_zone {
	/* Active cooling temperature threshold */
	UINT32 ac[ATZ_NLEVELS];
	/* Package of references to all active cooling devices for a level */
	ACPI_BUFFER al[ATZ_NLEVELS];
	/* Critical temperature threshold for system shutdown */
	UINT32 crt;
	/* Critical temperature threshold for S4 sleep */
	UINT32 hot;
	/* Package of references to processor objects for passive cooling */
	ACPI_BUFFER psl;
	/* Passive cooling temperature threshold */
	UINT32 psv;
	/* Thermal constants for use in passive cooling formulas */
	UINT32 tc1, tc2;
	/* Current temperature of the thermal zone */
	UINT32 tmp;
	/* Thermal sampling period for passive cooling, in tenths of seconds */
	UINT32 tsp;
	/* Package of references to devices in this TZ (optional) */
	ACPI_BUFFER tzd;
	/* Recommended TZ polling frequency, in tenths of seconds */
	UINT32 tzp;
};

struct acpitz_softc {
	struct device sc_dev;
	struct acpi_devnode *sc_devnode;
	struct acpitz_zone sc_zone;
	struct callout sc_callout;
	struct envsys_tre_data sc_data[ATZ_NUMSENSORS];
	struct envsys_basic_info sc_info[ATZ_NUMSENSORS];
	struct sysmon_envsys sc_sysmon;
	struct simplelock sc_slock;
	int sc_active;		/* active cooling level */
	int sc_flags;
	int sc_rate;		/* tz poll rate */
	int sc_zone_expire;
};

static void	acpitz_get_status(void *);
static void	acpitz_get_zone(void *, int);
static void	acpitz_get_zone_quiet(void *);
static char*	acpitz_celcius_string(int);
static void	acpitz_print_status(struct acpitz_softc *);
static void	acpitz_power_off(struct acpitz_softc *);
static void	acpitz_power_zone(struct acpitz_softc *, int, int);
static void	acpitz_sane_temp(UINT32 *tmp);
static ACPI_STATUS
		acpitz_switch_cooler(ACPI_OBJECT *, void *);
static void	acpitz_notify_handler(ACPI_HANDLE, UINT32, void *);
static int	acpitz_get_integer(struct acpitz_softc *, const char *, UINT32 *);
static void	acpitz_tick(void *);
static void	acpitz_init_envsys(struct acpitz_softc *);
static int	acpitz_gtredata(struct sysmon_envsys *,
				struct envsys_tre_data *);
static int	acpitz_streinfo(struct sysmon_envsys *,
				struct envsys_basic_info *);

CFATTACH_DECL(acpitz, sizeof(struct acpitz_softc), acpitz_match,
    acpitz_attach, NULL, NULL);

/*
 * acpitz_match: autoconf(9) match routine
 */
static int
acpitz_match(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_THERMAL)
		return 0;

	return 1;
}

/*
 * acpitz_attach: autoconf(9) attach routine
 */
static void
acpitz_attach(struct device *parent __unused, struct device *self, void *aux)
{
	struct acpitz_softc *sc = (struct acpitz_softc *)self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;
	ACPI_INTEGER v;

#if 0
	sc->sc_flags = ATZ_F_VERBOSE;
#endif
	sc->sc_devnode = aa->aa_node;

	aprint_naive(": ACPI Thermal Zone\n");
	aprint_normal(": ACPI Thermal Zone\n");

	rv = acpi_eval_integer(sc->sc_devnode->ad_handle, "_TZP", &v);
	if (ACPI_FAILURE(rv)) {
		aprint_verbose("%s: unable to get polling interval; using default of",
		    sc->sc_dev.dv_xname);
		sc->sc_zone.tzp = ATZ_TZP_RATE;
	} else {
		sc->sc_zone.tzp = v;
		aprint_verbose("%s: polling interval is", sc->sc_dev.dv_xname);
	}
	aprint_verbose(" %d.%ds\n", sc->sc_zone.tzp / 10, sc->sc_zone.tzp % 10);

	/* XXX a value of 0 means "polling is not necessary" */
	if (sc->sc_zone.tzp == 0)
		sc->sc_zone.tzp = ATZ_TZP_RATE;

	sc->sc_zone_expire = ATZ_ZONE_EXPIRE / sc->sc_zone.tzp;

	acpitz_get_zone(sc, 1);
	acpitz_get_status(sc);

	rv = AcpiInstallNotifyHandler(sc->sc_devnode->ad_handle,
	    ACPI_SYSTEM_NOTIFY, acpitz_notify_handler, sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error("%s: unable to install SYSTEM NOTIFY handler: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}

	callout_init(&sc->sc_callout);
	callout_reset(&sc->sc_callout, sc->sc_zone.tzp * hz / 10,
	    acpitz_tick, sc);

	acpitz_init_envsys(sc);
}

static void
acpitz_get_zone_quiet(void *opaque)
{
	acpitz_get_zone(opaque, 0);
}

static void
acpitz_get_status(void *opaque)
{
	struct acpitz_softc *sc = opaque;
	UINT32 tmp, active;
	int i, flags;

	sc->sc_zone_expire--;
	if (sc->sc_zone_expire <= 0) {
		sc->sc_zone_expire = ATZ_ZONE_EXPIRE / sc->sc_zone.tzp;
		if (sc->sc_flags & ATZ_F_VERBOSE)
			printf("%s: force refetch zone\n", sc->sc_dev.dv_xname);
		acpitz_get_zone(sc, 0);
	}

	if (acpitz_get_integer(sc, "_TMP", &tmp)) {
		printf("%s: failed to evaluate _TMP\n", sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_zone.tmp = tmp;
	/* XXX sanity check for tmp here? */

	/*
	 * The temperature unit for envsys(4) is microKelvin, so convert to
	 * that from ACPI's microKelvin. Also, the ACPI specification assumes
	 * that K = C + 273.2 rather than the nominal 273.15 used by envsys(4),
	 * so we correct for that too.
	 */
	sc->sc_data[ATZ_SENSOR_TEMP].cur.data_us =
	    sc->sc_zone.tmp * 100000 - 50000;
	sc->sc_data[ATZ_SENSOR_TEMP].validflags |= ENVSYS_FCURVALID;

	if (sc->sc_flags & ATZ_F_VERBOSE)
		acpitz_print_status(sc);

	if (sc->sc_flags & ATZ_F_PASSIVEONLY) {
		/* Passive Cooling: XXX not yet */

	} else {
		/* Active Cooling */

		/* temperature threshold: _AC0 > ... > _AC9 */
		active = ATZ_ACTIVE_NONE;
		for (i = ATZ_NLEVELS - 1; i >= 0; i--) {
			if (sc->sc_zone.ac[i] == ATZ_TMP_INVALID)
				continue;

			/* we want to keep highest cooling mode in 'active' */
			if (sc->sc_zone.ac[i] <= tmp)
				active = i;
		}

		flags = sc->sc_flags & ~(ATZ_F_CRITICAL|ATZ_F_HOT|ATZ_F_PASSIVE);
		if (sc->sc_zone.psv != ATZ_TMP_INVALID &&
		    tmp >= sc->sc_zone.psv)
			flags |= ATZ_F_PASSIVE;
		if (sc->sc_zone.hot != ATZ_TMP_INVALID &&
		    tmp >= sc->sc_zone.hot)
			flags |= ATZ_F_HOT;
		if (sc->sc_zone.crt != ATZ_TMP_INVALID &&
		    tmp >= sc->sc_zone.crt)
			flags |= ATZ_F_CRITICAL;

		if (flags != sc->sc_flags) {
			int changed = (sc->sc_flags ^ flags) & flags;
			sc->sc_flags = flags;
			if (changed & ATZ_F_CRITICAL)
				printf("%s: zone went critical at temp %sC\n",
				    sc->sc_dev.dv_xname,
				    acpitz_celcius_string(tmp));
			else if (changed & ATZ_F_HOT)
				printf("%s: zone went hot at temp %sC\n",
				    sc->sc_dev.dv_xname,
				    acpitz_celcius_string(tmp));
		}

		/* power on fans */
		if (sc->sc_active != active) {
			if (sc->sc_active != ATZ_ACTIVE_NONE)
				acpitz_power_zone(sc, sc->sc_active, 0);

			if (active != ATZ_ACTIVE_NONE) {
				if (sc->sc_flags & ATZ_F_VERBOSE)
					printf("%s: active cooling level %u\n",
					    sc->sc_dev.dv_xname, active);
				acpitz_power_zone(sc, active, 1);
			}

			sc->sc_active = active;
		}
	}

	return;
}

static char *
acpitz_celcius_string(int dk)
{
	static char buf[10];

	snprintf(buf, sizeof(buf), "%d.%d", (dk - ATZ_ZEROC) / 10,
	    (dk - ATZ_ZEROC) % 10);

	return buf;
}

static void
acpitz_print_status(struct acpitz_softc *sc)
{

	printf("%s: zone temperature is now %sC\n", sc->sc_dev.dv_xname,
	    acpitz_celcius_string(sc->sc_zone.tmp));

	return;
}

static ACPI_STATUS
acpitz_switch_cooler(ACPI_OBJECT *obj, void *arg)
{
	ACPI_HANDLE cooler;
	ACPI_STATUS rv;
	int pwr_state, flag;

	flag = *(int *)arg;

	if (flag)
		pwr_state = ACPI_STATE_D0;
	else
		pwr_state = ACPI_STATE_D3;

	switch(obj->Type) {
	case ACPI_TYPE_ANY:
		cooler = obj->Reference.Handle;
		break;
	case ACPI_TYPE_STRING:
		rv = AcpiGetHandle(NULL, obj->String.Pointer, &cooler);
		if (ACPI_FAILURE(rv)) {
			printf("failed to get handler from %s\n",
			    obj->String.Pointer);
			return rv;
		}
		break;
	default:
		printf("unknown power type: %d\n", obj->Type);
		return AE_OK;
	}

	rv = acpi_pwr_switch_consumer(cooler, pwr_state);
	if (rv != AE_BAD_PARAMETER && ACPI_FAILURE(rv)) {
		printf("failed to change state for %s: %s\n",
		    acpi_name(obj->Reference.Handle),
		    AcpiFormatException(rv));
	}

	return AE_OK;
}

/*
 * acpitz_power_zone:
 *	power on or off the i:th part of the zone zone
 */
static void
acpitz_power_zone(struct acpitz_softc *sc, int i, int on)
{
	KASSERT(i >= 0 && i < ATZ_NLEVELS);

	acpi_foreach_package_object(sc->sc_zone.al[i].Pointer,
	    acpitz_switch_cooler, &on);
}


/*
 * acpitz_power_off:
 *	power off parts of the zone
 */
static void
acpitz_power_off(struct acpitz_softc *sc)
{
	int i;

	for (i = 0 ; i < ATZ_NLEVELS; i++) {
		if (sc->sc_zone.al[i].Pointer == NULL)
			continue;
		acpitz_power_zone(sc, i, 0);
	}
	sc->sc_active = ATZ_ACTIVE_NONE;
	sc->sc_flags &= ~(ATZ_F_CRITICAL|ATZ_F_HOT|ATZ_F_PASSIVE);
}

static void
acpitz_get_zone(void *opaque, int verbose)
{
	struct acpitz_softc *sc = opaque;
	ACPI_STATUS rv;
	char buf[8];
	int i, valid_levels;
	static int first = 1;

	if (!first) {
		acpitz_power_off(sc);

		for (i = 0; i < ATZ_NLEVELS; i++) {
			if (sc->sc_zone.al[i].Pointer != NULL)
				AcpiOsFree(sc->sc_zone.al[i].Pointer);
			sc->sc_zone.al[i].Pointer = NULL;
		}
	}

	valid_levels = 0;

	for (i = 0; i < ATZ_NLEVELS; i++) {
		ACPI_OBJECT *obj;

		snprintf(buf, sizeof(buf), "_AC%d", i);
		if (acpitz_get_integer(sc, buf, &sc->sc_zone.ac[i]))
			continue;

		snprintf(buf, sizeof(buf), "_AL%d", i);
		rv = acpi_eval_struct(sc->sc_devnode->ad_handle, buf,
		    &sc->sc_zone.al[i]);
		if (ACPI_FAILURE(rv)) {
			printf("failed getting _AL%d", i);
			sc->sc_zone.al[i].Pointer = NULL;
			continue;
		}

		obj = sc->sc_zone.al[i].Pointer;
		if (obj != NULL) {
			if (obj->Type != ACPI_TYPE_PACKAGE) {
				printf("%s: ac%d not package\n",
				    sc->sc_dev.dv_xname, i);
				AcpiOsFree(obj);
				sc->sc_zone.al[i].Pointer = NULL;
				continue;
			}
		}

		if (first)
			printf("%s: active cooling level %d: %sC\n",
			    sc->sc_dev.dv_xname, i,
			    acpitz_celcius_string(sc->sc_zone.ac[i]));

		valid_levels++;
	}

	if (valid_levels == 0) {
		sc->sc_flags |= ATZ_F_PASSIVEONLY;
		if (first)
			printf("%s: passive cooling mode only\n",
			    sc->sc_dev.dv_xname);
	}

	acpitz_get_integer(sc, "_TMP", &sc->sc_zone.tmp);
	acpitz_get_integer(sc, "_CRT", &sc->sc_zone.crt);
	acpitz_get_integer(sc, "_HOT", &sc->sc_zone.hot);
	sc->sc_zone.psl.Length = ACPI_ALLOCATE_BUFFER;
	sc->sc_zone.psl.Pointer = NULL;
	AcpiEvaluateObject(sc, "_PSL", NULL, &sc->sc_zone.psl);
	acpitz_get_integer(sc, "_PSV", &sc->sc_zone.psv);
	acpitz_get_integer(sc, "_TC1", &sc->sc_zone.tc1);
	acpitz_get_integer(sc, "_TC2", &sc->sc_zone.tc2);

	acpitz_sane_temp(&sc->sc_zone.tmp);
	acpitz_sane_temp(&sc->sc_zone.crt);
	acpitz_sane_temp(&sc->sc_zone.hot);
	acpitz_sane_temp(&sc->sc_zone.psv);

	if (verbose) {
		printf("%s:", sc->sc_dev.dv_xname);
		if (sc->sc_zone.crt != ATZ_TMP_INVALID)
			printf(" critical %sC",
			    acpitz_celcius_string(sc->sc_zone.crt));
		if (sc->sc_zone.hot != ATZ_TMP_INVALID)
			printf(" hot %sC",
			    acpitz_celcius_string(sc->sc_zone.hot));
		if (sc->sc_zone.psv != ATZ_TMP_INVALID)
			printf(" passive %sC",
			    acpitz_celcius_string(sc->sc_zone.tmp));
		printf("\n");
	}

	for (i = 0; i < ATZ_NLEVELS; i++)
		acpitz_sane_temp(&sc->sc_zone.ac[i]);

	acpitz_power_off(sc);
	first = 0;
}


static void
acpitz_notify_handler(ACPI_HANDLE hdl __unused, UINT32 notify, void *opaque)
{
	struct acpitz_softc *sc = opaque;
	ACPI_OSD_EXEC_CALLBACK func = NULL;
	const char *name;
	int rv;

	switch (notify) {
	case ACPI_NOTIFY_ThermalZoneStatusChanged:
		func = acpitz_get_status;
		name = "status check";
		break;
	case ACPI_NOTIFY_ThermalZoneTripPointsChanged:
	case ACPI_NOTIFY_DeviceListsChanged:
		func = acpitz_get_zone_quiet;
		name = "get zone";
		break;
	default:
		printf("%s: received unhandled notify message 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
		return;
	}

	KASSERT(func != NULL);

	rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO, func, sc);
	if (rv != AE_OK)
		printf("%s: unable to queue %s\n", sc->sc_dev.dv_xname, name);

	return;
}

static void
acpitz_sane_temp(UINT32 *tmp)
{
	/* Sane temperatures are beteen 0 and 150 C */
	if (*tmp < ATZ_ZEROC || *tmp > ATZ_ZEROC + 1500)
		*tmp = ATZ_TMP_INVALID;
}

static int
acpitz_get_integer(struct acpitz_softc *sc, const char *cm, UINT32 *val)
{
	ACPI_STATUS rv;
	ACPI_INTEGER tmp;

	rv = acpi_eval_integer(sc->sc_devnode->ad_handle, cm, &tmp);
	if (ACPI_FAILURE(rv)) {
#ifdef ACPI_DEBUG
		printf("%s: failed to evaluate %s: %s\n", sc->sc_dev.dv_xname,
		    cm, AcpiFormatException(rv));
#endif
		*val = ATZ_TMP_INVALID;
		return 1;
	}

	*val = tmp;

	return 0;
}

static void
acpitz_tick(void *opaque)
{
	struct acpitz_softc *sc = opaque;

	callout_reset(&sc->sc_callout, sc->sc_zone.tzp * hz / 10,
	    acpitz_tick, opaque);
	AcpiOsQueueForExecution(OSD_PRIORITY_LO, acpitz_get_status, sc);

	return;
}

static void
acpitz_init_envsys(struct acpitz_softc *sc)
{
	int i;

	simple_lock_init(&sc->sc_slock);

	for (i = 0; i < ATZ_NUMSENSORS; i++) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags = ENVSYS_FVALID;
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = ENVSYS_WARN_OK;
	}
#define INITDATA(index, unit, string) \
	sc->sc_data[index].units = unit;				   \
	sc->sc_info[index].units = unit;				   \
	snprintf(sc->sc_info[index].desc, sizeof(sc->sc_info[index].desc), \
	    "%s %s", sc->sc_dev.dv_xname, string);

	INITDATA(ATZ_SENSOR_TEMP, ENVSYS_STEMP, "temperature");

	/* hook into sysmon */
	sc->sc_sysmon.sme_ranges = acpitz_ranges;
	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = acpitz_gtredata;
	sc->sc_sysmon.sme_streinfo = acpitz_streinfo;
	sc->sc_sysmon.sme_nsensors = ATZ_NUMSENSORS;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

int
acpitz_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct acpitz_softc *sc = sme->sme_cookie;

	simple_lock(&sc->sc_slock);

	*tred = sc->sc_data[tred->sensor];

	simple_unlock(&sc->sc_slock);

	return 0;
}

static int
acpitz_streinfo(struct sysmon_envsys *sme __unused,
    struct envsys_basic_info *binfo)
{

	/* XXX not implemented */
	binfo->validflags = 0;

	return 0;
}
