/* $NetBSD: bcm2835_genfb.c,v 1.9 2018/04/01 04:35:03 ryo Exp $ */

/*-
 * Copyright (c) 2013 Jared D. McNeill <jmcneill@invisible.ca>
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

/*
 * Generic framebuffer console driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_genfb.c,v 1.9 2018/04/01 04:35:03 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <dev/wsfb/genfbvar.h>

struct bcmgenfb_softc {
	struct genfb_softc	sc_gen;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	uint32_t		sc_wstype;
};

static int	bcmgenfb_match(device_t, cfdata_t, void *);
static void	bcmgenfb_attach(device_t, device_t, void *);

static int	bcmgenfb_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	bcmgenfb_mmap(void *, void *, off_t, int);
static bool	bcmgenfb_shutdown(device_t, int);

void		bcmgenfb_set_console_dev(device_t);
void		bcmgenfb_set_ioctl(int(*)(void *, void *, u_long, void *, int, struct lwp *));
void		bcmgenfb_ddb_trap_callback(int);

static device_t bcmgenfb_console_dev = NULL;
int (*bcmgenfb_ioctl_handler)(void *, void *, u_long, void *, int, struct lwp *) = NULL;

CFATTACH_DECL_NEW(bcmgenfb, sizeof(struct bcmgenfb_softc),
    bcmgenfb_match, bcmgenfb_attach, NULL, NULL);

static int
bcmgenfb_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-fb", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bcmgenfb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmgenfb_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t dict = device_properties(self);
	static const struct genfb_ops zero_ops;
	struct genfb_ops ops = zero_ops;
	bool is_console = false;
	int error;

	sc->sc_gen.sc_dev = self;
	sc->sc_iot = faa->faa_bst;

	sc->sc_wstype = WSDISPLAY_TYPE_VC4;
	prop_dictionary_get_uint32(dict, "wsdisplay_type", &sc->sc_wstype);
	prop_dictionary_get_bool(dict, "is_console", &is_console);

	genfb_init(&sc->sc_gen);

	if (sc->sc_gen.sc_width == 0 ||
	    sc->sc_gen.sc_fbsize == 0) {
		aprint_normal(": disabled\n");
		return;
	}

	pmf_device_register1(self, NULL, NULL, bcmgenfb_shutdown);

	error = bus_space_map(sc->sc_iot, sc->sc_gen.sc_fboffset,
	    sc->sc_gen.sc_fbsize,
	    BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE, &sc->sc_ioh);
	if (error) {
		aprint_error_dev(self, "couldn't map framebuffer (%d)\n",
		    error);
		return;
	}
	sc->sc_gen.sc_fbaddr = bus_space_vaddr(sc->sc_iot, sc->sc_ioh);

	ops.genfb_ioctl = bcmgenfb_ioctl;
	ops.genfb_mmap = bcmgenfb_mmap;

	aprint_naive("\n");

	if (is_console)
		aprint_normal(": switching to framebuffer console\n");
	else
		aprint_normal("\n");

	genfb_attach(&sc->sc_gen, &ops);
}

static int
bcmgenfb_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	struct bcmgenfb_softc *sc = v;
	struct wsdisplayio_bus_id *busid;

	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(u_int *)data = sc->sc_wstype;
		return 0;
	case WSDISPLAYIO_GET_BUSID:
		busid = data;
		busid->bus_type = WSDISPLAYIO_BUS_SOC;
		return 0;
	case WSDISPLAYIO_GET_FBINFO:
		{
			struct wsdisplayio_fbinfo *fbi = data;
			struct rasops_info *ri = &sc->sc_gen.vd.active->scr_ri;
			int ret;

			ret = wsdisplayio_get_fbinfo(ri, fbi);
			fbi->fbi_flags |= WSFB_VRAM_IS_RAM;
			return ret;
		}
	default:
		if (bcmgenfb_ioctl_handler != NULL)
			return bcmgenfb_ioctl_handler(v, vs, cmd, data, flag, l);
		return EPASSTHROUGH;
	}
}

static paddr_t
bcmgenfb_mmap(void *v, void *vs, off_t offset, int prot)
{
	struct bcmgenfb_softc *sc = v;

	if (offset < 0 || offset >= sc->sc_gen.sc_fbsize)
		return -1;

	return bus_space_mmap(sc->sc_iot, sc->sc_gen.sc_fboffset, offset,
	    prot, BUS_SPACE_MAP_LINEAR|BUS_SPACE_MAP_PREFETCHABLE);
}

static bool
bcmgenfb_shutdown(device_t self, int flags)
{
	genfb_enable_polling(self);
	return true;
}
void
bcmgenfb_set_console_dev(device_t dev)
{
	/* skip if already set. called from each genfb0,genfb1,... */
	if (bcmgenfb_console_dev != NULL)
		return;

	bcmgenfb_console_dev = dev;
}

void
bcmgenfb_set_ioctl(int(*boo)(void *, void *, u_long, void *, int, struct lwp *))
{
	bcmgenfb_ioctl_handler = boo;
}

void
bcmgenfb_ddb_trap_callback(int where)
{
	if (bcmgenfb_console_dev == NULL)
		return;

	if (where) {
		genfb_enable_polling(bcmgenfb_console_dev);
	} else {
		genfb_disable_polling(bcmgenfb_console_dev);
	}
}
