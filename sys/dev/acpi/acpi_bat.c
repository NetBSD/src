/*	$NetBSD: acpi_bat.c,v 1.81 2010/01/31 06:45:09 jruoho Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: acpi_bat.c,v 1.81 2010/01/31 06:45:09 jruoho Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>		/* for hz */
#include <sys/kmem.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define _COMPONENT		 ACPI_BAT_COMPONENT
ACPI_MODULE_NAME		 ("acpi_bat")

/*
 * Sensor indexes.
 */
enum {
	ACPIBAT_PRESENT		 = 0,
	ACPIBAT_DCAPACITY	 = 1,
	ACPIBAT_LFCCAPACITY	 = 2,
	ACPIBAT_TECHNOLOGY	 = 3,
	ACPIBAT_DVOLTAGE	 = 4,
	ACPIBAT_WCAPACITY	 = 5,
	ACPIBAT_LCAPACITY	 = 6,
	ACPIBAT_VOLTAGE		 = 7,
	ACPIBAT_CHARGERATE	 = 8,
	ACPIBAT_DISCHARGERATE	 = 9,
	ACPIBAT_CAPACITY	 = 10,
	ACPIBAT_CHARGING	 = 11,
	ACPIBAT_CHARGE_STATE	 = 12,
	ACPIBAT_COUNT		 = 13
};

/*
 * Battery Information, _BIF
 * (ACPI 3.0, sec. 10.2.2.1).
 */
enum {
	ACPIBAT_BIF_UNIT	 = 0,
	ACPIBAT_BIF_DCAPACITY	 = 1,
	ACPIBAT_BIF_LFCCAPACITY	 = 2,
	ACPIBAT_BIF_TECHNOLOGY	 = 3,
	ACPIBAT_BIF_DVOLTAGE	 = 4,
	ACPIBAT_BIF_WCAPACITY	 = 5,
	ACPIBAT_BIF_LCAPACITY	 = 6,
	ACPIBAT_BIF_GRANULARITY1 = 7,
	ACPIBAT_BIF_GRANULARITY2 = 8,
	ACPIBAT_BIF_MODEL	 = 9,
	ACPIBAT_BIF_SERIAL	 = 10,
	ACPIBAT_BIF_TYPE	 = 11,
	ACPIBAT_BIF_OEM		 = 12,
	ACPIBAT_BIF_COUNT	 = 13
};

/*
 * Battery Status, _BST
 * (ACPI 3.0, sec. 10.2.2.3).
 */
enum {
	ACPIBAT_BST_STATE	 = 0,
	ACPIBAT_BST_RATE	 = 1,
	ACPIBAT_BST_CAPACITY	 = 2,
	ACPIBAT_BST_VOLTAGE	 = 3,
	ACPIBAT_BST_COUNT	 = 4
};

struct acpibat_softc {
	struct acpi_devnode	*sc_node;
	struct sysmon_envsys	*sc_sme;
	struct timeval		 sc_lastupdate;
	envsys_data_t		*sc_sensor;
	kmutex_t		 sc_mutex;
	kcondvar_t		 sc_condvar;
	int                      sc_present;
};

static const char * const bat_hid[] = {
	"PNP0C0A",
	NULL
};

#define ACPIBAT_PWRUNIT_MA	0x00000001  /* mA not mW */
#define ACPIBAT_ST_DISCHARGING	0x00000001  /* battery is discharging */
#define ACPIBAT_ST_CHARGING	0x00000002  /* battery is charging */
#define ACPIBAT_ST_CRITICAL	0x00000004  /* battery is critical */

/*
 * Flags for battery status from _STA return. Note that
 * this differs from the conventional evaluation of _STA:
 *
 *	"Unlike most other devices, when a battery is inserted or
 *	 removed from the system, the device itself (the battery bay)
 *	 is still considered to be present in the system. For most
 *	 systems, the _STA for this device will always return a value
 *	 with bits 0-3 set and will toggle bit 4 to indicate the actual
 *	 presence of a battery. (ACPI 3.0, sec. 10.2.1, p. 320.)"
 */
#define ACPIBAT_STA_PRESENT	0x00000010  /* battery present */

/*
 * A value used when _BST or _BIF is teporarily unknown (see ibid.).
 */
#define ACPIBAT_VAL_UNKNOWN	0xFFFFFFFF

#define ACPIBAT_VAL_ISVALID(x)						      \
	(((x) != ACPIBAT_VAL_UNKNOWN) ? ENVSYS_SVALID : ENVSYS_SINVALID)

static int	    acpibat_match(device_t, cfdata_t, void *);
static void	    acpibat_attach(device_t, device_t, void *);
static int	    acpibat_detach(device_t, int);
static int          acpibat_get_sta(device_t);
static ACPI_OBJECT *acpibat_get_object(ACPI_HANDLE, const char *, int);
static void         acpibat_get_info(device_t);
static void         acpibat_get_status(device_t);
static void         acpibat_update_info(void *);
static void         acpibat_update_status(void *);
static void         acpibat_init_envsys(device_t);
static void         acpibat_notify_handler(ACPI_HANDLE, UINT32, void *);
static void         acpibat_refresh(struct sysmon_envsys *, envsys_data_t *);
static bool	    acpibat_resume(device_t, pmf_qual_t);

CFATTACH_DECL_NEW(acpibat, sizeof(struct acpibat_softc),
    acpibat_match, acpibat_attach, acpibat_detach, NULL);

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

	aprint_naive(": ACPI Battery\n");
	aprint_normal(": ACPI Battery\n");

	sc->sc_node = aa->aa_node;
	sc->sc_present = 0;

	sc->sc_sme = NULL;
	sc->sc_sensor = NULL;

	mutex_init(&sc->sc_mutex, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&sc->sc_condvar, device_xname(self));

	if (pmf_device_register(self, NULL, acpibat_resume) != true)
		aprint_error_dev(self, "couldn't establish power handler\n");

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpibat_notify_handler, self);

	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "couldn't install notify handler\n");
		return;
	}

	sc->sc_sensor = kmem_zalloc(ACPIBAT_COUNT *
	    sizeof(*sc->sc_sensor), KM_SLEEP);

	if (sc->sc_sensor == NULL)
		return;

	acpibat_init_envsys(self);
}

/*
 * acpibat_detach:
 *
 *	Autoconfiguration `detach' routine.
 */
static int
acpibat_detach(device_t self, int flags)
{
	struct acpibat_softc *sc = device_private(self);
	ACPI_STATUS rv;

	rv = AcpiRemoveNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_ALL_NOTIFY, acpibat_notify_handler);

	if (ACPI_FAILURE(rv))
		return EBUSY;

	cv_destroy(&sc->sc_condvar);
	mutex_destroy(&sc->sc_mutex);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_sensor != NULL)
		kmem_free(sc->sc_sensor, ACPIBAT_COUNT *
		    sizeof(*sc->sc_sensor));

	pmf_device_deregister(self);

	return 0;
}

/*
 * acpibat_get_sta:
 *
 *	Evaluate whether the battery is present or absent.
 *
 *	Returns: 0 for no battery, 1 for present, and -1 on error.
 */
static int
acpibat_get_sta(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle, "_STA", &val);

	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(dv, "failed to evaluate _STA\n");
		return -1;
	}

	sc->sc_sensor[ACPIBAT_PRESENT].state = ENVSYS_SVALID;

	if ((val & ACPIBAT_STA_PRESENT) == 0) {
		sc->sc_sensor[ACPIBAT_PRESENT].value_cur = 0;
		return 0;
	}

	sc->sc_sensor[ACPIBAT_PRESENT].value_cur = 1;

	return 1;
}

static ACPI_OBJECT *
acpibat_get_object(ACPI_HANDLE hdl, const char *pth, int count)
{
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	rv = acpi_eval_struct(hdl, pth, &buf);

	if (ACPI_FAILURE(rv))
		return NULL;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_PACKAGE) {
		ACPI_FREE(buf.Pointer);
		return NULL;
	}

	if (obj->Package.Count != count) {
		ACPI_FREE(buf.Pointer);
		return NULL;
	}

	return obj;
}

/*
 * acpibat_get_info:
 *
 * 	Get, and possibly display, the battery info.
 */
static void
acpibat_get_info(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	int capunit, i, j, rateunit, val;
	ACPI_OBJECT *elm, *obj;
	ACPI_STATUS rv = AE_OK;

	obj = acpibat_get_object(hdl, "_BIF", ACPIBAT_BIF_COUNT);

	if (obj == NULL) {
		rv = AE_ERROR;
		goto out;
	}

	elm = obj->Package.Elements;

	for (i = ACPIBAT_BIF_UNIT; i < ACPIBAT_BIF_MODEL; i++) {

		if (elm[i].Type != ACPI_TYPE_INTEGER) {
			rv = AE_TYPE;
			goto out;
		}

		KDASSERT((uint64_t)elm[i].Integer.Value < INT_MAX);
	}

	aprint_verbose_dev(dv, "battery info: ");

	for (i = j = ACPIBAT_BIF_OEM; i > ACPIBAT_BIF_GRANULARITY2; i--) {

		if (elm[i].Type != ACPI_TYPE_STRING)
			continue;

		if (elm[i].String.Pointer == NULL)
			continue;

		aprint_verbose("%s ", elm[i].String.Pointer);

		j = 0;
	}

	if (j != 0)
		aprint_verbose("not available");

	aprint_verbose("\n");

	if ((elm[ACPIBAT_BIF_UNIT].Integer.Value & ACPIBAT_PWRUNIT_MA) != 0) {
		capunit = ENVSYS_SAMPHOUR;
		rateunit = ENVSYS_SAMPS;
	} else {
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

	/* Design capacity. */
	val = elm[ACPIBAT_BIF_DCAPACITY].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_DCAPACITY].value_cur = val;
	sc->sc_sensor[ACPIBAT_DCAPACITY].state = ACPIBAT_VAL_ISVALID(val);

	/* Last full charge capacity. */
	val = elm[ACPIBAT_BIF_LFCCAPACITY].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].value_cur = val;
	sc->sc_sensor[ACPIBAT_LFCCAPACITY].state = ACPIBAT_VAL_ISVALID(val);

	/* Battery technology. */
	val = elm[ACPIBAT_BIF_TECHNOLOGY].Integer.Value;
	sc->sc_sensor[ACPIBAT_TECHNOLOGY].value_cur = val;
	sc->sc_sensor[ACPIBAT_TECHNOLOGY].state = ACPIBAT_VAL_ISVALID(val);

	/* Design voltage. */
	val = elm[ACPIBAT_BIF_DVOLTAGE].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_DVOLTAGE].value_cur = val;
	sc->sc_sensor[ACPIBAT_DVOLTAGE].state = ACPIBAT_VAL_ISVALID(val);

	/* Design warning capacity. */
	val = elm[ACPIBAT_BIF_WCAPACITY].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_WCAPACITY].value_cur = val;
	sc->sc_sensor[ACPIBAT_WCAPACITY].state = ACPIBAT_VAL_ISVALID(val);
	sc->sc_sensor[ACPIBAT_WCAPACITY].flags |=
	    ENVSYS_FPERCENT | ENVSYS_FVALID_MAX;

	/* Design low capacity. */
	val = elm[ACPIBAT_BIF_LCAPACITY].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_LCAPACITY].value_cur = val;
	sc->sc_sensor[ACPIBAT_LCAPACITY].state = ACPIBAT_VAL_ISVALID(val);
	sc->sc_sensor[ACPIBAT_LCAPACITY].flags |=
	    ENVSYS_FPERCENT | ENVSYS_FVALID_MAX;

	/*
	 * Initialize the maximum of current, warning, and
	 * low capacity to the last full charge capacity.
	 */
	val = sc->sc_sensor[ACPIBAT_LFCCAPACITY].value_cur;

	sc->sc_sensor[ACPIBAT_CAPACITY].value_max = val;
	sc->sc_sensor[ACPIBAT_WCAPACITY].value_max = val;
	sc->sc_sensor[ACPIBAT_LCAPACITY].value_max = val;

out:
	if (obj != NULL)
		ACPI_FREE(obj);

	if (ACPI_FAILURE(rv))
		aprint_error_dev(dv, "failed to evaluate _BIF: %s\n",
		    AcpiFormatException(rv));
}

/*
 * acpibat_get_status:
 *
 *	Get, and possibly display, the current battery line status.
 */
static void
acpibat_get_status(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	int i, rate, state, val;
	ACPI_OBJECT *elm, *obj;
	ACPI_STATUS rv = AE_OK;

	obj = acpibat_get_object(hdl, "_BST", ACPIBAT_BST_COUNT);

	if (obj == NULL) {
		rv = AE_ERROR;
		goto out;
	}

	elm = obj->Package.Elements;

	for (i = ACPIBAT_BST_STATE; i < ACPIBAT_BST_COUNT; i++) {

		if (elm[i].Type != ACPI_TYPE_INTEGER) {
			rv = AE_TYPE;
			goto out;
		}
	}

	state = elm[ACPIBAT_BST_STATE].Integer.Value;

	if ((state & ACPIBAT_ST_CHARGING) != 0) {
		/* XXX rate can be invalid */
		rate = elm[ACPIBAT_BST_RATE].Integer.Value;
		sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGERATE].value_cur = rate * 1000;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].value_cur = 1;
	} else if ((state & ACPIBAT_ST_DISCHARGING) != 0) {
		rate = elm[ACPIBAT_BST_RATE].Integer.Value;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].value_cur = rate * 1000;
		sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].value_cur = 0;
	} else {
		sc->sc_sensor[ACPIBAT_CHARGING].state = ENVSYS_SVALID;
		sc->sc_sensor[ACPIBAT_CHARGING].value_cur = 0;
		sc->sc_sensor[ACPIBAT_CHARGERATE].state = ENVSYS_SINVALID;
		sc->sc_sensor[ACPIBAT_DISCHARGERATE].state = ENVSYS_SINVALID;
	}

	/* Remaining capacity. */
	val = elm[ACPIBAT_BST_CAPACITY].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_CAPACITY].value_cur = val;
	sc->sc_sensor[ACPIBAT_CAPACITY].state = ACPIBAT_VAL_ISVALID(val);
	sc->sc_sensor[ACPIBAT_CAPACITY].flags |=
	    ENVSYS_FPERCENT | ENVSYS_FVALID_MAX;

	/* Battery voltage. */
	val = elm[ACPIBAT_BST_VOLTAGE].Integer.Value * 1000;
	sc->sc_sensor[ACPIBAT_VOLTAGE].value_cur = val;
	sc->sc_sensor[ACPIBAT_VOLTAGE].state = ACPIBAT_VAL_ISVALID(val);

	sc->sc_sensor[ACPIBAT_CHARGE_STATE].state = ENVSYS_SVALID;
	sc->sc_sensor[ACPIBAT_CHARGE_STATE].value_cur =
	    ENVSYS_BATTERY_CAPACITY_NORMAL;

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

	if ((state & ACPIBAT_ST_CRITICAL) != 0) {
		sc->sc_sensor[ACPIBAT_CAPACITY].state = ENVSYS_SCRITICAL;
		sc->sc_sensor[ACPIBAT_CHARGE_STATE].value_cur =
		    ENVSYS_BATTERY_CAPACITY_CRITICAL;
	}

out:
	if (obj != NULL)
		ACPI_FREE(obj);

	if (ACPI_FAILURE(rv))
		aprint_error_dev(dv, "failed to evaluate _BST: %s\n",
		    AcpiFormatException(rv));
}

static void
acpibat_update_info(void *arg)
{
	device_t dv = arg;
	struct acpibat_softc *sc = device_private(dv);
	int i, rv;

	mutex_enter(&sc->sc_mutex);

	rv = acpibat_get_sta(dv);

	if (rv > 0)
		acpibat_get_info(dv);
	else {
		i = (rv < 0) ? 0 : ACPIBAT_DCAPACITY;

		while (i < ACPIBAT_COUNT) {
			sc->sc_sensor[i].state = ENVSYS_SINVALID;
			i++;
		}
	}

	sc->sc_present = rv;

	mutex_exit(&sc->sc_mutex);
}

static void
acpibat_update_status(void *arg)
{
	device_t dv = arg;
	struct acpibat_softc *sc = device_private(dv);
	int i, rv;

	mutex_enter(&sc->sc_mutex);

	rv = acpibat_get_sta(dv);

	if (rv > 0) {

		if (sc->sc_present == 0)
			acpibat_get_info(dv);

		acpibat_get_status(dv);
	} else {
		i = (rv < 0) ? 0 : ACPIBAT_DCAPACITY;

		while (i < ACPIBAT_COUNT) {
			sc->sc_sensor[i].state = ENVSYS_SINVALID;
			i++;
		}
	}

	sc->sc_present = rv;

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
	static const int handler = OSL_NOTIFY_HANDLER;
	device_t dv = context;

	switch (notify) {

	case ACPI_NOTIFY_BusCheck:
		break;

	case ACPI_NOTIFY_DeviceCheck:
	case ACPI_NOTIFY_BatteryInformationChanged:
		(void)AcpiOsExecute(handler, acpibat_update_info, dv);
		break;

	case ACPI_NOTIFY_BatteryStatusChanged:
		(void)AcpiOsExecute(handler, acpibat_update_status, dv);
		break;

	default:
		aprint_error_dev(dv, "unknown notify: 0x%02X\n", notify);
	}
}

static void
acpibat_init_envsys(device_t dv)
{
	struct acpibat_softc *sc = device_private(dv);
	int i;

#define INITDATA(index, unit, string)					\
	do {								\
		sc->sc_sensor[index].state = ENVSYS_SVALID;		\
		sc->sc_sensor[index].units = unit;			\
		(void)strlcpy(sc->sc_sensor[index].desc, string,	\
		    sizeof(sc->sc_sensor[index].desc));			\
	} while (/* CONSTCOND */ 0)

	INITDATA(ACPIBAT_PRESENT, ENVSYS_INDICATOR, "present");
	INITDATA(ACPIBAT_DCAPACITY, ENVSYS_SWATTHOUR, "design cap");
	INITDATA(ACPIBAT_LFCCAPACITY, ENVSYS_SWATTHOUR, "last full cap");
	INITDATA(ACPIBAT_TECHNOLOGY, ENVSYS_INTEGER, "technology");
	INITDATA(ACPIBAT_DVOLTAGE, ENVSYS_SVOLTS_DC, "design voltage");
	INITDATA(ACPIBAT_WCAPACITY, ENVSYS_SWATTHOUR, "warn cap");
	INITDATA(ACPIBAT_LCAPACITY, ENVSYS_SWATTHOUR, "low cap");
	INITDATA(ACPIBAT_VOLTAGE, ENVSYS_SVOLTS_DC, "voltage");
	INITDATA(ACPIBAT_CHARGERATE, ENVSYS_SWATTS, "charge rate");
	INITDATA(ACPIBAT_DISCHARGERATE, ENVSYS_SWATTS, "discharge rate");
	INITDATA(ACPIBAT_CAPACITY, ENVSYS_SWATTHOUR, "charge");
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

	for (i = 0; i < ACPIBAT_COUNT; i++) {

		if (sysmon_envsys_sensor_attach(sc->sc_sme,
			&sc->sc_sensor[i]))
			goto fail;
	}

	sc->sc_sme->sme_name = device_xname(dv);
	sc->sc_sme->sme_cookie = dv;
	sc->sc_sme->sme_refresh = acpibat_refresh;
	sc->sc_sme->sme_class = SME_CLASS_BATTERY;
	sc->sc_sme->sme_flags = SME_POLL_ONLY;

	acpibat_update_info(dv);
	acpibat_update_status(dv);

	if (sysmon_envsys_register(sc->sc_sme))
		goto fail;

	return;

fail:
	aprint_error_dev(dv, "failed to initialize sysmon\n");

	sysmon_envsys_destroy(sc->sc_sme);
	kmem_free(sc->sc_sensor, ACPIBAT_COUNT * sizeof(*sc->sc_sensor));

	sc->sc_sme = NULL;
	sc->sc_sensor = NULL;
}

static void
acpibat_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	device_t dv = sme->sme_cookie;
	struct acpibat_softc *sc = device_private(dv);
	struct timeval tv, tmp;
	ACPI_STATUS rv;

	tmp.tv_sec = 5;
	tmp.tv_usec = 0;
	microtime(&tv);
	timersub(&tv, &tmp, &tv);

	if (timercmp(&tv, &sc->sc_lastupdate, <))
		return;

	if (!mutex_tryenter(&sc->sc_mutex))
		return;

	rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_status, dv);

	if (ACPI_SUCCESS(rv))
		cv_timedwait(&sc->sc_condvar, &sc->sc_mutex, hz);

	mutex_exit(&sc->sc_mutex);
}

static bool
acpibat_resume(device_t dv, pmf_qual_t qual)
{

	(void)AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_info, dv);
	(void)AcpiOsExecute(OSL_NOTIFY_HANDLER, acpibat_update_status, dv);

	return true;
}
