/*	$NetBSD: acpi_bat.c,v 1.15 2003/02/16 16:50:09 tshiozak Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_bat.c,v 1.15 2003/02/16 16:50:09 tshiozak Exp $");

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
#define ACPIBAT_PRESENT		0
#define ACPIBAT_DCAPACITY	1
#define ACPIBAT_LFCCAPACITY	2
#define ACPIBAT_TECHNOLOGY	3
#define ACPIBAT_DVOLTAGE	4
#define ACPIBAT_WCAPACITY	5
#define ACPIBAT_LCAPACITY	6
#define ACPIBAT_VOLTAGE		7
#define ACPIBAT_LOAD		8
#define ACPIBAT_CAPACITY	9
#define ACPIBAT_CHARGING       10
#define ACPIBAT_DISCHARGING    11
#define ACPIBAT_NSENSORS       12  /* number of sensors */

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
	int sc_available;		/* available information level */

	struct sysmon_envsys sc_sysmon;
	struct envsys_basic_info sc_info[ACPIBAT_NSENSORS];
	struct envsys_tre_data sc_data[ACPIBAT_NSENSORS];

	ACPI_OBJECT sc_Ret[BAT_WORDS];	/* Return Buffer */

	struct simplelock sc_lock;
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
#define ABAT_F_LOCKED		0x08	/* is locked? */
#define ABAT_F_DISCHARGING	0x10	/* discharging */
#define ABAT_F_CHARGING		0x20	/* charging */
#define ABAT_F_CRITICAL		0x40	/* charging */

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

#define ABAT_ASSERT_LOCKED(sc)					\
do {								\
	if (!((sc)->sc_flags & ABAT_F_LOCKED))			\
		panic("acpi_bat (expected to be locked)");	\
} while(/*CONSTCOND*/0)
#define ABAT_ASSERT_UNLOCKED(sc)				\
do {								\
	if (((sc)->sc_flags & ABAT_F_LOCKED))			\
		panic("acpi_bat (expected to be unlocked)");	\
} while(/*CONSTCOND*/0)
#define ABAT_LOCK(sc, s)			\
do {						\
	ABAT_ASSERT_UNLOCKED(sc);		\
	(s) = splhigh();			\
	simple_lock(&(sc)->sc_lock);		\
	ABAT_SET((sc), ABAT_F_LOCKED);		\
} while(/*CONSTCOND*/0)
#define ABAT_UNLOCK(sc, s)			\
do {						\
	ABAT_ASSERT_LOCKED(sc);			\
	ABAT_CLEAR((sc), ABAT_F_LOCKED);	\
	simple_unlock(&(sc)->sc_lock);		\
	splx((s));				\
} while(/*CONSTCOND*/0)

int	acpibat_match(struct device *, struct cfdata *, void *);
void	acpibat_attach(struct device *, struct device *, void *);

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
static void acpibat_notify_handler(ACPI_HANDLE, UINT32, void *context);
static int acpibat_gtredata(struct sysmon_envsys *, struct envsys_tre_data *);
static int acpibat_streinfo(struct sysmon_envsys *, struct envsys_basic_info *);

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
	simple_lock_init(&sc->sc_lock);

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

#ifdef ACPI_BAT_DEBUG
	ABAT_SET(sc, ABAT_F_VERBOSE);
#endif

	acpibat_init_envsys(sc);
}

/*
 * clear informations
 */

void
acpibat_clear_presence(struct acpibat_softc *sc)
{

	ABAT_ASSERT_LOCKED(sc);

	acpibat_clear_info(sc);
	sc->sc_available = ABAT_ALV_NONE;
	ABAT_CLEAR(sc, ABAT_F_PRESENT);
}

void
acpibat_clear_info(struct acpibat_softc *sc)
{

	ABAT_ASSERT_LOCKED(sc);

	acpibat_clear_stat(sc);
	sc->sc_available = ABAT_ALV_PRESENCE;
	sc->sc_data[ACPIBAT_DCAPACITY].cur.data_s = 0;
	sc->sc_data[ACPIBAT_LFCCAPACITY].cur.data_s = 0;
	sc->sc_data[ACPIBAT_LFCCAPACITY].max.data_s = 0;
	sc->sc_data[ACPIBAT_CAPACITY].max.data_s = 0;
	sc->sc_data[ACPIBAT_TECHNOLOGY].cur.data_s = 0;
	sc->sc_data[ACPIBAT_DVOLTAGE].cur.data_s = 0;
	sc->sc_data[ACPIBAT_WCAPACITY].cur.data_s = 0;
	sc->sc_data[ACPIBAT_LCAPACITY].cur.data_s = 0;
}

void
acpibat_clear_stat(struct acpibat_softc *sc)
{

	ABAT_ASSERT_LOCKED(sc);

	sc->sc_available = ABAT_ALV_INFO;
	sc->sc_data[ACPIBAT_LOAD].cur.data_s = 0;
	sc->sc_data[ACPIBAT_CAPACITY].cur.data_s = 0;
	sc->sc_data[ACPIBAT_VOLTAGE].cur.data_s = 0;
	sc->sc_data[ACPIBAT_CAPACITY].warnflags = 0;
	sc->sc_data[ACPIBAT_DISCHARGING].cur.data_s = 0;
	sc->sc_data[ACPIBAT_CHARGING].cur.data_s = 0;
}


/*
 * returns 0 for no battery, 1 for present, and -1 on error
 */
int
acpibat_battery_present(struct acpibat_softc *sc)
{
	u_int32_t sta;
	int s;
	ACPI_OBJECT *p1;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	buf.Pointer = sc->sc_Ret;
	buf.Length = sizeof(sc->sc_Ret);

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "_STA", NULL, &buf);
	if (rv != AE_OK) {
		printf("%s: failed to evaluate _STA: %x\n",
		       sc->sc_dev.dv_xname, rv);
		return (-1);
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_INTEGER) {
		printf("%s: expected INTEGER, got %d\n", sc->sc_dev.dv_xname,
		       p1->Type);
		return (-1);
	}
	if (p1->Package.Count < 1) {
		printf("%s: expected 1 elts, got %d\n",
		       sc->sc_dev.dv_xname, p1->Package.Count);
		return (-1);
	}
	sta = p1->Integer.Value;

	ABAT_LOCK(sc, s);
	sc->sc_available = ABAT_ALV_PRESENCE;
	if (sta & ACPIBAT_STA_PRESENT) {
		ABAT_SET(sc, ABAT_F_PRESENT);
		sc->sc_data[ACPIBAT_PRESENT].cur.data_s = 1;
	} else
		sc->sc_data[ACPIBAT_PRESENT].cur.data_s = 0;
	ABAT_UNLOCK(sc, s);

	return ((sta & ACPIBAT_STA_PRESENT)?1:0);
}

/*
 * acpibat_get_info
 *
 * 	Get, and possibly display, the battery info.
 */

ACPI_STATUS
acpibat_get_info(struct acpibat_softc *sc)
{
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	int capunit, rateunit, s;
	const char *capstring, *ratestring;

	rv = acpi_eval_struct(sc->sc_node->ad_handle, "_BIF", &buf);
	if (rv != AE_OK) {
		printf("%s: failed to evaluate _BIF: 0x%x\n",
		    sc->sc_dev.dv_xname, rv);
		return (rv);
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

#define INITDATA(index, unit, string) \
	sc->sc_data[index].units = unit;     				\
	sc->sc_info[index].units = unit;     				\
	snprintf(sc->sc_info[index].desc, sizeof(sc->sc_info->desc),	\
	    "%s %s", sc->sc_dev.dv_xname, string);			\

	ABAT_LOCK(sc, s);
	p2 = p1->Package.Elements;
	/*
	 * XXX: It seems that the below attributes should not be overriden
	 * in such manner... we should unregister them for a while, maybe.
	 */
	if ((p2[0].Integer.Value & ACPIBAT_PWRUNIT_MA) != 0) {
		ABAT_SET(sc, ABAT_F_PWRUNIT_MA);
		sc->sc_sysmon.sme_ranges = acpibat_range_amp;
		capunit = ENVSYS_SAMPHOUR;
		capstring = "charge";
		rateunit = ENVSYS_SAMPS;
		ratestring = "current";
	} else {
		ABAT_CLEAR(sc, ABAT_F_PWRUNIT_MA);
		sc->sc_sysmon.sme_ranges = acpibat_range_watt;
		capunit = ENVSYS_SWATTHOUR;
		capstring = "energy";
		rateunit = ENVSYS_SWATTS;
		ratestring = "power";
	}
	INITDATA(ACPIBAT_DCAPACITY, capunit, "design cap");
	INITDATA(ACPIBAT_LFCCAPACITY, capunit, "lfc cap");
	INITDATA(ACPIBAT_WCAPACITY, capunit, "warn cap");
	INITDATA(ACPIBAT_LCAPACITY, capunit, "low cap");
	INITDATA(ACPIBAT_LOAD, rateunit, ratestring);
	INITDATA(ACPIBAT_CAPACITY, capunit, capstring);

	sc->sc_data[ACPIBAT_DCAPACITY].cur.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LFCCAPACITY].cur.data_s = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LFCCAPACITY].max.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_CAPACITY].max.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_TECHNOLOGY].cur.data_s = p2[3].Integer.Value;
	sc->sc_data[ACPIBAT_DVOLTAGE].cur.data_s = p2[4].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_WCAPACITY].cur.data_s = p2[5].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_LCAPACITY].cur.data_s = p2[6].Integer.Value * 1000;
	sc->sc_available = ABAT_ALV_INFO;
	ABAT_UNLOCK(sc, s);

	if ((sc->sc_flags & ABAT_F_VERBOSE))
		printf("%s: %s %s %s %s\n",
		       sc->sc_dev.dv_xname,
		       p2[12].String.Pointer, p2[11].String.Pointer,
		       p2[9].String.Pointer, p2[10].String.Pointer);

	rv = AE_OK;

out:
	AcpiOsFree(buf.Pointer);
	return (rv);
}

/*
 * acpibat_get_status:
 *
 *	Get, and possibly display, the current battery line status.
 */
ACPI_STATUS
acpibat_get_status(struct acpibat_softc *sc)
{
	int flags, status, s;
	ACPI_OBJECT *p1, *p2;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	buf.Pointer = sc->sc_Ret;
	buf.Length = sizeof(sc->sc_Ret);

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "_BST", NULL, &buf);

	if (rv != AE_OK) {
		printf("bat: failed to evaluate _BST: 0x%x\n", rv);
		return (rv);
	}
	p1 = (ACPI_OBJECT *)buf.Pointer;

	if (p1->Type != ACPI_TYPE_PACKAGE) {
		printf("bat: expected PACKAGE, got %d\n", p1->Type);
		return (AE_ERROR);
	}
	if (p1->Package.Count < 4) {
		printf("bat: expected 4 elts, got %d\n", p1->Package.Count);
		return (AE_ERROR);
	}
	p2 = p1->Package.Elements;

	ABAT_LOCK(sc, s);
	status = p2[0].Integer.Value;
	sc->sc_data[ACPIBAT_LOAD].cur.data_s = p2[1].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_CAPACITY].cur.data_s = p2[2].Integer.Value * 1000;
	sc->sc_data[ACPIBAT_VOLTAGE].cur.data_s = p2[3].Integer.Value * 1000;

	flags = 0;
	if (sc->sc_data[ACPIBAT_CAPACITY].cur.data_s <
	    sc->sc_data[ACPIBAT_WCAPACITY].cur.data_s)
		flags |= ENVSYS_WARN_UNDER;
	if (status & ACPIBAT_ST_CRITICAL)
		flags |= ENVSYS_WARN_CRITUNDER;
	sc->sc_data[ACPIBAT_CAPACITY].warnflags = flags;
	sc->sc_data[ACPIBAT_DISCHARGING].cur.data_s =
	    ((status & ACPIBAT_ST_DISCHARGING) != 0);
	sc->sc_data[ACPIBAT_CHARGING].cur.data_s =
	    ((status & ACPIBAT_ST_CHARGING) != 0);
	sc->sc_available = ABAT_ALV_STAT;
	ABAT_UNLOCK(sc, s);

	return (AE_OK);
}

#define SCALE(x)	((x)/1000000), (((x)%1000000)/1000)
#define CAPUNITS(sc)	(ABAT_ISSET((sc), ABAT_F_PWRUNIT_MA)?"mAh":"mWh")
#define RATEUNITS(sc)	(ABAT_ISSET((sc), ABAT_F_PWRUNIT_MA)?"mA":"mW")
static void
acpibat_print_info(struct acpibat_softc *sc)
{
	const char *tech;

	if (sc->sc_data[ACPIBAT_TECHNOLOGY].cur.data_s)
		tech = "secondary";
	else
		tech = "primary";

	printf("%s: %s battery, Design %d.%03d%s, Predicted %d.%03d%s"
	       "Warn %d.%03d%s Low %d.%03d%s\n",
	       sc->sc_dev.dv_xname, tech,
	       SCALE(sc->sc_data[ACPIBAT_DCAPACITY].cur.data_s), CAPUNITS(sc),
	       SCALE(sc->sc_data[ACPIBAT_LFCCAPACITY].cur.data_s),CAPUNITS(sc),
	       SCALE(sc->sc_data[ACPIBAT_WCAPACITY].cur.data_s), CAPUNITS(sc),
	       SCALE(sc->sc_data[ACPIBAT_LCAPACITY].cur.data_s), CAPUNITS(sc));
}

static void
acpibat_print_stat(struct acpibat_softc *sc)
{
	const char *capstat, *chargestat;
	int percent;

	if (sc->sc_data[ACPIBAT_CAPACITY].warnflags&ENVSYS_WARN_CRITUNDER)
		capstat = "CRITICAL ";
	else if (sc->sc_data[ACPIBAT_CAPACITY].warnflags&ENVSYS_WARN_UNDER)
		capstat = "UNDER ";
	else
		capstat = "";
	if (sc->sc_data[ACPIBAT_CHARGING].cur.data_s)
		chargestat = "charging";
	else if (sc->sc_data[ACPIBAT_DISCHARGING].cur.data_s)
		chargestat = "discharging";
	else
		chargestat = "idling";
	if (sc->sc_data[ACPIBAT_DCAPACITY].cur.data_s>0)
		percent =
		    (sc->sc_data[ACPIBAT_CAPACITY].cur.data_s*100)/
		    sc->sc_data[ACPIBAT_DCAPACITY].cur.data_s;
	printf("%s: %s%s: %d.%03dV cap %d.%03d%s (%d%%) rate %d.%03d%s\n",
	       sc->sc_dev.dv_xname,
	       capstat, chargestat,
	       SCALE(sc->sc_data[ACPIBAT_VOLTAGE].cur.data_s),
	       SCALE(sc->sc_data[ACPIBAT_CAPACITY].cur.data_s), CAPUNITS(sc),
	       percent,
	       SCALE(sc->sc_data[ACPIBAT_LOAD].cur.data_s), RATEUNITS(sc));
}

static void
acpibat_update(void *arg)
{
	struct acpibat_softc *sc = arg;

	if (sc->sc_available < ABAT_ALV_INFO) {
		/* current information is invalid */
		if (sc->sc_available < ABAT_ALV_PRESENCE)
			/* presence is invalid */
			if (acpibat_battery_present(sc)<0)
				/* error */
				return;
		if (ABAT_ISSET(sc, ABAT_F_PRESENT)) {
			/* the battery is present. */
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				printf("%s: battery is present.\n",
				       sc->sc_dev.dv_xname);
			if (ACPI_FAILURE(acpibat_get_info(sc)))
				return;
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				acpibat_print_info(sc);
		} else {
			/* the battery is not present. */
			if (ABAT_ISSET(sc, ABAT_F_VERBOSE))
				printf("%s: battery is not present.\n",
				       sc->sc_dev.dv_xname);
			return;
		}
	} else {
		/* current information is valid */
		if (!ABAT_ISSET(sc, ABAT_F_PRESENT))
			/* the battery is not present. */
			return;
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
void
acpibat_notify_handler(ACPI_HANDLE handle, UINT32 notify, void *context)
{
	struct acpibat_softc *sc = context;
	int rv, s;

#ifdef ACPI_BAT_DEBUG
	printf("%s: received notify message: 0x%x\n",
	       sc->sc_dev.dv_xname, notify);
#endif

	switch (notify) {
	case ACPI_NOTIFY_BusCheck:
		break;

	case ACPI_NOTIFY_BatteryInformationChanged:
		ABAT_LOCK(sc, s);
		acpibat_clear_presence(sc);
		ABAT_UNLOCK(sc, s);
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
					     acpibat_update, sc);
		if (rv != AE_OK)
			printf("%s: unable to queue status check: %d\n",
			       sc->sc_dev.dv_xname, rv);
		break;

	case ACPI_NOTIFY_BatteryStatusChanged:
		ABAT_LOCK(sc, s);
		acpibat_clear_stat(sc);
		ABAT_UNLOCK(sc, s);
		rv = AcpiOsQueueForExecution(OSD_PRIORITY_LO,
					     acpibat_update, sc);
		if (rv != AE_OK)
			printf("%s: unable to queue status check: %d\n",
			       sc->sc_dev.dv_xname, rv);
		break;

	default:
		printf("%s: received unknown notify message: 0x%x\n",
		       sc->sc_dev.dv_xname, notify);
	}
}

void
acpibat_init_envsys(struct acpibat_softc *sc)
{
	int capunit, rateunit, i;
	const char *capstring, *ratestring;

#if 0
	if (sc->sc_flags & ABAT_F_PWRUNIT_MA) {
#endif
		/* XXX */
		sc->sc_sysmon.sme_ranges = acpibat_range_amp;
		capunit = ENVSYS_SAMPHOUR;
		capstring = "charge";
		rateunit = ENVSYS_SAMPS;
		ratestring = "current";
#if 0
	} else {
		sc->sc_sysmon.sme_ranges = acpibat_range_watt;
		capunit = ENVSYS_SWATTHOUR;
		capstring = "energy";
		rateunit = ENVSYS_SWATTS;
		ratestring = "power";
	}
#endif

	for (i = 0 ; i < ACPIBAT_NSENSORS; i++) {
		sc->sc_data[i].sensor = sc->sc_info[i].sensor = i;
		sc->sc_data[i].validflags |= (ENVSYS_FVALID | ENVSYS_FCURVALID);
		sc->sc_info[i].validflags = ENVSYS_FVALID;
		sc->sc_data[i].warnflags = 0;
	}
	INITDATA(ACPIBAT_PRESENT, ENVSYS_INDICATOR, "present");
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

	if (sc->sc_available < ABAT_ALV_STAT)
		acpibat_update(sc);

	/* XXX locking */
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
