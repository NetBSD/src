/*	$NetBSD: genfb_vmbus.c,v 1.3 2021/08/07 16:19:11 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: genfb_vmbus.c,v 1.3 2021/08/07 16:19:11 thorpej Exp $");

#include "opt_wsfb.h"
#include "opt_genfb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/mutex.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/kauth.h>

#include <dev/wsfb/genfbvar.h>

#include <dev/hyperv/vmbusvar.h>
#include <dev/hyperv/genfb_vmbusvar.h>

static int	genfb_vmbus_match(device_t, cfdata_t, void *);
static void	genfb_vmbus_attach(device_t, device_t, void *);
static int	genfb_vmbus_ioctl(void *, void *, u_long, void *, int,
		    struct lwp *);
static paddr_t	genfb_vmbus_mmap(void *, void *, off_t, int);
static int	genfb_vmbus_drm_print(void *, const char *);
static bool	genfb_vmbus_shutdown(device_t, int);

CFATTACH_DECL_NEW(genfb_vmbus, sizeof(struct genfb_vmbus_softc),
    genfb_vmbus_match, genfb_vmbus_attach, NULL, NULL);

static int
genfb_vmbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct vmbus_attach_args *aa = aux;

	if (memcmp(aa->aa_type, &hyperv_guid_video, sizeof(*aa->aa_type)) != 0)
		return 0;

	if (!genfb_is_enabled())
		return 0;	/* explicitly disabled by MD code */

	/* Use genfb(4) at pci in Gen.1 VM. */
	if (hyperv_is_gen1())
		return 0;

	return 1;
}

static void
genfb_vmbus_attach(device_t parent, device_t self, void *aux)
{
	static const struct genfb_ops zero_ops;
	struct genfb_vmbus_softc *sc = device_private(self);
	struct vmbus_attach_args *aa = aux;
	struct genfb_ops ops = zero_ops;

	aprint_naive("\n");
	aprint_normal(": Hyper-V Synthetic Video\n");

	sc->sc_gen.sc_dev = self;
	sc->sc_memt = aa->aa_memt;

	genfb_init(&sc->sc_gen);

	/* firmware / MD code responsible for restoring the display */
	if (sc->sc_gen.sc_pmfcb == NULL)
		pmf_device_register1(self, NULL, NULL,
		    genfb_vmbus_shutdown);
	else
		pmf_device_register1(self,
		    sc->sc_gen.sc_pmfcb->gpc_suspend,
		    sc->sc_gen.sc_pmfcb->gpc_resume,
		    genfb_vmbus_shutdown);

	if ((sc->sc_gen.sc_width == 0) || (sc->sc_gen.sc_fbsize == 0)) {
		aprint_debug_dev(self, "not configured by firmware\n");
		return;
	}

	ops.genfb_ioctl = genfb_vmbus_ioctl;
	ops.genfb_mmap = genfb_vmbus_mmap;

	if (genfb_attach(&sc->sc_gen, &ops) != 0)
		return;

	/* now try to attach a DRM */
	config_found(self, aux, genfb_vmbus_drm_print,
	    CFARGS(.iattr = "drm"));
}

static int
genfb_vmbus_drm_print(void *aux, const char *pnp)
{

	if (pnp)
		aprint_normal("drm at %s", pnp);
	return UNCONF;
}

static int
genfb_vmbus_ioctl(void *v, void *vs, u_long cmd, void *data, int flag,
    struct lwp *l)
{

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = WSDISPLAY_TYPE_GENFB;
		return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
genfb_vmbus_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct genfb_vmbus_softc *sc = v;

	return bus_space_mmap(sc->sc_memt, sc->sc_gen.sc_fboffset, offset, prot,
	    BUS_SPACE_MAP_LINEAR | BUS_SPACE_MAP_PREFETCHABLE);
}

static bool
genfb_vmbus_shutdown(device_t self, int flags)
{

	genfb_enable_polling(self);
	return true;
}
