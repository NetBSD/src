/* $NetBSD: ims.c,v 1.1 2017/12/10 17:05:54 bouyer Exp $ */
/* $OpenBSD ims.c,v 1.1 2016/01/12 01:11:15 jcs Exp $ */

/*
 * HID-over-i2c mouse/trackpad driver
 *
 * Copyright (c) 2015, 2016 joshua stein <jcs@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ims.c,v 1.1 2017/12/10 17:05:54 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/ioctl.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/ihidev.h>

#include <dev/hid/hid.h>
#include <dev/hid/hidms.h>

struct ims_softc {
	struct ihidev	sc_hdev;
	struct hidms	sc_ms;
	bool		sc_enabled;
};

static void	ims_intr(struct ihidev *addr, void *ibuf, u_int len);

static int	ims_enable(void *);
static void	ims_disable(void *);
static int	ims_ioctl(void *, u_long, void *, int, struct lwp *);

const struct wsmouse_accessops ims_accessops = {
	ims_enable,
	ims_ioctl,
	ims_disable,
};

static int	ims_match(device_t, cfdata_t, void *);
static void	ims_attach(device_t, device_t, void *);
static int	ims_detach(device_t, int);
static void	ims_childdet(device_t, device_t);

CFATTACH_DECL2_NEW(ims, sizeof(struct ims_softc), ims_match, ims_attach,
    ims_detach, NULL, NULL, ims_childdet);

static int
ims_match(device_t parent, cfdata_t match, void *aux)
{
	struct ihidev_attach_arg *iha = (struct ihidev_attach_arg *)aux;
	int size;
	void *desc;

	ihidev_get_report_desc(iha->parent, &desc, &size);

	if (hid_is_collection(desc, size, iha->reportid,
	    HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_POINTER)))
		return (IMATCH_IFACECLASS);

	if (hid_is_collection(desc, size, iha->reportid,
	    HID_USAGE2(HUP_GENERIC_DESKTOP, HUG_MOUSE)))
		return (IMATCH_IFACECLASS);

	if (hid_is_collection(desc, size, iha->reportid,
	    HID_USAGE2(HUP_DIGITIZERS, HUD_PEN)))
		return (IMATCH_IFACECLASS);

	return (IMATCH_NONE);
}

static void
ims_attach(device_t parent, device_t self, void *aux)
{
	struct ims_softc *sc = device_private(self);
	struct hidms *ms = &sc->sc_ms;
	struct ihidev_attach_arg *iha = (struct ihidev_attach_arg *)aux;
	int size, repid;
	void *desc;

	sc->sc_hdev.sc_idev = self;
	sc->sc_hdev.sc_intr = ims_intr;
	sc->sc_hdev.sc_parent = iha->parent;
	sc->sc_hdev.sc_report_id = iha->reportid;

	if (!pmf_device_register(self, NULL, NULL))
		aprint_error_dev(self, "couldn't establish power handler\n");

	ihidev_get_report_desc(iha->parent, &desc, &size);
	repid = iha->reportid;
	sc->sc_hdev.sc_isize = hid_report_size(desc, size, hid_input, repid);
	sc->sc_hdev.sc_osize = hid_report_size(desc, size, hid_output, repid);
	sc->sc_hdev.sc_fsize = hid_report_size(desc, size, hid_feature, repid);

	if (!hidms_setup(self, ms, iha->reportid, desc, size) != 0)
		return;

	hidms_attach(self, ms, &ims_accessops);
}

static int
ims_detach(device_t self, int flags)
{
	struct ims_softc *sc = device_private(self);
	int rv = 0;

	/* No need to do reference counting of ums, wsmouse has all the goo. */
	if (sc->sc_ms.hidms_wsmousedev != NULL)
		rv = config_detach(sc->sc_ms.hidms_wsmousedev, flags);

	pmf_device_deregister(self);

	return rv;
}

void 
ims_childdet(device_t self, device_t child)
{
	struct ims_softc *sc = device_private(self);

	KASSERT(sc->sc_ms.hidms_wsmousedev == child);
	sc->sc_ms.hidms_wsmousedev = NULL;
}


static void
ims_intr(struct ihidev *addr, void *buf, u_int len)
{
	struct ims_softc *sc = (struct ims_softc *)addr;
	struct hidms *ms = &sc->sc_ms;

	if (sc->sc_enabled)
		hidms_intr(ms, buf, len);
}

static int
ims_enable(void *v)
{
	struct ims_softc *sc = v;
	int error;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_enabled = 1;
	sc->sc_ms.hidms_buttons = 0;

	error = ihidev_open(&sc->sc_hdev);
	if (error)
		sc->sc_enabled = 0;
	return error;
}

static void
ims_disable(void *v)
{
	struct ims_softc *sc = v;

#ifdef DIAGNOSTIC
	if (!sc->sc_enabled) {
		printf("ums_disable: not enabled\n");
		return;
	}
#endif

	if (sc->sc_enabled) {
		sc->sc_enabled = 0;
		ihidev_close(&sc->sc_hdev);
	}

}

static int
ims_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct ims_softc *sc = v;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		if (sc->sc_ms.flags & HIDMS_ABS) {
			*(u_int *)data = WSMOUSE_TYPE_TPANEL;
		} else {
			/* XXX: should we set something else? */
			*(u_int *)data = WSMOUSE_TYPE_USB;
		}
		return 0;
	}
	return EPASSTHROUGH;
}
