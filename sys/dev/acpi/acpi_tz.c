/* $NetBSD: acpi_tz.c,v 1.11 2004/05/01 12:03:48 kochi Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: acpi_tz.c,v 1.11 2004/05/01 12:03:48 kochi Exp $");

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

/* constants */
#define ATZ_TZP_RATE	300	/* default if no _TZP CM present (30 secs) */
#define ATZ_NLEVELS	10	/* number of cooling levels, from ACPI spec */

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
	int sc_flags;
	int sc_rate;		/* tz poll rate */
};

static void	acpitz_get_status(void *);
static void	acpitz_print_status(struct acpitz_softc *);
static void	acpitz_notify_handler(ACPI_HANDLE, UINT32, void *);
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
acpitz_match(struct device *parent, struct cfdata *match, void *aux)
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
acpitz_attach(struct device *parent, struct device *self, void *aux)
{
	struct acpitz_softc *sc = (struct acpitz_softc *)self;
	struct acpi_attach_args *aa = aux;
	ACPI_STATUS rv;
	ACPI_INTEGER v;

#if 0
	sc->sc_flags = ATZ_F_VERBOSE;
#endif
	sc->sc_devnode = aa->aa_node;

	printf(": ACPI Thermal Zone\n");

	rv = acpi_eval_integer(sc->sc_devnode->ad_handle, "_TZP", &v);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to get polling interval; using default of",
		    sc->sc_dev.dv_xname);
		sc->sc_zone.tzp = ATZ_TZP_RATE;
	} else {
		sc->sc_zone.tzp = v;
		printf("%s: polling interval is", sc->sc_dev.dv_xname);
	}
	printf(" %d.%ds\n", sc->sc_zone.tzp / 10, sc->sc_zone.tsp % 10);

	/* XXX a value of 0 means "polling is not necessary" */
	if (sc->sc_zone.tzp == 0)
		sc->sc_zone.tzp = ATZ_TZP_RATE;

	acpitz_get_status(sc);

	rv = AcpiInstallNotifyHandler(sc->sc_devnode->ad_handle,
	    ACPI_SYSTEM_NOTIFY, acpitz_notify_handler, sc);
	if (ACPI_FAILURE(rv)) {
		printf("%s: unable to install SYSTEM NOTIFY handler: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}

	callout_init(&sc->sc_callout);
	callout_reset(&sc->sc_callout, (sc->sc_zone.tzp / 10) * hz,
	    acpitz_tick, sc);

	acpitz_init_envsys(sc);
}

static void
acpitz_get_status(void *opaque)
{
	struct acpitz_softc *sc = opaque;
	ACPI_STATUS rv;
	ACPI_INTEGER v;

	rv = acpi_eval_integer(sc->sc_devnode->ad_handle, "_TMP", &v);
	if (ACPI_FAILURE(rv)) {
		printf("%s: failed to evaluate _TMP: %s\n",
		    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		return;
	}
	sc->sc_zone.tmp = v;

	/*
	 * The temperature unit for envsys(9) is microKelvin, so convert to
	 * that from ACPI's microKelvin. Also, the ACPI specification assumes
	 * that K = C + 273.2 rather than the nominal 273.15 used by envsys(9),
	 * so we correct for that too.
	 */
	sc->sc_data[ATZ_SENSOR_TEMP].cur.data_us =
	    sc->sc_zone.tmp * 100000 - 50000;
	sc->sc_data[ATZ_SENSOR_TEMP].validflags |= ENVSYS_FCURVALID;

	if (sc->sc_flags & ATZ_F_VERBOSE)
		acpitz_print_status(sc);

	return;
}

static void
acpitz_print_status(struct acpitz_softc *sc)
{

	printf("%s: zone temperature is now %d K\n", sc->sc_dev.dv_xname,
	    sc->sc_zone.tmp / 10);

	return;
}

static void
acpitz_notify_handler(ACPI_HANDLE hdl, UINT32 notify, void *opaque)
{
	struct acpitz_softc *sc = opaque;
	int rv;

	switch (notify) {
	case ACPI_NOTIFY_ThermalZoneStatusChanged:
	case ACPI_NOTIFY_ThermalZoneTripPointsChanged:
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
		    acpitz_get_status, sc);
		if (ACPI_FAILURE(rv)) {
			printf("%s: unable to queue status check: %s\n",
			    sc->sc_dev.dv_xname, AcpiFormatException(rv));
		}
		break;
	default:
		printf("%s: received unhandled notify message 0x%x\n",
		    sc->sc_dev.dv_xname, notify);
		break;
	}

	return;
}

static void
acpitz_tick(void *opaque)
{
	struct acpitz_softc *sc = opaque;

	callout_reset(&sc->sc_callout, (sc->sc_zone.tzp / 10) * hz,
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
acpitz_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *binfo)
{

	/* XXX not implemented */
	binfo->validflags = 0;

	return 0;
}
