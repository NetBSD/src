/*	$NetBSD: acpi_bat.c,v 1.1.2.6 2003/01/03 17:01:11 thorpej Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_bat.c,v 1.1.2.6 2003/01/03 17:01:11 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* for hz */
#include <sys/device.h>
#include <sys/callout.h>
#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

/* sensor indexes */
#define ACPIBAT_DCAPACITY	0
#define ACPIBAT_LFCCAPACITY	1
#define ACPIBAT_TECHNOLOGY	2
#define ACPIBAT_DVOLTAGE	3
#define ACPIBAT_WCAPACITY	4
#define ACPIBAT_LCAPACITY	5
#define ACPIBAT_VOLTAGE		6
#define ACPIBAT_LOAD		7
#define ACPIBAT_CAPACITY	8
#define ACPIBAT_CHARGING	9
#define ACPIBAT_DISCHARGING    10
#define ACPIBAT_NSENSORS       11  /* number of sensors */

const struct envsys_range acpibat_range_amp[] = {
	{ 0, 1,		ENVSYS_SVOLTS_DC },
	{ 1, 2,		ENVSYS_SAMPS },
	{ 2, 3,		ENVSYS_SAMPHOUR },
	{ 1, 0,		-1 },
};

const struct envsys_range acpibat_range_watt[] = {
	{ 0, 1,		ENVSYS_SVOLTS_DC },
	{ 1, 2,		ENVSYS_SWATTS },
	{ 2, 3,		ENVSYS_SWATTHOUR },
	{ 1, 0,		-1 },
};

#define	BAT_WORDS	13

struct acpibat_softc {
	struct device sc_dev;		/* base device glue */
	struct acpi_devnode *sc_node;	/* our ACPI devnode */
	int sc_flags;			/* see below */
	struct callout sc_callout; 	/* XXX temporary polling */
	int sc_present;			/* is battery present? */
	int sc_status;			/* power status */

	struct sysmon_envsys sc_sysmon;
	struct envsys_basic_info sc_info[ACPIBAT_NSENSORS];
	struct envsys_tre_data sc_data[ACPIBAT_NSENSORS];

	ACPI_OBJECT sc_Ret[BAT_WORDS];	/* Return Buffer */
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
#define ACPIBAT_STA_PRESENT	 0x00000010  /* battery present */

/*
 * These flags are used to set internal state in our softc.
 */
#define	ABAT_F_VERBOSE		0x01	/* verbose events */
#define ABAT_F_PWRUNIT_MA	0x02 	/* mA instead of mW */

#define ACM_RATEUNIT(sc) (((sc)->sc_flags & ABAT_F_PWRUNIT_MA)?"A":"W")
#define ACM_CAPUNIT(sc) (((sc)->sc_flags & ABAT_F_PWRUNIT_MA)?"Ah":"Wh")
#define ACM_SCALE(x)	((x) / 1000), ((x) % 1000)

int	acpibat_match(struct device *, struct cfdata *, void *);
void	acpibat_attach(struct device *, struct device *, void *);

CFATTACH_DECL(acpibat, sizeof(struct acpibat_softc),
    acpibat_match, acpibat_attach, NULL, NULL);

static void acpibat_get_status(void *);
static void acpibat_get_info(void *);
static void acpibat_init_envsys(struct acpibat_softc *);
void	acpibat_notify_handler(ACPI_HANDLE, UINT32, void *context);
static void acpibat_tick(void *);
static int acpibat_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);
static int acpibat_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);
static int acpibat_battery_present(void *);

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

	printf(": ACPI Battery (Control Method)\n");

	sc->sc_node = aa->aa_node;

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
				      ACPI_DEVICE_NOTIFY,
				      acpibat_notify_handler, sc);
	if (rv != AE_OK) {
		printf("%s: unable to register DEVICE NOTIFY handler: %d\n",
		       sc->sc_dev.dv_xname, rv);
		return;
	}

	/* XXX See acpibat_notify_handler() */
	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
				      ACPI_SYSTEM_NOTIFY,
				      acpibat_notify_handler, sc);
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
	acpibat_init_envsys(sc);
}

static void
acpibat_tick(void *arg)
{
	struct acpibat_softc *sc = arg;
	callout_reset(&sc->sc_callout, 60*hz, acpibat_tick, arg);
	AcpiOsQueueForExecution(OSD_PRIORITY_LO, acpibat_get_status, sc);
}

/*
 * returns 0 for no battery, 1 for present, and -1 on error
 */
static int
acpibat_battery_present(void *arg)
{
	struct acpibat_softc *sc = arg;
	u_int32_t sta;
	ACPI_OBJECT *p1;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	buf.Pointer = sc->sc_Ret;
	buf.Length = sizeof(sc->sc_Ret);

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "_STA", NULL, &buf);
	if (rv != AE_OK) {
		printf("%s: failed to evaluate _STA: %x\n",
		       sc->sc_dev.dv_xname, rv);
		sc->sc_present = -1;
		return (-1);
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_INTEGER) {
		printf("%s: expected INTEGER, got %d\n", sc->sc_dev.dv_xname,
		       p1->Type);
		sc->sc_present = -1;
		return (-1);
	}
	if (p1->Package.Count < 1) {
		printf("%s: expected 1 elts, got %d\n",
		       sc->sc_dev.dv_xname, p1->Package.Count);
		sc->sc_present = -1;
		return (-1);
	}
	sta = p1->Integer.Value;

	sc->sc_present = (sta & ACPIBAT_STA_PRESENT) ? 1 : 0;

	return (sc->sc_present);
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

	(void)acpibat_battery_present(sc);

	if (sc->sc_present != 1) {
		printf("%s: not present\n", sc->sc_dev.dv_xname);
		return;
	}

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
	if (p1->Package.Count < 13) {
		printf("%s: expected 13 elts, got %d\n",
		    sc->sc_dev.dv_xname, p1->Package.Count);
		goto out;
	}

	p2 = p1->Package.Elements;
	if ((p2[0].Integer.Value & ACPIBAT_PWRUNIT_MA) != 0)
		sc->sc_flags |= ABAT_F_PWRUNIT_MA;

	sc->sc_data[ACPIBAT_DCAPACITY].cur.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LFCCAPACITY].cur.data_s = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LFCCAPACITY].max.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_CAPACITY].max.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_TECHNOLOGY].cur.data_s = p2[3].Integer.Value;
	sc->sc_data[ACPIBAT_DVOLTAGE].cur.data_s = p2[4].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_WCAPACITY].cur.data_s = p2[5].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LCAPACITY].cur.data_s = p2[6].Integer.Value * 1000;

	printf("%s: %s %s %s %s\n",
	    sc->sc_dev.dv_xname,
	    p2[12].String.Pointer, p2[11].String.Pointer,
	    p2[9].String.Pointer, p2[10].String.Pointer);

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
	int flags;
	struct acpibat_softc *sc = arg;
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	if (sc->sc_present != 1) {
		return;
	}

	buf.Pointer = sc->sc_Ret;
	buf.Length = sizeof(sc->sc_Ret);

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "_BST", NULL, &buf);

	if (rv != AE_OK) {
		printf("bat: failed to evaluate _BST: %x\n", rv);
		return;
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		printf("bat: expected PACKAGE, got %d\n", p1->Type);
		return;
	}
	if (p1->Package.Count < 4) {
		printf("bat: expected 4 elts, got %d\n", p1->Package.Count);
		return;
	}
	p2 = p1->Package.Elements;

	sc->sc_status = p2[0].Integer.Value;
	sc->sc_data[ACPIBAT_LOAD].cur.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_CAPACITY].cur.data_s = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_VOLTAGE].cur.data_s = p2[3].Integer.Value * 1000;

	flags = 0;
	if (sc->sc_data[ACPIBAT_CAPACITY].cur.data_s < sc->sc_data[ACPIBAT_WCAPACITY].cur.data_s)
		flags |= ENVSYS_WARN_UNDER;
	if (sc->sc_status & 4)
		flags |= ENVSYS_WARN_CRITUNDER;
	sc->sc_data[ACPIBAT_CAPACITY].warnflags = flags;
	sc->sc_data[ACPIBAT_DISCHARGING].cur.data_s = ((sc->sc_status & ACPIBAT_ST_DISCHARGING) != 0);
	sc->sc_data[ACPIBAT_CHARGING].cur.data_s = ((sc->sc_status & ACPIBAT_ST_CHARGING) != 0);
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

#ifdef ACPI_BAT_DEBUG
	printf("%s: received notify message: 0x%x\n",
	       sc->sc_dev.dv_xname, notify);
#endif

	switch (notify) {
	case ACPI_NOTIFY_BusCheck:
		break;

	case ACPI_NOTIFY_BatteryInformationChanged:
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
					     acpibat_get_info, sc);
		if (rv != AE_OK)
			printf("%s: unable to queue status check: %d\n",
			       sc->sc_dev.dv_xname, rv);
		break;

	case ACPI_NOTIFY_BatteryStatusChanged:
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

static void
acpibat_init_envsys(struct acpibat_softc *sc)
{
	int capunit, rateunit, i;
	const char *capstring, *ratestring;

	if (sc->sc_flags & ABAT_F_PWRUNIT_MA) {
		sc->sc_sysmon.sme_ranges = acpibat_range_amp;
		capunit = ENVSYS_SAMPHOUR;
		capstring = "charge";
		rateunit = ENVSYS_SAMPS;
		ratestring = "current";
	} else {
		sc->sc_sysmon.sme_ranges = acpibat_range_watt;
		capunit = ENVSYS_SWATTHOUR;
		capstring = "energy";
		rateunit = ENVSYS_SWATTS;
		ratestring = "power";
	}

	for (i = 0 ; i < ACPIBAT_NSENSORS; i++) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags |= (ENVSYS_FVALID | ENVSYS_FCURVALID);
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = 0;
	}
#define INITDATA(index, unit, string) \
	sc->sc_data[index].units = unit;     				\
	sc->sc_info[index].units = unit;     				\
	snprintf(sc->sc_info[index].desc, sizeof(sc->sc_info->desc),	\
	    "%s %s", sc->sc_dev.dv_xname, string);			\

	INITDATA(ACPIBAT_DCAPACITY, capunit, "design cap");
	INITDATA(ACPIBAT_LFCCAPACITY, capunit, "lfc cap");
	INITDATA(ACPIBAT_TECHNOLOGY, ENVSYS_INTEGER, "technology");
	INITDATA(ACPIBAT_DVOLTAGE, ENVSYS_SVOLTS_DC, "design voltage");
	INITDATA(ACPIBAT_WCAPACITY, capunit, "warn cap");
	INITDATA(ACPIBAT_LCAPACITY, capunit, "low cap");
	INITDATA(ACPIBAT_VOLTAGE, ENVSYS_SVOLTS_DC, "voltage");
	INITDATA(ACPIBAT_LOAD, rateunit, ratestring);
	INITDATA(ACPIBAT_CAPACITY, capunit, capstring);
	INITDATA(ACPIBAT_CHARGING, ENVSYS_INDICATOR, "charging");
	INITDATA(ACPIBAT_DISCHARGING, ENVSYS_INDICATOR, "discharging");

	/*
	 * ACPIBAT_CAPACITY is the "gas gauge".
	 * ACPIBAT_LFCCAPACITY is the "wear gauge".
	 */
	sc->sc_data[ACPIBAT_CAPACITY].validflags |=
		ENVSYS_FMAXVALID | ENVSYS_FFRACVALID;
	sc->sc_data[ACPIBAT_LFCCAPACITY].validflags |=
		ENVSYS_FMAXVALID | ENVSYS_FFRACVALID;

	sc->sc_sysmon.sme_sensor_info = sc->sc_info;
	sc->sc_sysmon.sme_sensor_data = sc->sc_data;
	sc->sc_sysmon.sme_cookie = sc;
	sc->sc_sysmon.sme_gtredata = acpibat_gtredata;
	sc->sc_sysmon.sme_streinfo = acpibat_streinfo;
	sc->sc_sysmon.sme_nsensors = ACPIBAT_NSENSORS;
	sc->sc_sysmon.sme_envsys_version = 1000;

	if (sysmon_envsys_register(&sc->sc_sysmon))
		printf("%s: unable to register with sysmon\n",
		    sc->sc_dev.dv_xname);
}

int
acpibat_gtredata(struct sysmon_envsys *sme, struct envsys_tre_data *tred)
{
	struct acpibat_softc *sc = sme->sme_cookie;

	/* XXX locking */
	acpibat_get_status(sc);
	*tred = sc->sc_data[tred->sensor];
	/* XXX locking */

	return (0);
}


int
acpibat_streinfo(struct sysmon_envsys *sme, struct envsys_basic_info *binfo)
{

	/* XXX Not implemented */
	binfo->validflags = 0;

	return (0);
}
