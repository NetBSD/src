/* $NetBSD: asus_acpi.c,v 1.4.2.2 2008/10/10 22:30:58 skrll Exp $ */

/*-
 * Copyright (c) 2007, 2008 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: asus_acpi.c,v 1.4.2.2 2008/10/10 22:30:58 skrll Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/buf.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/pmf.h>

#include <dev/acpi/acpivar.h>

typedef struct asus_softc {
	device_t		sc_dev;
	struct acpi_devnode	*sc_node;

#define ASUS_PSW_DISPLAY_CYCLE	0
#define ASUS_PSW_LAST		1
	struct sysmon_pswitch	sc_smpsw[ASUS_PSW_LAST];
	bool			sc_smpsw_valid;

	ACPI_INTEGER		sc_brightness;
} asus_softc_t;

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

static int	asus_match(device_t, cfdata_t, void *);
static void	asus_attach(device_t, device_t, void *);

static void	asus_notify_handler(ACPI_HANDLE, UINT32, void *);

static void	asus_init(device_t);
static bool	asus_suspend(device_t PMF_FN_PROTO);
static bool	asus_resume(device_t PMF_FN_PROTO);

CFATTACH_DECL_NEW(asus, sizeof(asus_softc_t),
    asus_match, asus_attach, NULL, NULL);

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
	asus_softc_t *sc = device_private(self);
	struct acpi_attach_args *aa = opaque;
	ACPI_STATUS rv;

	sc->sc_node = aa->aa_node;
	sc->sc_dev = self;

	aprint_naive("\n");
	aprint_normal("\n");

	asus_init(self);

	sc->sc_smpsw_valid = true;
	sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE].smpsw_name =
	    PSWITCH_HK_DISPLAY_CYCLE;
	sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE].smpsw_type =
	    PSWITCH_TYPE_HOTKEY;
	if (sysmon_pswitch_register(&sc->sc_smpsw[ASUS_PSW_DISPLAY_CYCLE])) {
		aprint_error_dev(self, "couldn't register with sysmon\n");
		sc->sc_smpsw_valid = false;
	}

	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle, ACPI_ALL_NOTIFY,
	    asus_notify_handler, sc);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "couldn't install notify handler: %s\n",
		    AcpiFormatException(rv));

	if (!pmf_device_register(self, asus_suspend, asus_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static void
asus_notify_handler(ACPI_HANDLE hdl, UINT32 notify, void *opaque)
{
	asus_softc_t *sc = opaque;

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
	asus_softc_t *sc = device_private(self);
	ACPI_STATUS rv;
	ACPI_OBJECT param;
	ACPI_OBJECT_LIST params;
	ACPI_BUFFER ret;

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
}

static bool
asus_suspend(device_t self PMF_FN_ARGS)
{
	asus_softc_t *sc = device_private(self);
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
	asus_softc_t *sc = device_private(self);
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
