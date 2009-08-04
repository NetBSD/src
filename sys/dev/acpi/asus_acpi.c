/* $NetBSD: asus_acpi.c,v 1.10 2009/08/04 23:23:39 jmcneill Exp $ */

/*-
 * Copyright (c) 2007, 2008, 2009 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: asus_acpi.c,v 1.10 2009/08/04 23:23:39 jmcneill Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include <dev/acpi/acpivar.h>

struct asus_softc {
	device_t		sc_dev;
	struct acpi_devnode	*sc_node;

#define ASUS_PSW_DISPLAY_CYCLE	0
#define ASUS_PSW_LAST		1
	struct sysmon_pswitch	sc_smpsw[ASUS_PSW_LAST];
	bool			sc_smpsw_valid;

	struct sysmon_envsys	*sc_sme;
#define	ASUS_SENSOR_FAN		0
#define	ASUS_SENSOR_LAST	1
	envsys_data_t		sc_sensor[ASUS_SENSOR_LAST];

	ACPI_INTEGER		sc_brightness;
	ACPI_INTEGER		sc_cfvnum;

	struct sysctllog	*sc_log;
	int			sc_cfv_mib;
	int			sc_cfvnum_mib;
};

#define ASUS_NOTIFY_WirelessSwitch	0x10
#define ASUS_NOTIFY_BrightnessLow	0x20
#define	ASUS_NOTIFY_BrightnessHigh	0x2f
#define ASUS_NOTIFY_DisplayCycle	0x30
#define ASUS_NOTIFY_WindowSwitch	0x12	/* XXXJDM ?? */
#define ASUS_NOTIFY_VolumeMute		0x13
#define ASUS_NOTIFY_VolumeDown		0x14
#define ASUS_NOTIFY_VolumeUp		0x15

#define	ASUS_METHOD_SDSP	"SDSP"
#define		ASUS_SDSP_LCD	0x01
#define		ASUS_SDSP_CRT	0x02
#define		ASUS_SDSP_TV	0x04
#define		ASUS_SDSP_DVI	0x08
#define		ASUS_SDSP_ALL	\
		(ASUS_SDSP_LCD | ASUS_SDSP_CRT | ASUS_SDSP_TV | ASUS_SDSP_DVI)
#define ASUS_METHOD_PBLG	"PBLG"
#define ASUS_METHOD_PBLS	"PBLS"
#define	ASUS_METHOD_CFVS	"CFVS"
#define	ASUS_METHOD_CFVG	"CFVG"

#define	ASUS_EC_METHOD_FAN_RPMH	"\\_SB.PCI0.SBRG.EC0.SC05"
#define	ASUS_EC_METHOD_FAN_RPML	"\\_SB.PCI0.SBRG.EC0.SC06"

static int	asus_match(device_t, cfdata_t, void *);
static void	asus_attach(device_t, device_t, void *);
static int	asus_detach(device_t, int);

static void	asus_notify_handler(ACPI_HANDLE, UINT32, void *);

static void	asus_init(device_t);
static bool	asus_suspend(device_t PMF_FN_PROTO);
static bool	asus_resume(device_t PMF_FN_PROTO);

static void	asus_sysctl_setup(struct asus_softc *);

static void	asus_sensors_refresh(struct sysmon_envsys *, envsys_data_t *);
static bool	asus_get_fan_speed(struct asus_softc *, uint32_t *);

CFATTACH_DECL_NEW(asus, sizeof(struct asus_softc),
    asus_match, asus_attach, asus_detach, NULL);

static const char * const asus_ids[] = {
	"ASUS010",
	NULL
};

static int
asus_match(device_t parent, cfdata_t match, void *opaque)
{
	struct acpi_attach_args *aa = opaque;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, asus_ids);
}

static void
asus_attach(device_t parent, device_t self, void *opaque)
{
	struct asus_softc *sc = device_private(self);
	struct acpi_attach_args *aa = opaque;
	ACPI_STATUS rv;

	sc->sc_node = aa->aa_node;
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	asus_init(self);
	asus_sysctl_setup(sc);

	sc->sc_smpsw_valid = true;
	sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE].smpsw_name =
	    PSWITCH_HK_DISPLAY_CYCLE;
	sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE].smpsw_type =
	    PSWITCH_TYPE_HOTKEY;
	if (sysmon_pswitch_register(&sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE])) {
		aprint_error_dev(self, "couldn't register with sysmon\n");
		sc->sc_smpsw_valid = false;
	}

	if (asus_get_fan_speed(sc, NULL) == false)
		goto nosensors;

	sc->sc_sme = sysmon_envsys_create();

	strcpy(sc->sc_sensor[ASUS_SENSOR_FAN].desc, "fan");
	sc->sc_sensor[ASUS_SENSOR_FAN].units = ENVSYS_SFANRPM;
	sysmon_envsys_sensor_attach(sc->sc_sme,
	    &sc->sc_sensor[ASUS_SENSOR_FAN]);

	sc->sc_sme->sme_name = device_xname(self);
	sc->sc_sme->sme_cookie = sc;
	sc->sc_sme->sme_refresh = asus_sensors_refresh;
	sc->sc_sme->sme_flags = SME_POLL_ONLY;

	if (sysmon_envsys_register(sc->sc_sme)) {
		aprint_error_dev(self, "couldn't register with envsys\n");
		sysmon_envsys_destroy(sc->sc_sme);
		sc->sc_sme = NULL;
	}
nosensors:

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle, ACPI_ALL_NOTIFY,
	    asus_notify_handler, sc);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "couldn't install notify handler: %s\n",
		    AcpiFormatException(rv));

	if (!pmf_device_register(self, asus_suspend, asus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
asus_detach(device_t self, int flags)
{
	struct asus_softc *sc = device_private(self);
	int i;

	if (sc->sc_smpsw_valid)
		for (i = 0; i < ASUS_PSW_LAST; i++)
			sysmon_pswitch_unregister(&sc->sc_smpsw[i]);

	if (sc->sc_sme)
		sysmon_envsys_unregister(sc->sc_sme);
	if (sc->sc_log)
		sysctl_teardown(&sc->sc_log);
	pmf_device_deregister(self);

	return 0;
}

static void
asus_notify_handler(ACPI_HANDLE hdl, UINT32 notify, void *opaque)
{
	struct asus_softc *sc = opaque;

	if (notify >= ASUS_NOTIFY_BrightnessLow &&
	    notify <= ASUS_NOTIFY_BrightnessHigh) {
		aprint_debug_dev(sc->sc_dev, "brightness %d percent\n",
		    (notify & 0xf) * 100 / 0xf);
		return;
	}

	switch (notify) {
	case ASUS_NOTIFY_WirelessSwitch:	/* handled by AML */
	case ASUS_NOTIFY_WindowSwitch:		/* XXXJDM what is this? */
		break;
	case ASUS_NOTIFY_DisplayCycle:
		if (sc->sc_smpsw_valid == false)
			break;
		sysmon_pswitch_event(&sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE],
		    PSWITCH_EVENT_PRESSED);
		break;
	case ASUS_NOTIFY_VolumeMute:
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_TOGGLE);
		break;
	case ASUS_NOTIFY_VolumeDown:
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_DOWN);
		break;
	case ASUS_NOTIFY_VolumeUp:
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_UP);
		break;
	default:
		aprint_debug_dev(sc->sc_dev, "unknown event 0x%02x\n", notify);
		break;
	}
}

static void
asus_init(device_t self)
{
	struct asus_softc *sc = device_private(self);
	ACPI_STATUS rv;
	ACPI_OBJECT param;
	ACPI_OBJECT_LIST params;
	ACPI_BUFFER ret;
	ACPI_INTEGER cfv;

	ret.Pointer = NULL;
	ret.Length = ACPI_ALLOCATE_BUFFER;
	param.Type = ACPI_TYPE_INTEGER;
	param.Integer.Value = 0x40;	/* disable ASL display switching */
	params.Pointer = &param;
	params.Count = 1;

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, "INIT",
	    &params, &ret);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "couldn't evaluate INIT: %s\n",
		    AcpiFormatException(rv));

	if (ret.Pointer)
		AcpiOsFree(ret.Pointer);

	rv = acpi_eval_integer(sc->sc_node->ad_handle, ASUS_METHOD_CFVG, &cfv);
	if (ACPI_FAILURE(rv))
		return;

	sc->sc_cfvnum = (cfv >> 8) & 0xff;
}

static bool
asus_suspend(device_t self PMF_FN_ARGS)
{
	struct asus_softc *sc = device_private(self);
	ACPI_STATUS rv;

	/* capture display brightness when we're sleeping */
	rv = acpi_eval_integer(sc->sc_node->ad_handle, ASUS_METHOD_PBLG,
	    &sc->sc_brightness);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(sc->sc_dev, "couldn't evaluate PBLG: %s\n",
		    AcpiFormatException(rv));

	return true;
}

static bool
asus_resume(device_t self PMF_FN_ARGS)
{
	struct asus_softc *sc = device_private(self);
	ACPI_STATUS rv;
	ACPI_OBJECT param;
	ACPI_OBJECT_LIST params;
	ACPI_BUFFER ret;

	asus_init(self);

	/* restore previous display brightness */
	ret.Pointer = NULL;
	ret.Length = ACPI_ALLOCATE_BUFFER;
	param.Type = ACPI_TYPE_INTEGER;
	param.Integer.Value = sc->sc_brightness;
	params.Pointer = &param;
	params.Count = 1;

	rv = AcpiEvaluateObject(sc->sc_node->ad_handle, ASUS_METHOD_PBLS,
	    &params, &ret);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "couldn't evaluate PBLS: %s\n",
		    AcpiFormatException(rv));

	return true;
}

static int
asus_sysctl_verify(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct asus_softc *sc;
	ACPI_STATUS rv;
	ACPI_INTEGER cfv;
	ACPI_OBJECT param, retval;
	ACPI_OBJECT_LIST params;
	ACPI_BUFFER ret;
	int err, tmp;

	node = *rnode;
	sc = rnode->sysctl_data;
	if (node.sysctl_num == sc->sc_cfv_mib) {
		rv = acpi_eval_integer(sc->sc_node->ad_handle,
		    ASUS_METHOD_CFVG, &cfv);
		if (ACPI_FAILURE(rv))
			return ENXIO;
		tmp = cfv & 0xff;
		node.sysctl_data = &tmp;
		err = sysctl_lookup(SYSCTLFN_CALL(&node));
		if (err || newp == NULL)
			return err;

		if (tmp < 0 || tmp >= sc->sc_cfvnum)
			return EINVAL;

		ret.Pointer = &retval;
		ret.Length = sizeof(retval);
		param.Type = ACPI_TYPE_INTEGER;
		param.Integer.Value = tmp;
		params.Pointer = &param;
		params.Count = 1;

		rv = AcpiEvaluateObject(sc->sc_node->ad_handle,
		    ASUS_METHOD_CFVS, &params, &ret);
		if (ACPI_FAILURE(rv))
			return ENXIO;
	}

	return 0;
}

static void
asus_sysctl_setup(struct asus_softc *sc)
{
	const struct sysctlnode *node, *node_cfv, *node_ncfv;
	int err, node_mib;

	if (sc->sc_cfvnum == 0)
		return;

	err = sysctl_createv(&sc->sc_log, 0, NULL, NULL, 0,
	    CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW, CTL_EOL);
	if (err)
		goto sysctl_err;
	err = sysctl_createv(&sc->sc_log, 0, NULL, &node, 0,
	    CTLTYPE_NODE, device_xname(sc->sc_dev), NULL, NULL, 0,
	    NULL, 0, CTL_HW, CTL_CREATE, CTL_EOL);
	if (err)
		goto sysctl_err;
	node_mib = node->sysctl_num;
	err = sysctl_createv(&sc->sc_log, 0, NULL, &node_ncfv,
	    CTLFLAG_READONLY, CTLTYPE_INT, "ncfv",
	    SYSCTL_DESCR("Number of CPU frequency/voltage modes"),
	    NULL, 0, &sc->sc_cfvnum, 0,
	    CTL_HW, node_mib, CTL_CREATE, CTL_EOL);
	if (err)
		goto sysctl_err;
	sc->sc_cfvnum_mib = node_ncfv->sysctl_num;
	err = sysctl_createv(&sc->sc_log, 0, NULL, &node_cfv,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "cfv",
	    SYSCTL_DESCR("Current CPU frequency/voltage mode"),
	    asus_sysctl_verify, 0, sc, 0,
	    CTL_HW, node_mib, CTL_CREATE, CTL_EOL);
	if (err)
		goto sysctl_err;
	sc->sc_cfv_mib = node_cfv->sysctl_num;

	return;
sysctl_err:
	aprint_error_dev(sc->sc_dev, "failed to add sysctl nodes. (%d)\n", err);
}

static void
asus_sensors_refresh(struct sysmon_envsys *sme, envsys_data_t *edata)
{
	struct asus_softc *sc = sme->sme_cookie;
	uint32_t rpm;

	switch (edata->sensor) {
	case ASUS_SENSOR_FAN:
		if (asus_get_fan_speed(sc, &rpm)) {
			edata->value_cur = rpm;
			edata->state = ENVSYS_SVALID;
		} else
			edata->state = ENVSYS_SINVALID;
		break;
	}
}

static bool
asus_get_fan_speed(struct asus_softc *sc, uint32_t *speed)
{
	ACPI_INTEGER rpmh, rpml;
	ACPI_STATUS rv;

	rv = acpi_eval_integer(sc->sc_node->ad_handle,
	    ASUS_EC_METHOD_FAN_RPMH, &rpmh);
	if (ACPI_FAILURE(rv))
		return false;
	rv = acpi_eval_integer(sc->sc_node->ad_handle,
	    ASUS_EC_METHOD_FAN_RPML, &rpml);
	if (ACPI_FAILURE(rv))
		return false;

	if (speed)
		*speed = (rpmh << 8) | rpml;
	return true;
}
