/* $NetBSD: thinkpad_acpi.c,v 1.5 2007/12/21 21:24:45 jmcneill Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by Jared D. McNeill.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: thinkpad_acpi.c,v 1.5 2007/12/21 21:24:45 jmcneill Exp $");

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
#include <machine/pio.h>
#endif

#define THINKPAD_NSENSORS	8

typedef struct thinkpad_softc {
	device_t		sc_dev;
	device_t		sc_ecdev;
	struct acpi_devnode	*sc_node;
	ACPI_HANDLE		sc_cmoshdl;
	bool			sc_cmoshdl_valid;

	struct sysmon_pswitch	sc_smpsw[2];
#define TP_PSW_SLEEP		0
#define	TP_PSW_HIBERNATE	1
	bool			sc_smpsw_valid;

	struct sysmon_envsys	*sc_sme;
	envsys_data_t		sc_sensor[THINKPAD_NSENSORS];
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

#define THINKPAD_HKEY_VERSION		0x0100

static int	thinkpad_match(device_t, struct cfdata *, void *);
static void	thinkpad_attach(device_t, device_t, void *);

static ACPI_STATUS thinkpad_mask_init(thinkpad_softc_t *, uint32_t);
static void	thinkpad_notify_handler(ACPI_HANDLE, UINT32, void *);
static void	thinkpad_get_hotkeys(void *);

static void	thinkpad_temp_init(thinkpad_softc_t *);
static void	thinkpad_temp_refresh(struct sysmon_envsys *, envsys_data_t *);

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
thinkpad_match(device_t parent, struct cfdata *match, void *opaque)
{
	struct acpi_attach_args *aa = (struct acpi_attach_args *)opaque;
	ACPI_HANDLE hdl;
	ACPI_INTEGER ver;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	if (!acpi_match_hid(aa->aa_node->ad_devinfo, thinkpad_ids))
		return 0;

	/* No point in attaching if we can't find the CMOS method */
	if (ACPI_FAILURE(AcpiGetHandle(NULL, "\\UCMS", &hdl)))
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
	device_t curdev;
	ACPI_STATUS rv;
	ACPI_INTEGER val;

	sc->sc_node = aa->aa_node;
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	/* T61 uses \UCMS method for issuing CMOS commands */
	rv = AcpiGetHandle(NULL, "\\UCMS", &sc->sc_cmoshdl);
	if (ACPI_FAILURE(rv))
		sc->sc_cmoshdl_valid = false;
	else {
		aprint_verbose_dev(self, "using CMOS at \\UCMS\n");
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
		aprint_verbose_dev(self, "using EC at %s\n",
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
	sc->sc_smpsw_valid = true;

	sc->sc_smpsw[TP_PSW_SLEEP].smpsw_name = device_xname(self);
	sc->sc_smpsw[TP_PSW_SLEEP].smpsw_type = PSWITCH_TYPE_SLEEP;
#if notyet
	sc->sc_smpsw[TP_PSW_HIBERNATE].smpsw_name = device_xname(self);
	sc->sc_smpsw[TP_PSW_HIBERNATE].smpsw_type = PSWITCH_TYPE_HIBERNATE;
#endif

	if (sysmon_pswitch_register(&sc->sc_smpsw[TP_PSW_SLEEP]) != 0) {
		aprint_error_dev(self, "couldn't register with sysmon\n");
		sc->sc_smpsw_valid = false;
	}
#if notyet
	if (sysmon_pswitch_register(&sc->sc_smpsw[TP_PSW_HIBERNATE]) != 0) {
		aprint_error_dev(self, "couldn't register with sysmon\n");
		sc->sc_smpsw_valid = false;
	}
#endif

	/* Register temperature sensors with envsys */
	thinkpad_temp_init(sc);

fail:
	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    thinkpad_brightness_up, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    thinkpad_brightness_down, true))
		aprint_error_dev(self, "couldn't register event handler\n");

	return;
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
		case THINKPAD_NOTIFY_DisplayCycle:
#if notyet
			pmf_event_inject(NULL, PMFE_DISPLAY_CYCLE);
#endif
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
			break;
#endif
		case THINKPAD_NOTIFY_FnF1:
		case THINKPAD_NOTIFY_LockScreen:
		case THINKPAD_NOTIFY_BatteryInfo:
		case THINKPAD_NOTIFY_WirelessSwitch:
		case THINKPAD_NOTIFY_FnF6:
		case THINKPAD_NOTIFY_PointerSwitch:
		case THINKPAD_NOTIFY_EjectButton:
		case THINKPAD_NOTIFY_FnF10:
		case THINKPAD_NOTIFY_FnF11:
		case THINKPAD_NOTIFY_ThinkLight:
		case THINKPAD_NOTIFY_Zoom:
		case THINKPAD_NOTIFY_ThinkVantage:
			/* XXXJDM we should deliver hotkeys as keycodes */
			break;
		default:
			aprint_debug_dev(self, "notify event 0x%03x\n", event);
			break;
		}
	}

	return;
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
thinkpad_temp_init(thinkpad_softc_t *sc)
{
	char sname[5] = "TMP?";
	int i, err;

	if (sc->sc_ecdev == NULL)
		return;	/* no chance of this working */

	sc->sc_sme = sysmon_envsys_create();
	for (i = 0; i < THINKPAD_NSENSORS; i++) {
		sname[3] = '0' + i;
		strcpy(sc->sc_sensor[i].desc, sname);
		sc->sc_sensor[i].units = ENVSYS_STEMP;

		if (sysmon_envsys_sensor_attach(sc->sc_sme, &sc->sc_sensor[i]))
			aprint_error_dev(sc->sc_dev,
			    "couldn't attach sensor %s\n", sname);
	}

	sc->sc_sme->sme_name = device_xname(sc->sc_dev);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = thinkpad_temp_refresh;

	err = sysmon_envsys_register(sc->sc_sme);
	if (err) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't register with sysmon: %d\n", err);
		sysmon_envsys_destroy(sc->sc_sme);
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
	outb(0x70, 0x6c);
	return inb(0x71) & 7;
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
