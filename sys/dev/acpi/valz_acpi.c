/*	$NetBSD: valz_acpi.c,v 1.3.6.2 2010/08/11 22:53:17 yamt Exp $	*/

/*
 * Copyright (c) 2010 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: valz_acpi.c,v 1.3.6.2 2010/08/11 22:53:17 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

struct valz_softc {
	device_t sc_dev;
	ACPI_HANDLE sc_lcd;
};

enum valz_action {
	VALZ_BACKLIGHT_NOCHANGE,
	VALZ_BACKLIGHT_DECREASE,
	VALZ_BACKLIGHT_INCREASE,
};

static const char * const valz_hids[] = {
	"TOS1900",
	NULL
};

static int	valz_match(device_t, cfdata_t, void *);
static void	valz_attach(device_t, device_t, void *);
static ACPI_STATUS valz_check_bcl(ACPI_HANDLE, uint32_t, void*, void **);
static void	valz_locate_lcd(struct valz_softc *);
static void	valz_lcd_notify_handler(ACPI_HANDLE, uint32_t, void *);
static void	valz_adjust_backlight(struct valz_softc *, enum valz_action);
static void	valz_pmf_brightness_decrease(device_t);
static void	valz_pmf_brightness_increase(device_t);

CFATTACH_DECL_NEW(valz, sizeof(struct valz_softc),
    valz_match, valz_attach, NULL, NULL);

static int
valz_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, valz_hids);
}

static void
valz_attach(device_t parent, device_t self, void *aux)
{
	struct valz_softc *sc;
	ACPI_STATUS rv;

	aprint_naive("\n");
	aprint_normal("\n");

	sc = device_private(self);
	sc->sc_dev = self;
	sc->sc_lcd = NULL;

	valz_locate_lcd(sc);

	if (sc->sc_lcd == NULL) {
		aprint_error_dev(self, "failed to find a usable LCD\n");
		return;
	}

	if(!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    valz_pmf_brightness_decrease, false))
		aprint_error_dev(self, "failed to register event handler\n");

	if(!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    valz_pmf_brightness_increase, false))
		aprint_error_dev(self, "failed to register event handler\n");

	/*
	 * XXX: This is broken.
	 *
	 *	It claims resources that do not belong to it.
	 *
	 *	It either prevents an ACPI video driver for
	 *	receiving events or duplicates the notifications.
	 */
	rv = AcpiInstallNotifyHandler(sc->sc_lcd, ACPI_DEVICE_NOTIFY,
		valz_lcd_notify_handler, sc);
	if (ACPI_FAILURE(rv))
		aprint_error_dev(self, "failed to install notify handler; %s\n",
		     AcpiFormatException(rv));
}

static void
valz_lcd_notify_handler(ACPI_HANDLE handle, uint32_t notify, void *context)
{
	struct valz_softc *sc = context;

	/* XXX magic */
	switch (notify) {
	case 0x86:
		pmf_event_inject(sc->sc_dev, PMFE_DISPLAY_BRIGHTNESS_UP);
		break;
	case 0x87:
		pmf_event_inject(sc->sc_dev, PMFE_DISPLAY_BRIGHTNESS_DOWN);
		break;
	default:
		log(LOG_INFO, "%s: unknown notify: 0x%x\n",
		    device_xname(sc->sc_dev), notify);
		break;
	}
}

static void
valz_locate_lcd(struct valz_softc *sc)
{
	ACPI_HANDLE parent;
	ACPI_STATUS rv;

	rv = AcpiGetHandle(ACPI_ROOT_OBJECT, "\\_SB_", &parent);
	if (ACPI_FAILURE(rv))
		return;

        AcpiWalkNamespace(ACPI_TYPE_DEVICE, parent, 100,
			  valz_check_bcl, NULL, sc, NULL);
}

static ACPI_STATUS
valz_check_bcl(ACPI_HANDLE handle, uint32_t level,
    void *context, void **status)
{
	struct valz_softc *sc = context;
	ACPI_STATUS rv;
	ACPI_BUFFER buf;

	rv = acpi_eval_struct(handle, "_BCL", &buf);
	if (ACPI_FAILURE(rv))
		return AE_OK;

	sc->sc_lcd = handle;

	if (buf.Pointer)
		AcpiOsFree(buf.Pointer);

	return AE_OK;
}

static void
valz_adjust_backlight(struct valz_softc *sc, enum valz_action action)
{
	ACPI_STATUS rv;
	ACPI_INTEGER old_level, new_level;

	if (sc->sc_lcd == NULL)
		return;

	/* Get current backlight level. */
	rv = acpi_eval_integer(sc->sc_lcd, "_BQC", &old_level);
	if (ACPI_FAILURE(rv)) {
		log(LOG_CRIT, "%s: _BQC failed evaluation: %s\n",
		    device_xname(sc->sc_dev), AcpiFormatException(rv));
		return;
	}

	new_level = old_level;

	switch (action) {
	case VALZ_BACKLIGHT_DECREASE:
		if(old_level >= 10)
			new_level -= 10;
		break;
	case VALZ_BACKLIGHT_INCREASE:
		if(old_level <= 60)
			new_level += 10;
		break;
	default:
		return;
	}

	if (new_level == old_level)
		return;

	rv = acpi_eval_set_integer(sc->sc_lcd, "_BCM", new_level);
	if (ACPI_FAILURE(rv))
		log(LOG_CRIT, "%s: _BCM failed evaluation: %s\n",
		    device_xname(sc->sc_dev), AcpiFormatException(rv));
}

static void
valz_pmf_brightness_decrease(device_t self)
{
	struct valz_softc *sc;
	sc = device_private(self);
	valz_adjust_backlight(sc, VALZ_BACKLIGHT_DECREASE);
}

static void
valz_pmf_brightness_increase(device_t self)
{
	struct valz_softc *sc;
	sc = device_private(self);
	valz_adjust_backlight(sc, VALZ_BACKLIGHT_INCREASE);
}
