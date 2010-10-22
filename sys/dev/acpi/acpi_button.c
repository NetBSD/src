/*	$NetBSD: acpi_button.c,v 1.30.2.2 2010/10/22 07:21:51 uebayasi Exp $	*/

/*
 * Copyright 2001, 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 * ACPI Button driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_button.c,v 1.30.2.2 2010/10/22 07:21:51 uebayasi Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/systm.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define _COMPONENT		 ACPI_BUTTON_COMPONENT
ACPI_MODULE_NAME		 ("acpi_button")

#define ACPI_NOTIFY_BUTTON	 0x80

struct acpibut_softc {
	struct acpi_devnode	*sc_node;
	struct sysmon_pswitch	 sc_smpsw;
};

static const char * const power_button_hid[] = {
	"PNP0C0C",
	NULL
};

static const char * const sleep_button_hid[] = {
	"PNP0C0E",
	NULL
};

static int	acpibut_match(device_t, cfdata_t, void *);
static void	acpibut_attach(device_t, device_t, void *);
static int	acpibut_detach(device_t, int);
static void	acpibut_pressed_event(void *);
static void	acpibut_notify_handler(ACPI_HANDLE, uint32_t, void *);

CFATTACH_DECL_NEW(acpibut, sizeof(struct acpibut_softc),
    acpibut_match, acpibut_attach, acpibut_detach, NULL);

/*
 * acpibut_match:
 *
 *	Autoconfiguration `match' routine.
 */
static int
acpibut_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, power_button_hid))
		return 1;

	if (acpi_match_hid(aa->aa_node->ad_devinfo, sleep_button_hid))
		return 1;

	return 0;
}

/*
 * acpibut_attach:
 *
 *	Autoconfiguration `attach' routine.
 */
static void
acpibut_attach(device_t parent, device_t self, void *aux)
{
	struct acpibut_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	const char *desc;

	sc->sc_smpsw.smpsw_name = device_xname(self);

	if (acpi_match_hid(aa->aa_node->ad_devinfo, power_button_hid)) {
		sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_POWER;
		desc = "Power";
	} else if (acpi_match_hid(aa->aa_node->ad_devinfo, sleep_button_hid)) {
		sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_SLEEP;
		desc = "Sleep";
	} else
		panic("%s: impossible", __func__);

	aprint_naive(": ACPI %s Button\n", desc);
	aprint_normal(": ACPI %s Button\n", desc);

	sc->sc_node = aa->aa_node;

	(void)pmf_device_register(self, NULL, NULL);
	(void)sysmon_pswitch_register(&sc->sc_smpsw);
	(void)acpi_register_notify(sc->sc_node, acpibut_notify_handler);
}

/*
 * acpibut_detach:
 *
 *	Autoconfiguration `detach' routine.
 */
static int
acpibut_detach(device_t self, int flags)
{
	struct acpibut_softc *sc = device_private(self);

	pmf_device_deregister(self);
	acpi_deregister_notify(sc->sc_node);
	sysmon_pswitch_unregister(&sc->sc_smpsw);

	return 0;
}

/*
 * acpibut_pressed_event:
 *
 *	Deal with a button being pressed.
 */
static void
acpibut_pressed_event(void *arg)
{
	device_t dv = arg;
	struct acpibut_softc *sc = device_private(dv);

	aprint_debug_dev(dv, "button pressed\n");
	sysmon_pswitch_event(&sc->sc_smpsw, PSWITCH_EVENT_PRESSED);
}

/*
 * acpibut_notify_handler:
 *
 *	Callback from ACPI interrupt handler to notify us of an event.
 */
static void
acpibut_notify_handler(ACPI_HANDLE handle, uint32_t notify, void *context)
{
	static const int handler = OSL_NOTIFY_HANDLER;
	device_t dv = context;

	switch (notify) {

	case ACPI_NOTIFY_BUTTON:
		(void)AcpiOsExecute(handler, acpibut_pressed_event, dv);
		break;

	case ACPI_NOTIFY_DEVICE_WAKE:
		break;

	default:
		aprint_debug_dev(dv, "unknown notify 0x%02X\n", notify);
	}
}

#ifdef _MODULE

MODULE(MODULE_CLASS_DRIVER, acpibut, NULL);
CFDRIVER_DECL(acpibut, DV_DULL, NULL);

static int acpibutloc[] = { -1 };
extern struct cfattach acpibut_ca;

static struct cfparent acpiparent = {
	"acpinodebus", NULL, DVUNIT_ANY
};

static struct cfdata acpibut_cfdata[] = {
	{
		.cf_name = "acpibut",
		.cf_atname = "acpibut",
		.cf_unit = 0,
		.cf_fstate = FSTATE_STAR,
		.cf_loc = acpibutloc,
		.cf_flags = 0,
		.cf_pspec = &acpiparent,
	},

	{ NULL }
};

static int
acpibut_modcmd(modcmd_t cmd, void *context)
{
	int err;

	switch (cmd) {

	case MODULE_CMD_INIT:

		err = config_cfdriver_attach(&acpibut_cd);

		if (err != 0)
			return err;

		err = config_cfattach_attach("acpibut", &acpibut_ca);

		if (err != 0) {
			config_cfdriver_detach(&acpibut_cd);
			return err;
		}

		err = config_cfdata_attach(acpibut_cfdata, 1);

		if (err != 0) {
			config_cfattach_detach("acpibut", &acpibut_ca);
			config_cfdriver_detach(&acpibut_cd);
			return err;
		}

		return 0;

	case MODULE_CMD_FINI:

		err = config_cfdata_detach(acpibut_cfdata);

		if (err != 0)
			return err;

		config_cfattach_detach("acpibut", &acpibut_ca);
		config_cfdriver_detach(&acpibut_cd);

		return 0;

	default:
		return ENOTTY;
	}
}

#endif	/* _MODULE */
