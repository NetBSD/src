/*	$NetBSD: wmi_eeepc.c,v 1.2.8.2 2011/06/06 09:07:44 jruoho Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wmi_eeepc.c,v 1.2.8.2 2011/06/06 09:07:44 jruoho Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/module.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/wmi/wmi_acpivar.h>

#include <dev/sysmon/sysmonvar.h>

#define _COMPONENT			ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME			("wmi_eeepc")

#define WMI_EEEPC_HK_VOLUME_UP		0x30
#define WMI_EEEPC_HK_VOLUME_DOWN	0x31
#define WMI_EEEPC_HK_VOLUME_MUTE	0x32
#define WMI_EEEPC_HK_DISPLAY_CYCLE	0xCC
#define WMI_EEEPC_HK_DISPLAY_OFF	0xE9
/*      WMI_EEEPC_HK_UNKNOWN		0xXX */

#define WMI_EEEPC_PSW_DISPLAY_CYCLE	0
#define WMI_EEEPC_PSW_COUNT		1

#define WMI_EEEPC_GUID_EVENT		"ABBC0F72-8EA1-11D1-00A0-C90629100000"

struct wmi_eeepc_softc {
	device_t		sc_dev;
	device_t		sc_parent;
	struct sysmon_pswitch	sc_smpsw[WMI_EEEPC_PSW_COUNT];
	bool			sc_smpsw_valid;
};

static int	wmi_eeepc_match(device_t, cfdata_t, void *);
static void	wmi_eeepc_attach(device_t, device_t, void *);
static int	wmi_eeepc_detach(device_t, int);
static void	wmi_eeepc_notify_handler(ACPI_HANDLE, uint32_t, void *);
static bool	wmi_eeepc_suspend(device_t, const pmf_qual_t *);
static bool	wmi_eeepc_resume(device_t, const pmf_qual_t *);

CFATTACH_DECL_NEW(wmieeepc, sizeof(struct wmi_eeepc_softc),
    wmi_eeepc_match, wmi_eeepc_attach, wmi_eeepc_detach, NULL);

static int
wmi_eeepc_match(device_t parent, cfdata_t match, void *aux)
{
	return acpi_wmi_guid_match(parent, WMI_EEEPC_GUID_EVENT);
}

static void
wmi_eeepc_attach(device_t parent, device_t self, void *aux)
{
	static const int dc = WMI_EEEPC_PSW_DISPLAY_CYCLE;
	struct wmi_eeepc_softc *sc = device_private(self);
	ACPI_STATUS rv;

	sc->sc_dev = self;
	sc->sc_parent = parent;
	sc->sc_smpsw_valid = false;

	rv = acpi_wmi_event_register(parent, wmi_eeepc_notify_handler);

	if (ACPI_FAILURE(rv)) {
		aprint_error(": failed to install WMI notify handler\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Asus Eee PC WMI mappings\n");

	sc->sc_smpsw[dc].smpsw_type = PSWITCH_TYPE_HOTKEY;
	sc->sc_smpsw[dc].smpsw_name = PSWITCH_HK_DISPLAY_CYCLE;

	if (sysmon_pswitch_register(&sc->sc_smpsw[dc]) == 0)
		sc->sc_smpsw_valid = true;

	(void)pmf_device_register(self, wmi_eeepc_suspend, wmi_eeepc_resume);
}

static int
wmi_eeepc_detach(device_t self, int flags)
{
	struct wmi_eeepc_softc *sc = device_private(self);
	device_t parent = sc->sc_parent;

	(void)pmf_device_deregister(self);
	(void)acpi_wmi_event_deregister(parent);

	return 0;
}

static bool
wmi_eeepc_suspend(device_t self, const pmf_qual_t *qual)
{
	struct wmi_eeepc_softc *sc = device_private(self);
	device_t parent = sc->sc_parent;

	(void)acpi_wmi_event_deregister(parent);

	return true;
}

static bool
wmi_eeepc_resume(device_t self, const pmf_qual_t *qual)
{
	struct wmi_eeepc_softc *sc = device_private(self);
	device_t parent = sc->sc_parent;

	(void)acpi_wmi_event_register(parent, wmi_eeepc_notify_handler);

	return true;
}

static void
wmi_eeepc_notify_handler(ACPI_HANDLE hdl, uint32_t evt, void *aux)
{
	static const int dc = WMI_EEEPC_PSW_DISPLAY_CYCLE;
	struct wmi_eeepc_softc *sc;
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

	if (obj->Type != ACPI_TYPE_INTEGER) {
		rv = AE_TYPE;
		goto out;
	}

	if (obj->Integer.Value > UINT32_MAX) {
		rv = AE_LIMIT;
		goto out;
	}

	val = obj->Integer.Value;

	switch (val) {

	case WMI_EEEPC_HK_VOLUME_UP:
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_UP);
		break;

	case WMI_EEEPC_HK_VOLUME_DOWN:
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_DOWN);
		break;

	case WMI_EEEPC_HK_VOLUME_MUTE:
		pmf_event_inject(NULL, PMFE_AUDIO_VOLUME_TOGGLE);
		break;

	case WMI_EEEPC_HK_DISPLAY_CYCLE:

		if (sc->sc_smpsw_valid != true) {
			rv = AE_ABORT_METHOD;
			break;
		}

		sysmon_pswitch_event(&sc->sc_smpsw[dc], PSWITCH_EVENT_PRESSED);
		break;

	case WMI_EEEPC_HK_DISPLAY_OFF:
		break;

	default:
		aprint_debug_dev(sc->sc_dev,
		    "unknown key 0x%02X for event 0x%02X\n", val, evt);
		break;
	}

out:
	if (buf.Pointer != NULL)
		ACPI_FREE(buf.Pointer);

	if (ACPI_FAILURE(rv))
		aprint_error_dev(sc->sc_dev, "failed to get data for "
		    "event 0x%02X: %s\n", evt, AcpiFormatException(rv));
}

MODULE(MODULE_CLASS_DRIVER, wmieeepc, "acpiwmi");

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
wmieeepc_modcmd(modcmd_t cmd, void *aux)
{
	int rv = 0;

	switch (cmd) {

	case MODULE_CMD_INIT:

#ifdef _MODULE
		rv = config_init_component(cfdriver_ioconf_wmieeepc,
		    cfattach_ioconf_wmieeepc, cfdata_ioconf_wmieeepc);
#endif
		break;

	case MODULE_CMD_FINI:

#ifdef _MODULE
		rv = config_fini_component(cfdriver_ioconf_wmieeepc,
		    cfattach_ioconf_wmieeepc, cfdata_ioconf_wmieeepc);
#endif
		break;

	default:
		rv = ENOTTY;
	}

	return rv;
}
