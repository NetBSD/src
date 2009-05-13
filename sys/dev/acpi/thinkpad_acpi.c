/* $NetBSD: thinkpad_acpi.c,v 1.17.4.1 2009/05/13 17:19:10 jym Exp $ */

/*-
 * Copyright (c) 2007 Jared D. McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: thinkpad_acpi.c,v 1.17.4.1 2009/05/13 17:19:10 jym Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/pmf.h>
#include <sys/queue.h>
#include <sys/kmem.h>

#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_ecvar.h>

#if defined(__i386__) || defined(__amd64__)
#include <dev/isa/isareg.h>
#include <machine/pio.h>
#endif

#define	THINKPAD_NTEMPSENSORS	8
#define	THINKPAD_NFANSENSORS	1
#define	THINKPAD_NSENSORS	(THINKPAD_NTEMPSENSORS + THINKPAD_NFANSENSORS)

typedef struct thinkpad_softc {
	device_t		sc_dev;
	device_t		sc_ecdev;
	struct acpi_devnode	*sc_node;
	ACPI_HANDLE		sc_cmoshdl;
	bool			sc_cmoshdl_valid;

#define	TP_PSW_SLEEP		0
#define	TP_PSW_HIBERNATE	1
#define	TP_PSW_DISPLAY_CYCLE	2
#define	TP_PSW_LOCK_SCREEN	3
#define	TP_PSW_BATTERY_INFO	4
#define	TP_PSW_EJECT_BUTTON	5
#define	TP_PSW_ZOOM_BUTTON	6
#define	TP_PSW_VENDOR_BUTTON	7
#define	TP_PSW_LAST		8
	struct sysmon_pswitch	sc_smpsw[TP_PSW_LAST];
	bool			sc_smpsw_valid;

	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sensor[THINKPAD_NSENSORS];

	int			sc_display_state;
} thinkpad_softc_t;

/* Hotkey events */
#define	THINKPAD_NOTIFY_FnF1		0x001
#define	THINKPAD_NOTIFY_LockScreen	0x002
#define	THINKPAD_NOTIFY_BatteryInfo	0x003
#define	THINKPAD_NOTIFY_SleepButton	0x004
#define	THINKPAD_NOTIFY_WirelessSwitch	0x005
#define	THINKPAD_NOTIFY_FnF6		0x006
#define	THINKPAD_NOTIFY_DisplayCycle	0x007
#define	THINKPAD_NOTIFY_PointerSwitch	0x008
#define	THINKPAD_NOTIFY_EjectButton	0x009
#define	THINKPAD_NOTIFY_FnF10		0x00a
#define	THINKPAD_NOTIFY_FnF11		0x00b
#define	THINKPAD_NOTIFY_HibernateButton	0x00c
#define	THINKPAD_NOTIFY_BrightnessUp	0x010
#define	THINKPAD_NOTIFY_BrightnessDown	0x011
#define	THINKPAD_NOTIFY_ThinkLight	0x012
#define	THINKPAD_NOTIFY_Zoom		0x014
#define	THINKPAD_NOTIFY_ThinkVantage	0x018

#define	THINKPAD_CMOS_BRIGHTNESS_UP	0x04
#define	THINKPAD_CMOS_BRIGHTNESS_DOWN	0x05

#define	THINKPAD_HKEY_VERSION		0x0100

#define	THINKPAD_DISPLAY_LCD		0x01
#define	THINKPAD_DISPLAY_CRT		0x02
#define	THINKPAD_DISPLAY_DVI		0x08
#define	THINKPAD_DISPLAY_ALL \
	(THINKPAD_DISPLAY_LCD | THINKPAD_DISPLAY_CRT | THINKPAD_DISPLAY_DVI)

static int	thinkpad_match(device_t, cfdata_t, void *);
static void	thinkpad_attach(device_t, device_t, void *);

static ACPI_STATUS thinkpad_mask_init(thinkpad_softc_t *, uint32_t);
static void	thinkpad_notify_handler(ACPI_HANDLE, UINT32, void *);
static void	thinkpad_get_hotkeys(void *);

static void	thinkpad_sensors_init(thinkpad_softc_t *);
static void	thinkpad_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	thinkpad_temp_refresh(struct sysmon_envsys *, envsys_data_t *);
static void	thinkpad_fan_refresh(struct sysmon_envsys *, envsys_data_t *);

static void	thinkpad_wireless_toggle(thinkpad_softc_t *);

static bool	thinkpad_resume(device_t PMF_FN_PROTO);
static void	thinkpad_brightness_up(device_t);
static void	thinkpad_brightness_down(device_t);
static uint8_t	thinkpad_brightness_read(thinkpad_softc_t *sc);
static void	thinkpad_cmos(thinkpad_softc_t *, uint8_t);

CFATTACH_DECL_NEW(thinkpad, sizeof(thinkpad_softc_t),
    thinkpad_match, thinkpad_attach, NULL, NULL);

static const char * const thinkpad_ids[] = {
	"IBM0068",
	NULL
};

static int
thinkpad_match(device_t parent, cfdata_t match, void *opaque)
{
	struct acpi_attach_args *aa = (struct acpi_attach_args *)opaque;
	ACPI_INTEGER ver;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	if (!acpi_match_hid(aa->aa_node->ad_devinfo, thinkpad_ids))
		return 0;

	/* We only support hotkey version 0x0100 */
	if (ACPI_FAILURE(acpi_eval_integer(aa->aa_node->ad_handle, "MHKV",
	    &ver)))
		return 0;

	if (ver != THINKPAD_HKEY_VERSION)
		return 0;

	/* Cool, looks like we're good to go */
	return 1;
}

static void
thinkpad_attach(device_t parent, device_t self, void *opaque)
{
	thinkpad_softc_t *sc = device_private(self);
	struct acpi_attach_args *aa = (struct acpi_attach_args *)opaque;
	struct sysmon_pswitch *psw;
	device_t curdev;
	ACPI_STATUS rv;
	ACPI_INTEGER val;
	int i;

	sc->sc_node = aa->aa_node;
	sc->sc_dev = self;
	sc->sc_display_state = THINKPAD_DISPLAY_LCD;

	aprint_naive("\n");
	aprint_normal("\n");

	/* T61 uses \UCMS method for issuing CMOS commands */
	rv = AcpiGetHandle(NULL, "\\UCMS", &sc->sc_cmoshdl);
	if (ACPI_FAILURE(rv))
		sc->sc_cmoshdl_valid = false;
	else {
		aprint_debug_dev(self, "using CMOS at \\UCMS\n");
		sc->sc_cmoshdl_valid = true;
	}

	sc->sc_ecdev = NULL;
	TAILQ_FOREACH(curdev, &alldevs, dv_list)
		if (device_is_a(curdev, "acpiecdt") ||
		    device_is_a(curdev, "acpiec")) {
			sc->sc_ecdev = curdev;
			break;
		}
	if (sc->sc_ecdev)
		aprint_debug_dev(self, "using EC at %s\n",
		    device_xname(sc->sc_ecdev));

	/* Get the supported event mask */
	rv = acpi_eval_integer(sc->sc_node->ad_handle, "MHKA", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "couldn't evaluate MHKA: %s\n",
		    AcpiFormatException(rv));
		goto fail;
	}

	/* Enable all supported events */
	rv = thinkpad_mask_init(sc, val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "couldn't set event mask: %s\n",
		    AcpiFormatException(rv));
		goto fail;
	}

	/* Install notify handler for events */
	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, thinkpad_notify_handler, sc);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "couldn't install notify handler: %s\n",
		    AcpiFormatException(rv));

	/* Register power switches with sysmon */
	psw = sc->sc_smpsw;
	sc->sc_smpsw_valid = true;

	psw[TP_PSW_SLEEP].smpsw_name = device_xname(self);
	psw[TP_PSW_SLEEP].smpsw_type = PSWITCH_TYPE_SLEEP;
#if notyet
	psw[TP_PSW_HIBERNATE].smpsw_name = device_xname(self);
	mpsw[TP_PSW_HIBERNATE].smpsw_type = PSWITCH_TYPE_HIBERNATE;
#endif
	for (i = TP_PSW_DISPLAY_CYCLE; i < TP_PSW_LAST; i++)
		sc->sc_smpsw[i].smpsw_type = PSWITCH_TYPE_HOTKEY;
	psw[TP_PSW_DISPLAY_CYCLE].smpsw_name = PSWITCH_HK_DISPLAY_CYCLE;
	psw[TP_PSW_LOCK_SCREEN].smpsw_name = PSWITCH_HK_LOCK_SCREEN;
	psw[TP_PSW_BATTERY_INFO].smpsw_name = PSWITCH_HK_BATTERY_INFO;
	psw[TP_PSW_EJECT_BUTTON].smpsw_name = PSWITCH_HK_EJECT_BUTTON;
	psw[TP_PSW_ZOOM_BUTTON].smpsw_name = PSWITCH_HK_ZOOM_BUTTON;
	psw[TP_PSW_VENDOR_BUTTON].smpsw_name = PSWITCH_HK_VENDOR_BUTTON;

	for (i = 0; i < TP_PSW_LAST; i++) {
		/* not supported yet */
		if (i == TP_PSW_HIBERNATE)
			continue;
		if (sysmon_pswitch_register(&sc->sc_smpsw[i]) != 0) {
			aprint_error_dev(self,
			    "couldn't register with sysmon\n");
			sc->sc_smpsw_valid = false;
			break;
		}
	}

	/* Register temperature and fan sensors with envsys */
	thinkpad_sensors_init(sc);

fail:
	if (!pmf_device_register(self, NULL, thinkpad_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    thinkpad_brightness_up, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    thinkpad_brightness_down, true))
		aprint_error_dev(self, "couldn't register event handler\n");
}

static void
thinkpad_notify_handler(ACPI_HANDLE hdl, UINT32 notify, void *opaque)
{
	thinkpad_softc_t *sc = (thinkpad_softc_t *)opaque;
	device_t self = sc->sc_dev;
	ACPI_STATUS rv;
 
	if (notify != 0x80) {
		aprint_debug_dev(self, "unknown notify 0x%02x\n", notify);
		return;
	}

	rv = AcpiOsExecute(OSL_NOTIFY_HANDLER, thinkpad_get_hotkeys, sc);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "couldn't queue hotkey handler: %s\n",
		    AcpiFormatException(rv));
}

static void
thinkpad_get_hotkeys(void *opaque)
{
	thinkpad_softc_t *sc = (thinkpad_softc_t *)opaque;
	device_t self = sc->sc_dev;
	ACPI_STATUS rv;
	ACPI_INTEGER val;
	int type, event;
	
	for (;;) {
		rv = acpi_eval_integer(sc->sc_node->ad_handle, "MHKP", &val);
		if (ACPI_FAILURE(rv)) {
			aprint_error_dev(self, "couldn't evaluate MHKP: %s\n",
			    AcpiFormatException(rv));
			return;
		}

		if (val == 0)
			return;

		type = (val & 0xf000) >> 12;
		event = val & 0x0fff;

		if (type != 1)
			/* Only type 1 events are supported for now */
			continue;

		switch (event) {
		case THINKPAD_NOTIFY_BrightnessUp:
			thinkpad_brightness_up(self);
			break;
		case THINKPAD_NOTIFY_BrightnessDown:
			thinkpad_brightness_down(self);
			break;
		case THINKPAD_NOTIFY_WirelessSwitch:
			thinkpad_wireless_toggle(sc);
			break;
		case THINKPAD_NOTIFY_SleepButton:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(&sc->sc_smpsw[TP_PSW_SLEEP],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_HibernateButton:
#if notyet
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(&sc->sc_smpsw[TP_PSW_HIBERNATE],
			    PSWITCH_EVENT_PRESSED);
#endif
			break;
		case THINKPAD_NOTIFY_DisplayCycle:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(
			    &sc->sc_smpsw[TP_PSW_DISPLAY_CYCLE],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_LockScreen:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(
			    &sc->sc_smpsw[TP_PSW_LOCK_SCREEN],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_BatteryInfo:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(
			    &sc->sc_smpsw[TP_PSW_BATTERY_INFO],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_EjectButton:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(
			    &sc->sc_smpsw[TP_PSW_EJECT_BUTTON],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_Zoom:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(
			    &sc->sc_smpsw[TP_PSW_ZOOM_BUTTON],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_ThinkVantage:
			if (sc->sc_smpsw_valid == false)
				break;
			sysmon_pswitch_event(
			    &sc->sc_smpsw[TP_PSW_VENDOR_BUTTON],
			    PSWITCH_EVENT_PRESSED);
			break;
		case THINKPAD_NOTIFY_FnF1:
		case THINKPAD_NOTIFY_FnF6:
		case THINKPAD_NOTIFY_PointerSwitch:
		case THINKPAD_NOTIFY_FnF10:
		case THINKPAD_NOTIFY_FnF11:
		case THINKPAD_NOTIFY_ThinkLight:
			/* XXXJDM we should deliver hotkeys as keycodes */
			break;
		default:
			aprint_debug_dev(self, "notify event 0x%03x\n", event);
			break;
		}
	}
}

static ACPI_STATUS
thinkpad_mask_init(thinkpad_softc_t *sc, uint32_t mask)
{
	ACPI_OBJECT param[2];
	ACPI_OBJECT_LIST params;
	ACPI_STATUS rv;
	int i;

	/* Update hotkey mask */
	params.Count = 2;
	params.Pointer = param;
	param[0].Type = param[1].Type = ACPI_TYPE_INTEGER;

	for (i = 0; i < 32; i++) {
		param[0].Integer.Value = i + 1;
		param[1].Integer.Value = (((1 << i) & mask) != 0);

		rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "MHKM",
		    &params, NULL);
		if (ACPI_FAILURE(rv))
			return rv;
	}

	/* Enable hotkey events */
	params.Count = 1;
	param[0].Integer.Value = 1;
	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "MHKC", &params, NULL);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "couldn't enable hotkeys: %s\n",
		    AcpiFormatException(rv));
		return rv;
	}

	/* Claim ownership of brightness control */
	param[0].Integer.Value = 0;
	(void)AcpiEvaluateObject(sc->sc_node->ad_handle, "PWMS", &params, NULL);

	return AE_OK;
}

static void
thinkpad_sensors_init(thinkpad_softc_t *sc)
{
	char sname[5] = "TMP?";
	char fname[5] = "FAN?";
	int i, j, err;

	if (sc->sc_ecdev == NULL)
		return;	/* no chance of this working */

	sc->sc_sme = sysmon_envsys_create();
	for (i = 0; i < THINKPAD_NTEMPSENSORS; i++) {
		sname[3] = '0' + i;
		strcpy(sc->sc_sensor[i].desc, sname);
		sc->sc_sensor[i].units = ENVSYS_STEMP;

		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i]))
			aprint_error_dev(sc->sc_dev,
			    "couldn't attach sensor %s\n", sname);
	}
	j = i; /* THINKPAD_NTEMPSENSORS */
	for (; i < (j + THINKPAD_NFANSENSORS); i++) {
		fname[3] = '0' + (i - j);
		strcpy(sc->sc_sensor[i].desc, fname);
		sc->sc_sensor[i].units = ENVSYS_SFANRPM;

		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i]))
			aprint_error_dev(sc->sc_dev,
			    "couldn't attach sensor %s\n", fname);
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = thinkpad_sensors_refresh;

	err = sysmon_envsys_register(sc->sc_sme);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't register with sysmon: %d\n", err);
		sysmon_envsys_destroy(sc->sc_sme);
	}
}

static void
thinkpad_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	switch (edata->units) {
	case ENVSYS_STEMP:
		thinkpad_temp_refresh(sme, edata);
		break;
	case ENVSYS_SFANRPM:
		thinkpad_fan_refresh(sme, edata);
		break;
	default:
		break;
	}
}

static void
thinkpad_temp_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	thinkpad_softc_t *sc = sme->sme_cookie;
	char sname[5] = "TMP?";
	ACPI_INTEGER val;
	ACPI_STATUS rv;
	int temp;

	sname[3] = '0' + edata->sensor;
	rv = acpi_eval_integer(acpiec_get_handle(sc->sc_ecdev), sname, &val);
	if (ACPI_FAILURE(rv)) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	temp = (int)val;
	if (temp > 127 || temp < -127) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	edata->value_cur = temp * 1000000 + 273150000;
	edata->state = ENVSYS_SVALID;
}

static void
thinkpad_fan_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	thinkpad_softc_t *sc = sme->sme_cookie;
	ACPI_INTEGER lo;
	ACPI_INTEGER hi;
	ACPI_STATUS rv;
	int rpm;

	/*
	 * Read the low byte first to avoid a firmware bug.
	 */
	rv = acpiec_bus_read(sc->sc_ecdev, 0x84, &lo, 1);
	if (ACPI_FAILURE(rv)) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	rv = acpiec_bus_read(sc->sc_ecdev, 0x85, &hi, 1);
	if (ACPI_FAILURE(rv)) {
		edata->state = ENVSYS_SINVALID;
		return;
	}
	rpm = ((((int)hi) << 8) | ((int)lo));
	if (rpm < 0) {
		edata->state = ENVSYS_SINVALID;
		return;
	}

	edata->value_cur = rpm;
	if (rpm < edata->value_min || edata->value_min == -1)
		edata->value_min = rpm;
	if (rpm > edata->value_max || edata->value_max == -1)
		edata->value_max = rpm;
	edata->state = ENVSYS_SVALID;
}

static void
thinkpad_wireless_toggle(thinkpad_softc_t *sc)
{
	/* Ignore return value, as the hardware may not support bluetooth */
	(void)AcpiEvaluateObject(sc->sc_node->ad_handle, "BTGL", NULL, NULL);
}

static uint8_t
thinkpad_brightness_read(thinkpad_softc_t *sc)
{
#if defined(__i386__) || defined(__amd64__)
	/*
	 * We have two ways to get the current brightness -- either via
	 * magic RTC registers, or using the EC. Since I don't dare mess
	 * with the EC, and Thinkpads are x86-only, this will have to do
	 * for now.
	 */
	outb(IO_RTC, 0x6c);
	return inb(IO_RTC+1) & 7;
#else
	return 0;
#endif
}

static void
thinkpad_brightness_up(device_t self)
{
	thinkpad_softc_t *sc = device_private(self);

	if (thinkpad_brightness_read(sc) == 7)
		return;
	thinkpad_cmos(sc, THINKPAD_CMOS_BRIGHTNESS_UP);
}

static void
thinkpad_brightness_down(device_t self)
{
	thinkpad_softc_t *sc = device_private(self);

	if (thinkpad_brightness_read(sc) == 0)
		return;
	thinkpad_cmos(sc, THINKPAD_CMOS_BRIGHTNESS_DOWN);
}

static void
thinkpad_cmos(thinkpad_softc_t *sc, uint8_t cmd)
{
	ACPI_OBJECT param;
	ACPI_OBJECT_LIST params;
	ACPI_STATUS rv;

	if (sc->sc_cmoshdl_valid == false)
		return;
	
	params.Count = 1;
	params.Pointer = &param;
	param.Type = ACPI_TYPE_INTEGER;
	param.Integer.Value = cmd;
	rv = AcpiEvaluateObject(sc->sc_cmoshdl, NULL, &params, NULL);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(sc->sc_dev, "couldn't evalute CMOS: %s\n",
		    AcpiFormatException(rv));
}

static bool
thinkpad_resume(device_t dv PMF_FN_ARGS)
{
	ACPI_STATUS rv;
	ACPI_HANDLE pubs;

	rv = AcpiGetHandle(NULL, "\\_SB.PCI0.LPC.EC.PUBS", &pubs);
	if (ACPI_FAILURE(rv))
		return true;	/* not fatal */

	rv = AcpiEvaluateObject(pubs, "_ON", NULL, NULL);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(dv, "failed to execute PUBS._ON: %s\n",
		    AcpiFormatException(rv));

	return true;
}
