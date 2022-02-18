/*	$NetBSD: drmfb.c,v 1.14 2022/02/18 18:31:18 wiz Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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
 * drmfb: wsdisplay support, via genfb, for any drm device.  Use this
 * if you're too lazy to write a hardware-accelerated framebuffer using
 * wsdisplay directly.
 *
 * This doesn't actually do anything interesting -- just hooks up
 * drmkms crap and genfb crap.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drmfb.c,v 1.14 2022/02/18 18:31:18 wiz Exp $");

#ifdef _KERNEL_OPT
#include "vga.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kauth.h>

#if NVGA > 0
/*
 * XXX All we really need is vga_is_console from vgavar.h, but the
 * header files are missing their own dependencies, so we need to
 * explicitly drag in the other crap.
 */
#include <dev/ic/mc6845reg.h>
#include <dev/ic/pcdisplayvar.h>
#include <dev/ic/vgareg.h>
#include <dev/ic/vgavar.h>
#endif

#include <dev/wsfb/genfbvar.h>

#include <drm/drm_device.h>
#include <drm/drm_fb_helper.h>
#include <drm/drmfb.h>

static int	drmfb_genfb_ioctl(void *, void *, unsigned long, void *, int,
		    struct lwp *);
static paddr_t	drmfb_genfb_mmap(void *, void *, off_t, int);
static int	drmfb_genfb_enable_polling(void *);
static int	drmfb_genfb_disable_polling(void *);
static bool	drmfb_genfb_setmode(struct genfb_softc *, int);

static const struct genfb_mode_callback drmfb_genfb_mode_callback = {
	.gmc_setmode = drmfb_genfb_setmode,
};

int
drmfb_attach(struct drmfb_softc *sc, const struct drmfb_attach_args *da)
{
	const struct drm_fb_helper_surface_size *const sizes = da->da_fb_sizes;
	struct drm_connector_list_iter conn_iter;
	struct drm_connector *connector;
	const prop_dictionary_t dict = device_properties(da->da_dev);
#if NVGA > 0
	struct drm_device *const dev = da->da_fb_helper->dev;
#endif
	static const struct genfb_ops zero_genfb_ops;
	struct genfb_ops genfb_ops = zero_genfb_ops;
	enum { CONS_VGA, CONS_GENFB, CONS_NONE } what_was_cons;
	bool is_console;
	int error;

	/* genfb requires this.  */
	KASSERTMSG((void *)&sc->sc_genfb == device_private(da->da_dev),
	    "drmfb_softc must be first member of device softc");

	sc->sc_da = *da;

	prop_dictionary_set_uint32(dict, "width", sizes->surface_width);
	prop_dictionary_set_uint32(dict, "height", sizes->surface_height);
	prop_dictionary_set_uint8(dict, "depth", sizes->surface_bpp);
	prop_dictionary_set_uint16(dict, "linebytes", da->da_fb_linebytes);
	prop_dictionary_set_uint32(dict, "address", 0); /* XXX >32-bit */
	CTASSERT(sizeof(uintptr_t) <= sizeof(uint64_t));
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (uint64_t)(uintptr_t)da->da_fb_vaddr);

	prop_dictionary_set_uint64(dict, "mode_callback",
	    (uint64_t)(uintptr_t)&drmfb_genfb_mode_callback);

	if (!prop_dictionary_get_bool(dict, "is_console", &is_console)) {
		/* XXX Whattakludge!  */
#if NVGA > 0
		if ((da->da_params->dp_is_vga_console != NULL) &&
		    (*da->da_params->dp_is_vga_console)(dev)) {
			what_was_cons = CONS_VGA;
			prop_dictionary_set_bool(dict, "is_console", true);
			vga_cndetach();
			if (da->da_params->dp_disable_vga)
				(*da->da_params->dp_disable_vga)(dev);
		} else
#endif
		if (genfb_is_console() && genfb_is_enabled()) {
			what_was_cons = CONS_GENFB;
			prop_dictionary_set_bool(dict, "is_console", true);
		} else {
			what_was_cons = CONS_NONE;
			prop_dictionary_set_bool(dict, "is_console", false);
		}
	} else {
		what_was_cons = CONS_NONE;
	}

	/* Make the first EDID we find available to wsfb */
	drm_connector_list_iter_begin(da->da_fb_helper->dev, &conn_iter);
	drm_client_for_each_connector_iter(connector, &conn_iter) {
		struct drm_property_blob *edid = connector->edid_blob_ptr;
		if (edid && edid->length) {
			prop_dictionary_set_data(dict, "EDID", edid->data,
			    edid->length);
			break;
		}
	}
	drm_connector_list_iter_end(&conn_iter);

	sc->sc_genfb.sc_dev = sc->sc_da.da_dev;
	genfb_init(&sc->sc_genfb);
	genfb_ops.genfb_ioctl = drmfb_genfb_ioctl;
	genfb_ops.genfb_mmap = drmfb_genfb_mmap;
	genfb_ops.genfb_enable_polling = drmfb_genfb_enable_polling;
	genfb_ops.genfb_disable_polling = drmfb_genfb_disable_polling;

	KERNEL_LOCK(1, NULL);
	error = genfb_attach(&sc->sc_genfb, &genfb_ops);
	KERNEL_UNLOCK_ONE(NULL);
	if (error) {
		aprint_error_dev(sc->sc_da.da_dev,
		    "failed to attach genfb: %d\n", error);
		goto fail0;
	}

	/* Success!  */
	return 0;

fail0:	KASSERT(error);
	/* XXX Restore console...  */
	switch (what_was_cons) {
	case CONS_VGA:
		break;
	case CONS_GENFB:
		break;
	case CONS_NONE:
		break;
	default:
		break;
	}
	return error;
}

int
drmfb_detach(struct drmfb_softc *sc, int flags)
{

	/* XXX genfb detach?  */
	return 0;
}

static int
drmfb_genfb_ioctl(void *v, void *vs, unsigned long cmd, void *data, int flag,
    struct lwp *l)
{
	struct genfb_softc *const genfb = v;
	struct drmfb_softc *const sc = container_of(genfb, struct drmfb_softc,
	    sc_genfb);
	int error;

	if (sc->sc_da.da_params->dp_ioctl) {
		error = (*sc->sc_da.da_params->dp_ioctl)(sc, cmd, data, flag,
		    l);
		if (error != EPASSTHROUGH)
			return error;
	}

	switch (cmd) {
	/*
	 * Screen blanking ioctls.  Not to be confused with backlight
	 * (can be disabled while stuff is still drawn on the screen),
	 * brightness, or contrast (which we don't support).  Backlight
	 * and brightness are done through WSDISPLAYIO_{GET,SET}PARAM.
	 * This toggles between DPMS ON and DPMS OFF; backlight toggles
	 * between DPMS ON and DPMS SUSPEND.
	 */
	case WSDISPLAYIO_GVIDEO: {
		int *onp = (int *)data;

		/* XXX Can't really determine a single answer here.  */
		*onp = 1;
		return 0;
	}
	case WSDISPLAYIO_SVIDEO: {
		const int on = *(const int *)data;
		const int dpms_mode = on? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF;
		struct drm_fb_helper *const fb_helper = sc->sc_da.da_fb_helper;

		mutex_lock(&fb_helper->lock);
		drm_client_modeset_dpms(&fb_helper->client, dpms_mode);
		mutex_unlock(&fb_helper->lock);

		return 0;
	}
	default:
		return EPASSTHROUGH;
	}
}

static paddr_t
drmfb_genfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_softc *const genfb = v;
	struct drmfb_softc *const sc = container_of(genfb, struct drmfb_softc,
	    sc_genfb);

	KASSERT(0 <= offset);

	if (offset < genfb->sc_fbsize) {
		if (sc->sc_da.da_params->dp_mmapfb == NULL)
			return -1;
		return (*sc->sc_da.da_params->dp_mmapfb)(sc, offset, prot);
	} else {
		if (kauth_authorize_machdep(kauth_cred_get(),
			KAUTH_MACHDEP_UNMANAGEDMEM, NULL, NULL, NULL, NULL)
		    != 0)
			return -1;
		if (sc->sc_da.da_params->dp_mmap == NULL)
			return -1;
		return (*sc->sc_da.da_params->dp_mmap)(sc, offset, prot);
	}
}

static int
drmfb_genfb_enable_polling(void *cookie)
{
	struct genfb_softc *const genfb = cookie;
	struct drmfb_softc *const sc = container_of(genfb, struct drmfb_softc,
	    sc_genfb);

	return drm_fb_helper_debug_enter_fb(sc->sc_da.da_fb_helper);
}

static int
drmfb_genfb_disable_polling(void *cookie)
{
	struct genfb_softc *const genfb = cookie;
	struct drmfb_softc *const sc = container_of(genfb, struct drmfb_softc,
	    sc_genfb);

	return drm_fb_helper_debug_leave_fb(sc->sc_da.da_fb_helper);
}

static bool
drmfb_genfb_setmode(struct genfb_softc *genfb, int mode)
{
	struct drmfb_softc *sc = container_of(genfb, struct drmfb_softc,
	    sc_genfb);
	struct drm_fb_helper *fb_helper = sc->sc_da.da_fb_helper;

	if (mode == WSDISPLAYIO_MODE_EMUL)
		drm_fb_helper_restore_fbdev_mode_unlocked(fb_helper);

	return true;
}

bool
drmfb_shutdown(struct drmfb_softc *sc, int flags __unused)
{

	genfb_enable_polling(sc->sc_da.da_dev);
	return true;
}
