/*	$NetBSD: sony_acpi.c,v 1.5.26.2 2007/08/05 18:59:19 jmcneill Exp $	*/

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: sony_acpi.c,v 1.5.26.2 2007/08/05 18:59:19 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/callout.h>
#include <sys/sysctl.h>

#include <machine/bus.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>

#define SONY_NOTIFY_FnKeyEvent			0x92
#define SONY_NOTIFY_BrightnessDownPressed	0x85
#define SONY_NOTIFY_BrightnessDownReleased	0x05
#define SONY_NOTIFY_BrightnessUpPressed		0x86
#define SONY_NOTIFY_BrightnessUpReleased	0x06
#define SONY_NOTIFY_DisplaySwitchPressed	0x87
#define SONY_NOTIFY_DisplaySwitchReleased	0x07
#define SONY_NOTIFY_ZoomPressed			0x8a
#define SONY_NOTIFY_ZoomReleased		0x0a
#define SONY_NOTIFY_SuspendPressed		0x8c
#define SONY_NOTIFY_SuspendReleased		0x0c

struct sony_acpi_softc {
        struct device sc_dev;
	struct sysctllog *sc_log;
	struct acpi_devnode *sc_node;

	struct device *sc_wskbddev;

	struct sony_acpi_pmstate {
		ACPI_INTEGER	brt;
	} sc_pmstate;
};

static int	sony_acpi_wskbd_enable(void *, int);
static void	sony_acpi_wskbd_set_leds(void *, int);
static int	sony_acpi_wskbd_ioctl(void *, u_long, void *, int, struct lwp *);

static const struct wskbd_accessops sony_acpi_accessops = {
	sony_acpi_wskbd_enable,
	sony_acpi_wskbd_set_leds,
	sony_acpi_wskbd_ioctl,
};

extern const struct wscons_keydesc ukbd_keydesctab[];

static const struct wskbd_mapdata sony_acpi_keymapdata = {
	ukbd_keydesctab,
	KB_US,
};

static const char * const sony_acpi_ids[] = {
	"SNY5001",
	NULL
};

#define SONY_ACPI_QUIRK_FNINIT	0x01

static const struct sony_acpi_quirk_table {
	const char *	product_name;
	int		quirks;
} sony_acpi_quirks[] = {
	{ "VGN-N250E",	SONY_ACPI_QUIRK_FNINIT },
	{ NULL, -1 }
};

static int	sony_acpi_match(struct device *, struct cfdata *, void *);
static void	sony_acpi_attach(struct device *, struct device *, void *);
static pnp_status_t sony_acpi_power(device_t, pnp_request_t, void *);
static ACPI_STATUS sony_acpi_eval_set_integer(ACPI_HANDLE, const char *,
    ACPI_INTEGER, ACPI_INTEGER *);
static void	sony_acpi_quirk_setup(struct sony_acpi_softc *);
static void	sony_acpi_notify_handler(ACPI_HANDLE, UINT32, void *);

CFATTACH_DECL(sony_acpi, sizeof(struct sony_acpi_softc),
    sony_acpi_match, sony_acpi_attach, NULL, NULL);

static int
sony_acpi_match(struct device *parent, struct cfdata *match,
    void *aux)
{
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;

	return acpi_match_hid(aa->aa_node->ad_devinfo, sony_acpi_ids);
}

static int
sony_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	ACPI_INTEGER acpi_val;
	ACPI_STATUS rv;
	int val, old_val, error;
	char buf[SYSCTL_NAMELEN + 1], *ptr;
	struct sony_acpi_softc *sc = rnode->sysctl_data;

	(void)snprintf(buf, sizeof(buf), "G%s", rnode->sysctl_name);
	for (ptr = buf; *ptr; ptr++)
		*ptr = toupper(*ptr);

	rv = acpi_eval_integer(sc->sc_node->ad_handle, buf, &acpi_val);
	if (ACPI_FAILURE(rv)) {
#ifdef DIAGNOSTIC
		printf("%s: couldn't get `%s'\n", device_xname(&sc->sc_dev), buf);
#endif
		return EIO;
	}
	val = old_val = acpi_val;

	node = *rnode;
	node.sysctl_data = &val;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	(void)snprintf(buf, sizeof(buf), "S%s", rnode->sysctl_name);
	acpi_val = val;
	rv = sony_acpi_eval_set_integer(sc->sc_node->ad_handle, buf,
	    acpi_val, NULL);
	if (ACPI_FAILURE(rv)) {
#ifdef DIAGNOSTIC
		printf("%s: couldn't set `%s' to %d\n",
		    device_xname(&sc->sc_dev), buf, val);
#endif
		return EIO;
	}
	return 0;
}

static ACPI_STATUS
sony_walk_cb(ACPI_HANDLE hnd, UINT32 v, void *context,
    void **status)
{
	struct sony_acpi_softc *sc = (void*)context;
	const struct sysctlnode *node, *snode;
	const char *name = acpi_name(hnd);
	ACPI_INTEGER acpi_val;
	char buf[SYSCTL_NAMELEN + 1], *ptr;
	int rv;

	if ((name = strrchr(name, '.')) == NULL)
		return AE_OK;

	name++;
	if ((*name != 'G') && (*name != 'S'))
		return AE_OK;

	(void)strlcpy(buf, name, sizeof(buf));
	*buf = 'G';

	/*
	 * We assume that if the 'get' of the name as an integer is
	 * successful it is ok.
	 */
	if (acpi_eval_integer(sc->sc_node->ad_handle, buf, &acpi_val))
		return AE_OK;

	for (ptr = buf; *ptr; ptr++)
		*ptr = tolower(*ptr);

	if ((rv = sysctl_createv(&sc->sc_log, 0, NULL, &node, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "hw", NULL, NULL, 0, NULL, 0, CTL_HW, CTL_EOL)) != 0)
		goto out;

	if ((rv = sysctl_createv(&sc->sc_log, 0, &node, &snode, 0,
	    CTLTYPE_NODE, device_xname(&sc->sc_dev),
	    SYSCTL_DESCR("sony controls"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

	if ((rv = sysctl_createv(&sc->sc_log, 0, &snode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, buf + 1, NULL,
	    sony_sysctl_helper, 0, sc, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto out;

out:
#ifdef DIAGNOSTIC
	if (rv)
		printf("%s: sysctl_createv failed (rv = %d)\n",
		    device_xname(&sc->sc_dev), rv);
#endif
	return AE_OK;
}

ACPI_STATUS
sony_acpi_eval_set_integer(ACPI_HANDLE handle, const char *path,
    ACPI_INTEGER val, ACPI_INTEGER *valp)
{
	ACPI_STATUS rv;
	ACPI_BUFFER buf;
	ACPI_OBJECT param, ret_val;
	ACPI_OBJECT_LIST params;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	params.Count = 1;
	params.Pointer = &param;

	param.Type = ACPI_TYPE_INTEGER;
	param.Integer.Value = val;

	buf.Pointer = &ret_val;
	buf.Length = sizeof(ret_val);

	rv = AcpiEvaluateObjectTyped(handle, path, &params, &buf,
	    ACPI_TYPE_INTEGER);

	if (ACPI_SUCCESS(rv) && valp)
		*valp = ret_val.Integer.Value;

	return rv;
}

static void
sony_acpi_attach(struct device *parent, struct device *self, void *aux)
{
	struct sony_acpi_softc *sc = (void *)self;
	struct acpi_attach_args *aa = aux;
	struct wskbddev_attach_args wska;
	ACPI_STATUS rv;

	aprint_naive(": Sony Miscellaneous Controller\n");
	aprint_normal(": Sony Miscellaneous Controller\n");

	sc->sc_node = aa->aa_node;

	sony_acpi_quirk_setup(sc);

	/* Install notify handler */
	rv = AcpiInstallNotifyHandler(sc->sc_node->ad_handle,
	    ACPI_DEVICE_NOTIFY, sony_acpi_notify_handler, sc);
	if (ACPI_FAILURE(rv))
		aprint_error("%s: couldn't install notify handler (%d)\n",
		    device_xname(self), rv);

	/* Install sysctl handler */
	rv = AcpiWalkNamespace(ACPI_TYPE_METHOD,
	    sc->sc_node->ad_handle, 1, sony_walk_cb, sc, NULL);
#ifdef DIAGNOSTIC
	if (ACPI_FAILURE(rv))
		aprint_error("%s: Cannot walk ACPI namespace (%d)\n",
		    device_xname(self), rv);
#endif

	if (pnp_register(self, sony_acpi_power) != PNP_STATUS_SUCCESS)
		aprint_error("%s: couldn't establish power handler\n",
		    device_xname(self));

	wska.console = 0;
	wska.keymap = &sony_acpi_keymapdata;
	wska.accessops = &sony_acpi_accessops;
	wska.accesscookie = sc;

	sc->sc_wskbddev = config_found(self, &wska, wskbddevprint);
}

static void
sony_acpi_quirk_setup(struct sony_acpi_softc *sc)
{
	const char *product_name;
	ACPI_HANDLE hdl;
	int i;

	hdl = sc->sc_node->ad_handle;

	product_name = pnp_get_platform("system-product-name");
	if (product_name == NULL)
		return;

	for (i = 0; sony_acpi_quirks[i].product_name != NULL; i++)
		if (strcmp(sony_acpi_quirks[i].product_name, product_name) == 0)
			break;

	if (sony_acpi_quirks[i].product_name == NULL)
		return;

	if (sony_acpi_quirks[i].quirks & SONY_ACPI_QUIRK_FNINIT) {
		/* Initialize extra Fn keys */
		sony_acpi_eval_set_integer(hdl, "SN02", 0x04, NULL);
		sony_acpi_eval_set_integer(hdl, "SN07", 0x02, NULL);
		sony_acpi_eval_set_integer(hdl, "SN02", 0x10, NULL);
		sony_acpi_eval_set_integer(hdl, "SN07", 0x00, NULL);
		sony_acpi_eval_set_integer(hdl, "SN03", 0x02, NULL);
		sony_acpi_eval_set_integer(hdl, "SN07", 0x101, NULL);
	}

	return;
}

static void
sony_acpi_notify_handler(ACPI_HANDLE hdl, UINT32 notify, void *opaque)
{
	struct sony_acpi_softc *sc;
	ACPI_STATUS rv;
	ACPI_INTEGER arg;
	int s;

	sc = (struct sony_acpi_softc *)opaque;

	if (sc->sc_wskbddev == NULL)
		return;

	if (notify == SONY_NOTIFY_FnKeyEvent) {
		rv = sony_acpi_eval_set_integer(hdl, "SN07", 0x202, &arg);
		if (ACPI_FAILURE(rv))
			return;

		notify = arg & 0xff;
	}

	s = spltty();
	switch (notify) {
	case SONY_NOTIFY_BrightnessDownPressed:
		rv = acpi_eval_integer(sc->sc_node->ad_handle, "GBRT", &arg);
		if (ACPI_FAILURE(rv) || arg == 0)
			break;
		arg--;
		sony_acpi_eval_set_integer(hdl, "SBRT", arg, NULL);
		break;
	case SONY_NOTIFY_BrightnessUpPressed:
		rv = acpi_eval_integer(sc->sc_node->ad_handle, "GBRT", &arg);
		if (ACPI_FAILURE(rv) || arg > 8)
			break;
		arg++;
		sony_acpi_eval_set_integer(hdl, "SBRT", arg, NULL);
		break;
	case SONY_NOTIFY_BrightnessDownReleased:
	case SONY_NOTIFY_BrightnessUpReleased:
		break;
	case SONY_NOTIFY_DisplaySwitchPressed:
	case SONY_NOTIFY_DisplaySwitchReleased:
	case SONY_NOTIFY_ZoomPressed:
	case SONY_NOTIFY_ZoomReleased:
	case SONY_NOTIFY_SuspendPressed:
	case SONY_NOTIFY_SuspendReleased:
		wskbd_input(sc->sc_wskbddev,
		    notify & 0x80 ? WSCONS_EVENT_KEY_UP : WSCONS_EVENT_KEY_DOWN,
		    notify);
		break;
	default:
		printf("%s: unknown notify event 0x%x\n",
		    device_xname(&sc->sc_dev), notify);
		break;
	}
	splx(s);

	return;
}

static pnp_status_t
sony_acpi_power(device_t dv, pnp_request_t req, void *opaque)
{
	struct sony_acpi_softc *sc;
	pnp_capabilities_t *pcaps;
	pnp_state_t *pstate;

	sc = (struct sony_acpi_softc *)dv;

	switch (req) {
	case PNP_REQUEST_GET_CAPABILITIES:
		pcaps = opaque;
		pcaps->state = PNP_STATE_D0 | PNP_STATE_D3;
		break;
	case PNP_REQUEST_GET_STATE:
		pstate = opaque;
		*pstate = PNP_STATE_D0; /* XXX */
		break;
	case PNP_REQUEST_SET_STATE:
		pstate = opaque;
		switch (*pstate) {
		case PNP_STATE_D0:
			sony_acpi_eval_set_integer(sc->sc_node->ad_handle,
			    "SBRT", sc->sc_pmstate.brt, NULL);
			sony_acpi_quirk_setup(sc);
			break;
		case PNP_STATE_D3:
			acpi_eval_integer(sc->sc_node->ad_handle, "GBRT",
			    &sc->sc_pmstate.brt);
			break;
		default:
			return PNP_STATUS_UNSUPPORTED;
		}
		break;
	default:
		return PNP_STATUS_UNSUPPORTED;
	}

	return PNP_STATUS_SUCCESS;
}

static int
sony_acpi_wskbd_enable(void *opaque, int on)
{

	return 0;
}

static void
sony_acpi_wskbd_set_leds(void *opaque, int leds)
{

	return;
}

static int
sony_acpi_wskbd_ioctl(void *opaque, u_long cmd, void *data, int flags,
    struct lwp *l)
{

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = WSKBD_TYPE_USB; /* close enough */
		return 0;
	default:
		break;
	}

	return EPASSTHROUGH;
}
