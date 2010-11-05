/*	$NetBSD: fujitsu_acpi.c,v 1.2 2010/11/05 10:35:00 gsutre Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gregoire Sutre.
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
 * ACPI Fujitsu Driver.
 *
 * This driver provides support for the ACPI devices FUJ02B1 and FUJ02E3 that
 * are commonly found in Fujitsu LifeBooks.  The driver does not support all
 * features of these devices, in particular volume control is not implemented.
 *
 * Information regarding the behavior of these devices was obtained from the
 * source code of the Linux and FreeBSD drivers, as well as from experiments on
 * a Fujitsu LifeBook P7120.
 *
 * The FUJ02B1 device is used to control the brightness level of the internal
 * display, the state (on/off) of the internal pointer, and the volume level of
 * the internal speakers or headphones.
 *
 * The FUJ02B1 device provides the following methods (or only a subset):
 *
 *	GSIF		supported hotkey status bits (bitmask for GHKS)
 *	GHKS		active hotkeys (bit field)
 *	{G,S}BLL	get/set the brightness level of the internal display
 *	{G,S}VOL	get/set the volume level of the internal speakers
 *	{G,S}MOU	get/set the switch state of the internal pointer
 *	RBLL		brightness radix (number of brightness levels)
 *	RVOL		volume radix (number of volume levels)
 *
 * Notifications are delivered to the FUJ02B1 device when functions hotkeys
 * (brightness, pointer) are released.  However, these notifications seem to be
 * purely informative: the BIOS already made the hardware changes corresponding
 * to the hotkey.
 *
 * Each bit in the value returned by GHKS remains set until the corresponding
 * get method (GBLL, GMOU or GVOL) is called.
 *
 * The FUJ02E3 device manages the laptop hotkeys (such as the `Eco' button) and
 * provides additional services (such as backlight on/off control) through the
 * FUNC method.
 *
 * The FUJ02E3 device provides the following methods (or only a subset):
 *
 *	GIRB		get next hotkey code from buffer
 *	FUNC		general-purpose method (four arguments)
 *
 * Notifications are delivered to the FUJ02E3 device when hotkeys are pressed
 * and when they are released.  The BIOS stores the corresponding codes in a
 * FIFO buffer, that can be read with the GIRB method.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fujitsu_acpi.c,v 1.2 2010/11/05 10:35:00 gsutre Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/mutex.h>
#include <sys/sysctl.h>
#include <sys/systm.h>

#include <dev/sysmon/sysmonvar.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>

#define _COMPONENT		ACPI_RESOURCE_COMPONENT
ACPI_MODULE_NAME		("fujitsu_acpi")

/* Notification code for events on ACPI Fujitsu devices. */
#define FUJITSU_NOTIFY_Event	0x80

/* Event bits returned by the GHKS method. */
#define FUJITSU_HKS_BRIGHTNESS	__BIT(0)
#define FUJITSU_HKS_POINTER	__BIT(3)

/* Modification status bits in the values returned by GBLL, GMOU and GIRB. */
#define FUJITSU_MODIFICATION_MASK	0xc0000000

/* Values returned by the GIRB method. */
#define FUJITSU_IRB_HOTKEY_RELEASE	0x0
#define FUJITSU_IRB_HOTKEY_PRESS(hk)	(0x410 | hk)

/* Hotkey index of a value returned by the GIRB method. */
#define FUJITSU_IRB_HOTKEY_INDEX(irb)	(irb & 0x3)

/* Values for the first argument of the FUNC method. */
#define FUJITSU_FUNC_TARGET_BACKLIGHT	0x1004

/* Values for the second argument of the FUNC method. */
#define FUJITSU_FUNC_COMMAND_SET	0x1
#define FUJITSU_FUNC_COMMAND_GET	0x2

/* Backlight values for the FUNC method. */
#define FUJITSU_FUNC_BACKLIGHT_ON	0x0
#define FUJITSU_FUNC_BACKLIGHT_REDUCED	0x2
#define FUJITSU_FUNC_BACKLIGHT_OFF	0x3

/* Value returned by FUNC on invalid arguments. */
#define FUJITSU_FUNC_INVALID_ARGS	0x80000000

/*
 * fujitsu_bp_softc:
 *
 *	Software state of an ACPI Fujitsu brightness & pointer controller.
 *	Valid brightness levels range from 0 to (sc_brightness_nlevels - 1).
 */
struct fujitsu_bp_softc {
	device_t		 sc_dev;
	struct acpi_devnode	*sc_node;
	struct sysctllog	*sc_log;
	kmutex_t		 sc_mtx;
	uint16_t		 sc_caps;
	uint8_t			 sc_brightness_nlevels;
};

/*
 * ACPI Fujitsu brightness & pointer controller capabilities (methods).
 */
#define FUJITSU_BP_CAP_GHKS	__BIT(0)
#define FUJITSU_BP_CAP_RBLL	__BIT(1)
#define FUJITSU_BP_CAP_GBLL	__BIT(2)
#define FUJITSU_BP_CAP_SBLL	__BIT(3)
#define FUJITSU_BP_CAP_GMOU	__BIT(4)
#define FUJITSU_BP_CAP_SMOU	__BIT(5)

/*
 * ACPI Fujitsu brightness & pointer controller HID.
 */
static const char * const fujitsu_bp_hid[] = {
	"FUJ02B1",
	NULL
};

/*
 * fujitsu_hk_softc:
 *
 *	Software state of an ACPI Fujitsu hotkeys controller.
 */
struct fujitsu_hk_softc {
	device_t		 sc_dev;
	struct acpi_devnode	*sc_node;
	struct sysctllog	*sc_log;
	kmutex_t		 sc_mtx;
	uint16_t		 sc_caps;
#define	FUJITSU_HK_PSW_COUNT	4
	struct sysmon_pswitch	 sc_smpsw[FUJITSU_HK_PSW_COUNT];
	char			 sc_smpsw_name[FUJITSU_HK_PSW_COUNT][16];
};

/*
 * ACPI Fujitsu hotkeys controller capabilities (methods).
 */
#define FUJITSU_HK_CAP_GIRB	__BIT(0)
#define FUJITSU_HK_CAP_FUNC	__BIT(1)

/*
 * ACPI Fujitsu hotkeys controller HID.
 */
static const char * const fujitsu_hk_hid[] = {
	"FUJ02E3",
	NULL
};

static int	fujitsu_bp_match(device_t, cfdata_t, void *);
static void	fujitsu_bp_attach(device_t, device_t, void *);
static int	fujitsu_bp_detach(device_t, int);

static int	fujitsu_hk_match(device_t, cfdata_t, void *);
static void	fujitsu_hk_attach(device_t, device_t, void *);
static int	fujitsu_hk_detach(device_t, int);

CFATTACH_DECL_NEW(fujbp, sizeof(struct fujitsu_bp_softc),
    fujitsu_bp_match, fujitsu_bp_attach, fujitsu_bp_detach, NULL);

CFATTACH_DECL_NEW(fujhk, sizeof(struct fujitsu_hk_softc),
    fujitsu_hk_match, fujitsu_hk_attach, fujitsu_hk_detach, NULL);

static bool	fujitsu_bp_suspend(device_t, const pmf_qual_t *);
static bool	fujitsu_bp_resume(device_t, const pmf_qual_t *);

static bool	fujitsu_hk_suspend(device_t, const pmf_qual_t *);
static bool	fujitsu_hk_resume(device_t, const pmf_qual_t *);

static void	fujitsu_bp_brightness_up(device_t);
static void	fujitsu_bp_brightness_down(device_t);

static uint16_t	fujitsu_bp_capabilities(const struct acpi_devnode *);
static uint16_t	fujitsu_hk_capabilities(const struct acpi_devnode *);

static void	fujitsu_bp_notify_handler(ACPI_HANDLE, uint32_t, void *);
static void	fujitsu_hk_notify_handler(ACPI_HANDLE, uint32_t, void *);

static void	fujitsu_bp_event_callback(void *);
static void	fujitsu_hk_event_callback(void *);

static void	fujitsu_bp_sysctl_setup(struct fujitsu_bp_softc *);
static void	fujitsu_hk_sysctl_setup(struct fujitsu_hk_softc *);
static int	fujitsu_bp_sysctl_brightness(SYSCTLFN_PROTO);
static int	fujitsu_bp_sysctl_pointer(SYSCTLFN_PROTO);
static int	fujitsu_hk_sysctl_backlight(SYSCTLFN_PROTO);

static int	fujitsu_get_hks(const struct fujitsu_bp_softc *, uint32_t *);
static int	fujitsu_get_irb(const struct fujitsu_hk_softc *, uint32_t *);
static int	fujitsu_get_brightness_nlevels(const struct fujitsu_bp_softc *,
		    uint8_t *);
static int	fujitsu_get_brightness_level(const struct fujitsu_bp_softc *,
		    uint8_t *);
static int	fujitsu_set_brightness_level(const struct fujitsu_bp_softc *,
		    uint8_t);
static int	fujitsu_get_pointer_state(const struct fujitsu_bp_softc *,
		    bool *);
static int	fujitsu_set_pointer_state(const struct fujitsu_bp_softc *,
		    bool);
static int	fujitsu_get_backlight_state(const struct fujitsu_hk_softc *,
		    bool *);
static int	fujitsu_set_backlight_state(const struct fujitsu_hk_softc *,
		    bool);

static bool	fujitsu_has_method(ACPI_HANDLE, const char *, ACPI_OBJECT_TYPE);
static ACPI_STATUS
		fujitsu_eval_nary_integer(ACPI_HANDLE, const char *, const
		    ACPI_INTEGER *, uint8_t, ACPI_INTEGER *);

/*
 * Autoconfiguration for the fujbp driver.
 */

static int
fujitsu_bp_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, fujitsu_bp_hid);
}

static void
fujitsu_bp_attach(device_t parent, device_t self, void *aux)
{
	struct fujitsu_bp_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_devnode *ad = aa->aa_node;

	aprint_naive(": Fujitsu Brightness & Pointer\n");
	aprint_normal(": Fujitsu Brightness & Pointer\n");

	sc->sc_dev = self;
	sc->sc_node = ad;
	sc->sc_log = NULL;
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_caps = fujitsu_bp_capabilities(ad);

	if (fujitsu_get_brightness_nlevels(sc, &sc->sc_brightness_nlevels))
		sc->sc_brightness_nlevels = 0;

	(void)acpi_register_notify(sc->sc_node, fujitsu_bp_notify_handler);
	fujitsu_bp_sysctl_setup(sc);

	if (!pmf_device_register(self, fujitsu_bp_suspend, fujitsu_bp_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");

	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    fujitsu_bp_brightness_up, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    fujitsu_bp_brightness_down, true))
		aprint_error_dev(self, "couldn't register event handler\n");
}

static int
fujitsu_bp_detach(device_t self, int flags)
{
	struct fujitsu_bp_softc *sc = device_private(self);

	pmf_event_deregister(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    fujitsu_bp_brightness_down, true);
	pmf_event_deregister(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    fujitsu_bp_brightness_up, true);

	pmf_device_deregister(self);

	if (sc->sc_log != NULL)
		sysctl_teardown(&sc->sc_log);

	acpi_deregister_notify(sc->sc_node);

	mutex_destroy(&sc->sc_mtx);

	return 0;
}

/*
 * Autoconfiguration for the fujhk driver.
 */

static int
fujitsu_hk_match(device_t parent, cfdata_t match, void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, fujitsu_hk_hid);
}

static void
fujitsu_hk_attach(device_t parent, device_t self, void *aux)
{
	struct fujitsu_hk_softc *sc = device_private(self);
	struct acpi_attach_args *aa = aux;
	struct acpi_devnode *ad = aa->aa_node;
	int i;

	aprint_naive(": Fujitsu Hotkeys\n");
	aprint_normal(": Fujitsu Hotkeys\n");

	sc->sc_dev = self;
	sc->sc_node = ad;
	sc->sc_log = NULL;
	mutex_init(&sc->sc_mtx, MUTEX_DEFAULT, IPL_NONE);
	sc->sc_caps = fujitsu_hk_capabilities(ad);

	for (i = 0; i < FUJITSU_HK_PSW_COUNT; i++) {
		(void)snprintf(sc->sc_smpsw_name[i],
		    sizeof(sc->sc_smpsw_name[i]), "%s-%d",
		    device_xname(self), i);
		sc->sc_smpsw[i].smpsw_name = sc->sc_smpsw_name[i];
		sc->sc_smpsw[i].smpsw_type = PSWITCH_TYPE_HOTKEY;
		(void)sysmon_pswitch_register(&sc->sc_smpsw[i]);
	}

	(void)acpi_register_notify(sc->sc_node, fujitsu_hk_notify_handler);
	fujitsu_hk_sysctl_setup(sc);

	if (!pmf_device_register(self, fujitsu_hk_suspend, fujitsu_hk_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
fujitsu_hk_detach(device_t self, int flags)
{
	struct fujitsu_hk_softc *sc = device_private(self);
	int i;

	pmf_device_deregister(self);

	if (sc->sc_log != NULL)
		sysctl_teardown(&sc->sc_log);

	acpi_deregister_notify(sc->sc_node);

	for (i = 0; i < FUJITSU_HK_PSW_COUNT; i++) {
		sysmon_pswitch_unregister(&sc->sc_smpsw[i]);
	}

	mutex_destroy(&sc->sc_mtx);

	return 0;
}

/*
 * Power management.
 */

/*
 * On some LifeBook models, a call to the SMOU method is required to make the
 * internal pointer work after resume.  On the P7120, the internal pointer is
 * always enabled after resume.  If it was disabled before suspend, the BIOS
 * apparently believes that it is still disabled after resume.
 *
 * To prevent these problems, we disable the internal pointer on suspend and
 * enable it on resume.
 */
static bool
fujitsu_bp_suspend(device_t self, const pmf_qual_t *qual)
{
	struct fujitsu_bp_softc *sc = device_private(self);

	mutex_enter(&sc->sc_mtx);
	(void)fujitsu_set_pointer_state(sc, false);
	mutex_exit(&sc->sc_mtx);

	return true;
}

static bool
fujitsu_bp_resume(device_t self, const pmf_qual_t *qual)
{
	struct fujitsu_bp_softc *sc = device_private(self);

	mutex_enter(&sc->sc_mtx);
	(void)fujitsu_set_pointer_state(sc, true);
	mutex_exit(&sc->sc_mtx);

	return true;
}

/*
 * On the P7120, the backlight needs to be enabled after resume, since the
 * laptop wakes up with the backlight off (even if it was on before suspend).
 */
static bool
fujitsu_hk_suspend(device_t self, const pmf_qual_t *qual)
{
	struct fujitsu_hk_softc *sc = device_private(self);

	mutex_enter(&sc->sc_mtx);
	(void)fujitsu_set_backlight_state(sc, false);
	mutex_exit(&sc->sc_mtx);

	return true;
}

static bool
fujitsu_hk_resume(device_t self, const pmf_qual_t *qual)
{
	struct fujitsu_hk_softc *sc = device_private(self);

	mutex_enter(&sc->sc_mtx);
	(void)fujitsu_set_backlight_state(sc, true);
	mutex_exit(&sc->sc_mtx);

	return true;
}

static void
fujitsu_bp_brightness_up(device_t self)
{
	struct fujitsu_bp_softc *sc = device_private(self);
	uint8_t level;

	mutex_enter(&sc->sc_mtx);

	if (fujitsu_get_brightness_level(sc, &level) == 0 &&
	    level < (uint8_t)(sc->sc_brightness_nlevels - 1))
		(void)fujitsu_set_brightness_level(sc, level + 1);

	mutex_exit(&sc->sc_mtx);
}

static void
fujitsu_bp_brightness_down(device_t self)
{
	struct fujitsu_bp_softc *sc = device_private(self);
	uint8_t level;

	mutex_enter(&sc->sc_mtx);

	if (fujitsu_get_brightness_level(sc, &level) == 0 &&
	    level > 0)
		(void)fujitsu_set_brightness_level(sc, level - 1);

	mutex_exit(&sc->sc_mtx);
}

/*
 * Capabilities (available methods).
 */

static uint16_t
fujitsu_bp_capabilities(const struct acpi_devnode *ad)
{
	uint16_t caps;

	caps = 0;

	if (fujitsu_has_method(ad->ad_handle, "GHKS", ACPI_TYPE_INTEGER))
		caps |= FUJITSU_BP_CAP_GHKS;

	if (fujitsu_has_method(ad->ad_handle, "RBLL", ACPI_TYPE_INTEGER))
		caps |= FUJITSU_BP_CAP_RBLL;

	if (fujitsu_has_method(ad->ad_handle, "GBLL", ACPI_TYPE_INTEGER))
		caps |= FUJITSU_BP_CAP_GBLL;

	if (fujitsu_has_method(ad->ad_handle, "SBLL", ACPI_TYPE_METHOD))
		caps |= FUJITSU_BP_CAP_SBLL;

	if (fujitsu_has_method(ad->ad_handle, "GMOU", ACPI_TYPE_INTEGER))
		caps |= FUJITSU_BP_CAP_GMOU;

	if (fujitsu_has_method(ad->ad_handle, "SMOU", ACPI_TYPE_METHOD))
		caps |= FUJITSU_BP_CAP_SMOU;

	return caps;
}

static uint16_t
fujitsu_hk_capabilities(const struct acpi_devnode *ad)
{
	uint16_t caps;

	caps = 0;

	if (fujitsu_has_method(ad->ad_handle, "GIRB", ACPI_TYPE_INTEGER))
		caps |= FUJITSU_HK_CAP_GIRB;

	if (fujitsu_has_method(ad->ad_handle, "FUNC", ACPI_TYPE_METHOD))
		caps |= FUJITSU_HK_CAP_FUNC;

	return caps;
}

/*
 * ACPI notify handlers.
 */

static void
fujitsu_bp_notify_handler(ACPI_HANDLE handle, uint32_t notify, void *context)
{
	struct fujitsu_bp_softc *sc = device_private(context);

	switch (notify) {
	case FUJITSU_NOTIFY_Event:
		(void)AcpiOsExecute(OSL_NOTIFY_HANDLER,
		    fujitsu_bp_event_callback, sc);
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "unknown notify: 0x%"PRIx32"\n", notify);
	}
}

static void
fujitsu_hk_notify_handler(ACPI_HANDLE handle, uint32_t notify, void *context)
{
	struct fujitsu_hk_softc *sc = device_private(context);

	switch (notify) {
	case FUJITSU_NOTIFY_Event:
		(void)AcpiOsExecute(OSL_NOTIFY_HANDLER,
		    fujitsu_hk_event_callback, sc);
		break;
	default:
		aprint_error_dev(sc->sc_dev,
		    "unknown notify: 0x%"PRIx32"\n", notify);
	}
}

/*
 * ACPI notify callbacks.
 */

static void
fujitsu_bp_event_callback(void *arg)
{
	struct fujitsu_bp_softc *sc = arg;
	int error;
	uint32_t hks;
	uint8_t level;
	bool state;

	if (fujitsu_get_hks(sc, &hks))
		return;

	if (hks & FUJITSU_HKS_BRIGHTNESS) {
		mutex_enter(&sc->sc_mtx);
		error = fujitsu_get_brightness_level(sc, &level);
		mutex_exit(&sc->sc_mtx);
		if (!error)
			aprint_verbose_dev(sc->sc_dev,
			    "brightness level is now: %"PRIu8"\n", level);
	}

	if (hks & FUJITSU_HKS_POINTER) {
		mutex_enter(&sc->sc_mtx);
		error = fujitsu_get_pointer_state(sc, &state);
		mutex_exit(&sc->sc_mtx);
		if (!error)
			aprint_verbose_dev(sc->sc_dev,
			    "internal pointer is now: %s\n",
			    state ? "enabled" : "disabled");
	}
}

static void
fujitsu_hk_event_callback(void *arg)
{
	struct fujitsu_hk_softc *sc = arg;
	const int max_irb_buffer_size = 100;
	uint32_t irb;
	int i, index;

	for (i = 0; i < max_irb_buffer_size; i++) {
		if (fujitsu_get_irb(sc, &irb) || irb == 0)
			return;

		switch (irb & ~FUJITSU_MODIFICATION_MASK) {
		case FUJITSU_IRB_HOTKEY_RELEASE:
			/* Hotkey button release event (nothing to do). */
			break;
		case FUJITSU_IRB_HOTKEY_PRESS(0):
		case FUJITSU_IRB_HOTKEY_PRESS(1):
		case FUJITSU_IRB_HOTKEY_PRESS(2):
		case FUJITSU_IRB_HOTKEY_PRESS(3):
			/* Hotkey button press event. */
			index = FUJITSU_IRB_HOTKEY_INDEX(irb);
			sysmon_pswitch_event(&sc->sc_smpsw[index],
			    PSWITCH_EVENT_PRESSED);
			break;
		default:
			aprint_error_dev(sc->sc_dev,
			    "unknown GIRB result: 0x%"PRIx32"\n", irb);
			break;
		}
	}
}

/*
 * Sysctl setup.
 */

static void
fujitsu_bp_sysctl_setup(struct fujitsu_bp_softc *sc)
{
	const struct sysctlnode *rnode;
	int access;
	uint8_t dummy_level;
	bool dummy_state;
	bool brightness, pointer;

	brightness = (fujitsu_get_brightness_level(sc, &dummy_level) == 0);
	pointer = (fujitsu_get_pointer_state(sc, &dummy_state) == 0);

	if (brightness || pointer) {
		if ((sysctl_createv(&sc->sc_log, 0, NULL, &rnode,
		    0, CTLTYPE_NODE, "hw", NULL,
		    NULL, 0, NULL, 0,
		    CTL_HW, CTL_EOL)) != 0)
			goto fail;

		if ((sysctl_createv(&sc->sc_log, 0, &rnode, &rnode,
		    0, CTLTYPE_NODE, "acpi", NULL,
		    NULL, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL)) != 0)
			goto fail;

		if ((sysctl_createv(&sc->sc_log, 0, &rnode, &rnode,
		    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
		    SYSCTL_DESCR("Fujitsu brightness & pointer controls"),
		    NULL, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL)) != 0)
			goto fail;
	}

	if (brightness) {
		if (sc->sc_caps & FUJITSU_BP_CAP_SBLL)
			access = CTLFLAG_READWRITE;
		else
			access = CTLFLAG_READONLY;

		(void)sysctl_createv(&sc->sc_log, 0, &rnode, NULL,
		    access, CTLTYPE_INT, "brightness",
		    SYSCTL_DESCR("Internal DFP brightness level"),
		    fujitsu_bp_sysctl_brightness, 0, sc, 0,
		    CTL_CREATE, CTL_EOL);
	}

	if (pointer) {
		if (sc->sc_caps & FUJITSU_BP_CAP_SMOU)
			access = CTLFLAG_READWRITE;
		else
			access = CTLFLAG_READONLY;

		(void)sysctl_createv(&sc->sc_log, 0, &rnode, NULL,
		    access, CTLTYPE_BOOL, "pointer",
		    SYSCTL_DESCR("Internal pointer switch state"),
		    fujitsu_bp_sysctl_pointer, 0, sc, 0,
		    CTL_CREATE, CTL_EOL);
	}

	return;

 fail:
	aprint_error_dev(sc->sc_dev, "couldn't add sysctl nodes\n");
}

static void
fujitsu_hk_sysctl_setup(struct fujitsu_hk_softc *sc)
{
	const struct sysctlnode *rnode;
	bool dummy_state;

	if (fujitsu_get_backlight_state(sc, &dummy_state) == 0) {
		if ((sysctl_createv(&sc->sc_log, 0, NULL, &rnode,
		    0, CTLTYPE_NODE, "hw", NULL,
		    NULL, 0, NULL, 0,
		    CTL_HW, CTL_EOL)) != 0)
			goto fail;

		if ((sysctl_createv(&sc->sc_log, 0, &rnode, &rnode,
		    0, CTLTYPE_NODE, "acpi", NULL,
		    NULL, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL)) != 0)
			goto fail;

		if ((sysctl_createv(&sc->sc_log, 0, &rnode, &rnode,
		    0, CTLTYPE_NODE, device_xname(sc->sc_dev),
		    SYSCTL_DESCR("Fujitsu hotkeys controls"),
		    NULL, 0, NULL, 0,
		    CTL_CREATE, CTL_EOL)) != 0)
			goto fail;

		(void)sysctl_createv(&sc->sc_log, 0, &rnode, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_BOOL, "backlight",
		    SYSCTL_DESCR("Internal DFP backlight switch state"),
		    fujitsu_hk_sysctl_backlight, 0, sc, 0,
		    CTL_CREATE, CTL_EOL);
	}

	return;

 fail:
	aprint_error_dev(sc->sc_dev, "couldn't add sysctl nodes\n");
}

/*
 * Sysctl callbacks.
 */

static int
fujitsu_bp_sysctl_brightness(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct fujitsu_bp_softc *sc;
	int val, error;
	uint8_t level;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_mtx);
	error = fujitsu_get_brightness_level(sc, &level);
	val = (int)level;
	mutex_exit(&sc->sc_mtx);

	if (error)
		return error;

	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (val < 0 || val > (uint8_t)(sc->sc_brightness_nlevels - 1))
		return EINVAL;

	mutex_enter(&sc->sc_mtx);
	error = fujitsu_set_brightness_level(sc, (uint8_t)val);
	mutex_exit(&sc->sc_mtx);

	return error;
}

static int
fujitsu_bp_sysctl_pointer(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct fujitsu_bp_softc *sc;
	bool val;
	int error;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_mtx);
	error = fujitsu_get_pointer_state(sc, &val);
	mutex_exit(&sc->sc_mtx);

	if (error)
		return error;

	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(&sc->sc_mtx);
	error = fujitsu_set_pointer_state(sc, val);
	mutex_exit(&sc->sc_mtx);

	return error;
}

static int
fujitsu_hk_sysctl_backlight(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct fujitsu_hk_softc *sc;
	bool val;
	int error;

	node = *rnode;
	sc = node.sysctl_data;

	mutex_enter(&sc->sc_mtx);
	error = fujitsu_get_backlight_state(sc, &val);
	mutex_exit(&sc->sc_mtx);

	if (error)
		return error;

	node.sysctl_data = &val;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	mutex_enter(&sc->sc_mtx);
	error = fujitsu_set_backlight_state(sc, val);
	mutex_exit(&sc->sc_mtx);

	return error;
}

/*
 * Evaluation of ACPI Fujitsu methods.
 */

static int
fujitsu_get_hks(const struct fujitsu_bp_softc *sc, uint32_t *valuep)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_BP_CAP_GHKS))
		return ENODEV;

	rv = acpi_eval_integer(hdl, "GHKS", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "GHKS", AcpiFormatException(rv));
		return EIO;
	}

	*valuep = (uint32_t)val;

	return 0;
}

static int
fujitsu_get_irb(const struct fujitsu_hk_softc *sc, uint32_t *valuep)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_HK_CAP_GIRB))
		return ENODEV;

	rv = acpi_eval_integer(hdl, "GIRB", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "GIRB", AcpiFormatException(rv));
		return EIO;
	}

	*valuep = (uint32_t)val;

	return 0;
}

static int
fujitsu_get_brightness_nlevels(const struct fujitsu_bp_softc *sc, uint8_t *valuep)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_BP_CAP_RBLL))
		return ENODEV;

	rv = acpi_eval_integer(hdl, "RBLL", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "RBLL", AcpiFormatException(rv));
		return EIO;
	}

	if (val > UINT8_MAX)
		return ERANGE;

	*valuep = (uint8_t)val;

	return 0;
}

static int
fujitsu_get_brightness_level(const struct fujitsu_bp_softc *sc, uint8_t *valuep)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_BP_CAP_GBLL))
		return ENODEV;

	rv = acpi_eval_integer(hdl, "GBLL", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "GBLL", AcpiFormatException(rv));
		return EIO;
	}

	/* Clear modification bits. */
	val &= ~FUJITSU_MODIFICATION_MASK;

	if (val > UINT8_MAX)
		return ERANGE;

	*valuep = (uint8_t)val;

	return 0;
}

static int
fujitsu_set_brightness_level(const struct fujitsu_bp_softc *sc, uint8_t value)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_BP_CAP_SBLL))
		return ENODEV;

	val = (ACPI_INTEGER)value;
	rv = acpi_eval_set_integer(hdl, "SBLL", val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "SBLL", AcpiFormatException(rv));
		return EIO;
	}

	return 0;
}

static int
fujitsu_get_pointer_state(const struct fujitsu_bp_softc *sc, bool *valuep)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_BP_CAP_GMOU))
		return ENODEV;

	rv = acpi_eval_integer(hdl, "GMOU", &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "GMOU", AcpiFormatException(rv));
		return EIO;
	}

	/* Clear modification bits. */
	val &= ~FUJITSU_MODIFICATION_MASK;

	if (val > 1)
		return ERANGE;

	*valuep = (bool)val;

	return 0;
}

static int
fujitsu_set_pointer_state(const struct fujitsu_bp_softc *sc, bool value)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_BP_CAP_SMOU))
		return ENODEV;

	val = (ACPI_INTEGER)value;
	rv = acpi_eval_set_integer(hdl, "SMOU", val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "SMOU", AcpiFormatException(rv));
		return EIO;
	}

	return 0;
}

static int
fujitsu_get_backlight_state(const struct fujitsu_hk_softc *sc, bool *valuep)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER args[] = {
		FUJITSU_FUNC_TARGET_BACKLIGHT,
		FUJITSU_FUNC_COMMAND_GET,
		0x4,
		0x0
	};
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_HK_CAP_FUNC))
		return ENODEV;

	rv = fujitsu_eval_nary_integer(hdl, "FUNC", args, 4, &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "FUNC", AcpiFormatException(rv));
		return EIO;
	}

	if (val == FUJITSU_FUNC_INVALID_ARGS)
		return ENODEV;

	if (val == FUJITSU_FUNC_BACKLIGHT_ON)
		*valuep = true;
	else if (val == FUJITSU_FUNC_BACKLIGHT_OFF)
		*valuep = false;
	else
		return ERANGE;

	return 0;
}

static int
fujitsu_set_backlight_state(const struct fujitsu_hk_softc *sc, bool value)
{
	ACPI_HANDLE hdl = sc->sc_node->ad_handle;
	ACPI_INTEGER args[] = {
		FUJITSU_FUNC_TARGET_BACKLIGHT,
		FUJITSU_FUNC_COMMAND_SET,
		0x4,
		0x0
	};
	ACPI_INTEGER val;
	ACPI_STATUS rv;

	if (!(sc->sc_caps & FUJITSU_HK_CAP_FUNC))
		return ENODEV;

	if (value)
		args[3] = FUJITSU_FUNC_BACKLIGHT_ON;
	else
		args[3] = FUJITSU_FUNC_BACKLIGHT_OFF;

	rv = fujitsu_eval_nary_integer(hdl, "FUNC", args, 4, &val);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(sc->sc_dev, "failed to evaluate %s.%s: %s\n",
		    acpi_name(hdl), "FUNC", AcpiFormatException(rv));
		return EIO;
	}

	if (val == FUJITSU_FUNC_INVALID_ARGS)
		return ENODEV;

	return 0;
}

/*
 * General-purpose utility functions.
 */

/*
 * acpidisp_has_method:
 *
 *	Returns true if and only if (a) the object handle.path exists and
 *	(b) this object is a method or has the given type.
 */
static bool
fujitsu_has_method(ACPI_HANDLE handle, const char *path, ACPI_OBJECT_TYPE type)
{
	ACPI_HANDLE hdl;
	ACPI_OBJECT_TYPE typ;

	KASSERT(handle != NULL);

	if (ACPI_FAILURE(AcpiGetHandle(handle, path, &hdl)))
		return false;

	if (ACPI_FAILURE(AcpiGetType(hdl, &typ)))
		return false;

	if (typ != ACPI_TYPE_METHOD && typ != type)
		return false;

	return true;
}

/*
 * fujitsu_eval_nary_integer:
 *
 *	Evaluate an object that takes as input an arbitrary (possible null)
 *	number of integer parameters.  If res is not NULL, then *res is filled
 *	with the result of the evaluation, and AE_NULL_OBJECT is returned if
 *	the evaluation produced no result.
 */
static ACPI_STATUS
fujitsu_eval_nary_integer(ACPI_HANDLE handle, const char *path, const
    ACPI_INTEGER *args, uint8_t count, ACPI_INTEGER *res)
{
	ACPI_OBJECT_LIST paramlist;
	ACPI_OBJECT retobj, objpool[4], *argobjs;
	ACPI_BUFFER buf;
	ACPI_STATUS rv;
	uint8_t i;

	/* Require that (args == NULL) if and only if (count == 0). */
	KASSERT((args != NULL || count == 0) && (args == NULL || count != 0));

	/* The object pool should be large enough for our callers. */
	KASSERT(count <= __arraycount(objpool));

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	/* Convert the given array args into an array of ACPI objects. */
	argobjs = objpool;
	for (i = 0; i < count; i++) {
		argobjs[i].Type = ACPI_TYPE_INTEGER;
		argobjs[i].Integer.Value = args[i];
	}

	paramlist.Count = count;
	paramlist.Pointer = argobjs;

	(void)memset(&retobj, 0, sizeof(retobj));
	buf.Pointer = &retobj;
	buf.Length = sizeof(retobj);

	rv = AcpiEvaluateObject(handle, path, &paramlist, &buf);

	if (ACPI_FAILURE(rv))
		return rv;

	/*
	 * If a return value is expected and desired (i.e. res != NULL),
	 * then copy the result into *res.
	 */
	if (res != NULL) {
		if (buf.Length == 0)
			return AE_NULL_OBJECT;

		if (retobj.Type != ACPI_TYPE_INTEGER)
			return AE_TYPE;

		*res = retobj.Integer.Value;
	}

	return AE_OK;
}
