/*	$NetBSD: wmi_hp.c,v 1.3 2010/10/24 16:25:31 jmcneill Exp $ */

/*-
 * Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jukka Ruohonen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*-
 * Copyright (c) 2009 Michael Gmelin <freebsd@grem.de>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wmi_hp.c,v 1.3 2010/10/24 16:25:31 jmcneill Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/module.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/wmi/wmi_acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#define _COMPONENT			ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME			("wmi_hp")

#define WMI_HP_METHOD_ARG_READ		0x01
#define WMI_HP_METHOD_ARG_WRITE		0x02
#define WMI_HP_METHOD_ARG_WRITE_SIZE	0x04
#define WMI_HP_METHOD_ARG_MAGIC		0x55434553
#define WMI_HP_METHOD_ARG_SIZE		0x05 * sizeof(uint32_t)

#define WMI_HP_METHOD_CMD_DISPLAY	0x01
#define WMI_HP_METHOD_CMD_HDDTEMP	0x02
#define WMI_HP_METHOD_CMD_ALS		0x03
#define WMI_HP_METHOD_CMD_DOCK		0x04
#define WMI_HP_METHOD_CMD_SWITCH	0x05
#define WMI_HP_METHOD_CMD_HOTKEY	0x0C

#define WMI_HP_EVENT_DOCK		0x01
#define WMI_HP_EVENT_HOTKEY		0x04
#define WMI_HP_EVENT_SWITCH		0x05
/*      WMI_HP_EVENT_UNKNOWN		0xXX */

#define WMI_HP_HOTKEY_BRIGHTNESS_UP	0x02
#define WMI_HP_HOTKEY_BRIGHTNESS_DOWN	0x03
/*      WMI_HP_HOTKEY_UNKNOWN		0xXX */

#define WMI_HP_SWITCH_WLAN		0x01
#define WMI_HP_SWITCH_BT		0x02
#define WMI_HP_SWITCH_WWAN		0x04

#define WMI_HP_SWITCH_ARG_WLAN_OFF	0x100
#define WMI_HP_SWITCH_ARG_WLAN_ON	0x101
#define WMI_HP_SWITCH_ARG_BT_OFF	0x200
#define WMI_HP_SWITCH_ARG_BT_ON		0x202
#define WMI_HP_SWITCH_ARG_WWAN_OFF	0x400
#define WMI_HP_SWITCH_ARG_WWAN_ON	0x404

#define WMI_HP_SWITCH_MASK_WLAN_ONAIR	__BIT(8)
#define WMI_HP_SWITCH_MASK_WLAN_ENABLED	__BIT(9)
#define WMI_HP_SWITCH_MASK_WLAN_RADIO	__BIT(11)
#define WMI_HP_SWITCH_MASK_BT_ONAIR	__BIT(16)
#define WMI_HP_SWITCH_MASK_BT_ENABLED	__BIT(17)
#define WMI_HP_SWITCH_MASK_BT_RADIO	__BIT(19)
#define WMI_HP_SWITCH_MASK_WWAN_ONAIR	__BIT(24)
#define WMI_HP_SWITCH_MASK_WWAN_ENABLED	__BIT(25)
#define WMI_HP_SWITCH_MASK_WWAN_RADIO	__BIT(27)

#define WMI_HP_GUID_EVENT		"95F24279-4D7B-4334-9387-ACCDC67EF61C"
#define WMI_HP_GUID_METHOD		"5FB7F034-2C63-45E9-BE91-3D44E2C707E4"

#define WMI_HP_SENSOR_WLAN		0
#define WMI_HP_SENSOR_BT		1
#define WMI_HP_SENSOR_WWAN		2
#define WMI_HP_SENSOR_COUNT		3
#define WMI_HP_SENSOR_SIZE		3 * sizeof(envsys_data_t)

struct wmi_hp_softc {
	device_t		sc_dev;
	device_t		sc_parent;
	struct sysmon_envsys   *sc_sme;
	envsys_data_t	       *sc_sensor;
	uint32_t	       *sc_arg;
	uint32_t		sc_val;
};

static int	wmi_hp_match(device_t, cfdata_t, void *);
static void	wmi_hp_attach(device_t, device_t, void *);
static int	wmi_hp_detach(device_t, int);
static bool	wmi_hp_suspend(device_t, const pmf_qual_t *);
static bool	wmi_hp_resume(device_t, const pmf_qual_t *);
static void	wmi_hp_notify_handler(ACPI_HANDLE, uint32_t, void *);
static void	wmi_hp_hotkey(void *);
static bool	wmi_hp_method(struct wmi_hp_softc *);
static bool	wmi_hp_method_read(struct wmi_hp_softc *, uint8_t);

#if 0
static bool	wmi_hp_method_write(struct wmi_hp_softc *, uint8_t, uint32_t);
#endif

static void	wmi_hp_sensor_init(struct wmi_hp_softc *);
static void	wmi_hp_sensor_update(void *);

CFATTACH_DECL_NEW(wmihp, sizeof(struct wmi_hp_softc),
    wmi_hp_match, wmi_hp_attach, wmi_hp_detach, NULL);

static int
wmi_hp_match(device_t parent, cfdata_t match, void *aux)
{
	return acpi_wmi_guid_match(parent, WMI_HP_GUID_METHOD);
}

static void
wmi_hp_attach(device_t parent, device_t self, void *aux)
{
	struct wmi_hp_softc *sc = device_private(self);
	ACPI_STATUS rv = AE_ERROR;

	sc->sc_dev = self;
	sc->sc_parent = parent;

	sc->sc_sme = NULL;
	sc->sc_sensor = NULL;

	sc->sc_arg = kmem_alloc(WMI_HP_METHOD_ARG_SIZE, KM_SLEEP);

	if (sc->sc_arg == NULL)
		return;

	aprint_naive("\n");
	aprint_normal(": HP WMI mappings\n");

	(void)pmf_device_register(sc->sc_dev, wmi_hp_suspend, wmi_hp_resume);

	if (acpi_wmi_guid_match(parent, WMI_HP_GUID_EVENT) != 0)
		rv = acpi_wmi_event_register(parent, wmi_hp_notify_handler);

	if (ACPI_FAILURE(rv))
		return;

	sc->sc_sensor = kmem_alloc(WMI_HP_SENSOR_SIZE, KM_SLEEP);

	if (sc->sc_sensor == NULL)
		return;

	wmi_hp_sensor_init(sc);
}

static int
wmi_hp_detach(device_t self, int flags)
{
	struct wmi_hp_softc *sc = device_private(self);
	device_t parent = sc->sc_parent;

	(void)acpi_wmi_event_deregister(parent);

	if (sc->sc_sme != NULL)
		sysmon_envsys_unregister(sc->sc_sme);

	if (sc->sc_sensor != NULL)
		kmem_free(sc->sc_sensor, WMI_HP_SENSOR_SIZE);

	if (sc->sc_arg != NULL)
		kmem_free(sc->sc_arg, WMI_HP_METHOD_ARG_SIZE);

	pmf_device_deregister(self);

	return 0;
}

static bool
wmi_hp_suspend(device_t self, const pmf_qual_t *qual)
{
	struct wmi_hp_softc *sc = device_private(self);
	device_t parent = sc->sc_parent;

	if (sc->sc_sensor != NULL)
		(void)acpi_wmi_event_deregister(parent);

	return true;
}

static bool
wmi_hp_resume(device_t self, const pmf_qual_t *qual)
{
	struct wmi_hp_softc *sc = device_private(self);
	device_t parent = sc->sc_parent;

	if (sc->sc_sensor != NULL)
		(void)acpi_wmi_event_register(parent, wmi_hp_notify_handler);

	return true;
}

static void
wmi_hp_notify_handler(ACPI_HANDLE hdl, uint32_t evt, void *aux)
{
	static const int handler = OSL_NOTIFY_HANDLER;
	struct wmi_hp_softc *sc;
	device_t self = aux;
	ACPI_OBJECT *obj;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint32_t val;

	buf.Pointer = NULL;

	sc = device_private(self);
	rv = acpi_wmi_event_get(sc->sc_parent, evt, &buf);

	if (ACPI_FAILURE(rv))
		goto out;

	obj = buf.Pointer;

	if (obj->Type != ACPI_TYPE_BUFFER) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->Buffer.Length != 8) {
		rv = AE_LIMIT;
		goto out;
	}

	val = *((uint8_t *)obj->Buffer.Pointer);

	if (val == 0x00) {
		rv = AE_BAD_DATA;
		goto out;
	}

	switch (val) {

	case WMI_HP_EVENT_SWITCH:
		rv = AcpiOsExecute(handler, wmi_hp_sensor_update, self);
		break;

	case WMI_HP_EVENT_HOTKEY:
		rv = AcpiOsExecute(handler, wmi_hp_hotkey, self);
		break;

	case WMI_HP_EVENT_DOCK:	/* FALLTHROUGH */

	default:
		aprint_debug_dev(sc->sc_dev, "unknown event 0x%02X\n", evt);
		break;
	}

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	if (ACPI_FAILURE(rv))
		aprint_error_dev(sc->sc_dev, "failed to get data for "
		    "event 0x%02X: %s\n", evt, AcpiFormatException(rv));
}

static void
wmi_hp_hotkey(void *aux)
{
	struct wmi_hp_softc *sc;
	device_t self = aux;

	sc = device_private(self);

	if (wmi_hp_method_read(sc, WMI_HP_METHOD_CMD_HOTKEY) != true)
		return;

	switch (sc->sc_val) {

	case WMI_HP_HOTKEY_BRIGHTNESS_UP:
		pmf_event_inject(NULL, PMFE_DISPLAY_BRIGHTNESS_UP);
		break;

	case WMI_HP_HOTKEY_BRIGHTNESS_DOWN:
		pmf_event_inject(NULL, PMFE_DISPLAY_BRIGHTNESS_DOWN);
		break;

	default:
		aprint_debug_dev(self, "unknown hotkey 0x%02x\n", sc->sc_val);
		break;
	}
}

static bool
wmi_hp_method(struct wmi_hp_softc *sc)
{
	ACPI_BUFFER ibuf, obuf;
	ACPI_STATUS rv = AE_OK;
	ACPI_OBJECT *obj;
	uint32_t cmd, *val;

	cmd = sc->sc_arg[2];

	KDASSERT(cmd != 0);
	KDASSERT(sc->sc_arg[0] == WMI_HP_METHOD_ARG_MAGIC);

	obuf.Pointer = NULL;
	ibuf.Pointer = sc->sc_arg;
	ibuf.Length = WMI_HP_METHOD_ARG_SIZE;

	rv = acpi_wmi_method(sc->sc_parent,
	    WMI_HP_GUID_METHOD, 0, 3, &ibuf, &obuf);

	if (ACPI_FAILURE(rv))
		goto out;

	obj = obuf.Pointer;

	if (obj->Type != ACPI_TYPE_BUFFER) {
		rv = AE_TYPE;
		goto out;
	}

	/*
	 *	val[0]	unknown
	 *	val[1]	error code
	 *	val[2]	return value
	 */
	val = (uint32_t *)obj->Buffer.Pointer;

	if (val[1] != 0) {
		rv = AE_ERROR;
		goto out;
	}

	sc->sc_val = val[2];

out:
	if (obuf.Pointer != NULL)
		ACPI_FREE(obuf.Pointer);

	if (ACPI_FAILURE(rv)) {
		aprint_debug_dev(sc->sc_dev, "failed to evaluate method "
		    "(cmd = 0x%02X): %s\n", cmd, AcpiFormatException(rv));
		return false;
	}

	return true;
}

static bool
wmi_hp_method_read(struct wmi_hp_softc *sc, uint8_t cmd)
{

	sc->sc_arg[0] = WMI_HP_METHOD_ARG_MAGIC;
	sc->sc_arg[1] = WMI_HP_METHOD_ARG_READ;
	sc->sc_arg[2] = cmd;
	sc->sc_arg[3] = 0;
	sc->sc_arg[4] = 0;

	return wmi_hp_method(sc);
}

#if 0
static bool
wmi_hp_method_write(struct wmi_hp_softc *sc, uint8_t cmd, uint32_t val)
{

	sc->sc_arg[0] = WMI_HP_METHOD_ARG_MAGIC;
	sc->sc_arg[1] = WMI_HP_METHOD_ARG_WRITE;
	sc->sc_arg[2] = cmd;
	sc->sc_arg[3] = WMI_HP_METHOD_ARG_WRITE_SIZE;
	sc->sc_arg[4] = val;

	return wmi_hp_method(sc);
}
#endif

static void
wmi_hp_sensor_init(struct wmi_hp_softc *sc)
{
	int i, j, sensor[3];

	const char desc[][ENVSYS_DESCLEN] = {
		"wireless", "bluetooth", "mobile"
	};

	KDASSERT(sc->sc_sme == NULL);
	KDASSERT(sc->sc_sensor != NULL);

	(void)memset(sc->sc_sensor, 0, WMI_HP_SENSOR_SIZE);

	if (wmi_hp_method_read(sc, WMI_HP_METHOD_CMD_SWITCH) != true)
		return;

	sc->sc_sme = sysmon_envsys_create();

	sensor[0] = WMI_HP_SWITCH_WLAN;
	sensor[1] = WMI_HP_SWITCH_BT;
	sensor[2] = WMI_HP_SWITCH_WWAN;

	CTASSERT(WMI_HP_SENSOR_WLAN == 0);
	CTASSERT(WMI_HP_SENSOR_BT   == 1);
	CTASSERT(WMI_HP_SENSOR_WWAN == 2);

	for (i = j = 0; i < 3; i++) {

		if ((sc->sc_val & sensor[i]) == 0)
			continue;

		(void)strlcpy(sc->sc_sensor[i].desc, desc[i], ENVSYS_DESCLEN);

		sc->sc_sensor[i].state = ENVSYS_SINVALID;
		sc->sc_sensor[i].units = ENVSYS_INDICATOR;

		if (sysmon_envsys_sensor_attach(sc->sc_sme,
			&sc->sc_sensor[i]) != 0)
			goto fail;

		j++;
	}

	if (j == 0)
		goto fail;

	sc->sc_sme->sme_flags = SME_DISABLE_REFRESH;
	sc->sc_sme->sme_name = device_xname(sc->sc_dev);

	if (sysmon_envsys_register(sc->sc_sme) != 0)
		goto fail;

	wmi_hp_sensor_update(sc->sc_dev);

	return;

fail:
	aprint_debug_dev(sc->sc_dev, "failed to initialize sysmon\n");

	sysmon_envsys_destroy(sc->sc_sme);
	kmem_free(sc->sc_sensor, WMI_HP_SENSOR_SIZE);

	sc->sc_sme = NULL;
	sc->sc_sensor = NULL;
}

static void
wmi_hp_sensor_update(void *aux)
{
	struct wmi_hp_softc *sc;
	device_t self = aux;

	sc = device_private(self);

	if (sc->sc_sme == NULL || sc->sc_sensor == NULL)
		return;

	if (wmi_hp_method_read(sc, WMI_HP_METHOD_CMD_SWITCH) != true) {
		sc->sc_sensor[WMI_HP_SENSOR_WLAN].state = ENVSYS_SINVALID;
		sc->sc_sensor[WMI_HP_SENSOR_WWAN].state = ENVSYS_SINVALID;
		sc->sc_sensor[WMI_HP_SENSOR_BT].state = ENVSYS_SINVALID;
		return;
	}

	if ((sc->sc_val & WMI_HP_SWITCH_WLAN) != 0) {
		sc->sc_sensor[WMI_HP_SENSOR_WLAN].value_cur = 0;

		if ((sc->sc_val & WMI_HP_SWITCH_MASK_WLAN_ONAIR) != 0)
			sc->sc_sensor[WMI_HP_SENSOR_WLAN].value_cur = 1;

		sc->sc_sensor[WMI_HP_SENSOR_WLAN].state = ENVSYS_SVALID;
	}

	if ((sc->sc_val & WMI_HP_SWITCH_BT) != 0) {
		sc->sc_sensor[WMI_HP_SENSOR_BT].value_cur = 0;

		if ((sc->sc_val & WMI_HP_SWITCH_MASK_BT_ONAIR) != 0)
			sc->sc_sensor[WMI_HP_SENSOR_BT].value_cur = 1;

		sc->sc_sensor[WMI_HP_SENSOR_BT].state = ENVSYS_SVALID;
	}

	if ((sc->sc_val & WMI_HP_SWITCH_WWAN) != 0) {
		sc->sc_sensor[WMI_HP_SENSOR_WWAN].value_cur = 0;

		if ((sc->sc_val & WMI_HP_SWITCH_MASK_WWAN_ONAIR) != 0)
			sc->sc_sensor[WMI_HP_SENSOR_WWAN].value_cur = 1;

		sc->sc_sensor[WMI_HP_SENSOR_WWAN].state = ENVSYS_SVALID;
	}
}

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, wmihp, NULL);
CFDRIVER_DECL(wmihp, DV_DULL, NULL);

static int wmihploc[] = { -1 };
extern struct cfattach wmihp_ca;

static struct cfparent wmiparent = {
	"acpiwmibus", NULL, DVUNIT_ANY
};

static struct cfdata wmihp_cfdata[] = {
	{
		.cf_name = "wmihp",
		.cf_atname = "wmihp",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = wmihploc,
		.cf_flags = 0,
		.cf_pspec = &wmiparent,
	},

	{ NULL }
};

static int
wmihp_modcmd(modcmd_t cmd, void *opaque)
{
	int err;

	switch (cmd) {

	case MODULE_CMD_INIT:

		err = config_cfdriver_attach(&wmihp_cd);

		if (err != 0)
			return err;

		err = config_cfattach_attach("wmihp", &wmihp_ca);

		if (err != 0) {
			config_cfdriver_detach(&wmihp_cd);
			return err;
		}

		err = config_cfdata_attach(wmihp_cfdata, 1);

		if (err != 0) {
			config_cfattach_detach("wmihp", &wmihp_ca);
			config_cfdriver_detach(&wmihp_cd);
			return err;
		}

		return 0;

	case MODULE_CMD_FINI:

		err = config_cfdata_detach(wmihp_cfdata);

		if (err != 0)
			return err;

		config_cfattach_detach("wmihp", &wmihp_ca);
		config_cfdriver_detach(&wmihp_cd);

		return 0;

	default:
		return ENOTTY;
	}
}

#endif	/* _MODULE */
