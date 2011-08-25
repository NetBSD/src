/* $NetBSD: genfb_thunkbus.c,v 1.1 2011/08/25 11:45:25 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: genfb_thunkbus.c,v 1.1 2011/08/25 11:45:25 jmcneill Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/wsfb/genfbvar.h>

#include <machine/mainbus.h>
#include <machine/thunk.h>

#ifdef _KERNEL_OPT
#include "opt_sdl.h"
#endif

#if !defined(SDL)
#error genfb_thunkbus requires options SDL
#endif

#define GENFB_THUNKBUS_WIDTH	640
#define	GENFB_THUNKBUS_HEIGHT	480
#define	GENFB_THUNKBUS_DEPTH	32
#define	GENFB_THUNKBUS_FBSIZE	\
	(GENFB_THUNKBUS_WIDTH * GENFB_THUNKBUS_HEIGHT * (GENFB_THUNKBUS_DEPTH / 8))

static int	genfb_thunkbus_match(device_t, cfdata_t, void *);
static void	genfb_thunkbus_attach(device_t, device_t, void *);
static int	genfb_thunkbus_ioctl(void *, void *, u_long, void *, int, lwp_t *);
static paddr_t	genfb_thunkbus_mmap(void *, void *, off_t, int);

int		genfb_thunkbus_cnattach(void);

struct genfb_thunkbus_softc {
	struct genfb_softc	sc_gen;
};

static void *	genfb_thunkbus_fbaddr;

CFATTACH_DECL_NEW(genfb_thunkbus, sizeof(struct genfb_thunkbus_softc),
    genfb_thunkbus_match, genfb_thunkbus_attach, NULL, NULL);

static int
genfb_thunkbus_match(device_t parent, cfdata_t match, void *opaque)
{
	struct thunkbus_attach_args *taa = opaque;

	if (taa->taa_type != THUNKBUS_TYPE_GENFB)
		return 0;

	if (thunk_getenv("DISPLAY") == NULL)
		return 0;

	return 1;
}

static void
genfb_thunkbus_attach(device_t parent, device_t self, void *opaque)
{
	struct genfb_thunkbus_softc *sc = device_private(self);
	struct genfb_ops ops;
	prop_dictionary_t dict = device_properties(self);

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_gen.sc_dev = self;
	sc->sc_gen.sc_fbaddr = genfb_thunkbus_fbaddr;

	prop_dictionary_set_bool(dict, "is_console", true);
	prop_dictionary_set_uint32(dict, "width", GENFB_THUNKBUS_WIDTH);
	prop_dictionary_set_uint32(dict, "height", GENFB_THUNKBUS_HEIGHT);
	prop_dictionary_set_uint8(dict, "depth", GENFB_THUNKBUS_DEPTH);
	prop_dictionary_set_uint16(dict, "linebytes",
	    GENFB_THUNKBUS_WIDTH * (GENFB_THUNKBUS_DEPTH / 8));
	prop_dictionary_set_uint64(dict, "address", 0);
	prop_dictionary_set_uint64(dict, "virtual_address",
	    (vaddr_t)sc->sc_gen.sc_fbaddr);

	genfb_init(&sc->sc_gen);

	memset(&ops, 0, sizeof(ops));
	ops.genfb_ioctl = genfb_thunkbus_ioctl;
	ops.genfb_mmap = genfb_thunkbus_mmap;

	if (genfb_attach(&sc->sc_gen, &ops) != 0)
		panic("%s: couldn't attach genfb", __func__);

}

static int
genfb_thunkbus_ioctl(void *v, void *vs, u_long cmd, void *data, int flag, lwp_t *l)
{
	switch (cmd) {
	case WSDISPLAYIO_GTYPE:
		*(unsigned int *)data = WSDISPLAY_TYPE_UNKNOWN;
		return 0;
	}

	return EPASSTHROUGH;
}

static paddr_t
genfb_thunkbus_mmap(void *v, void *vs, off_t off, int prot)
{
	return -1;
}

int
genfb_thunkbus_cnattach(void)
{
	int error;

	if (thunk_getenv("DISPLAY") == NULL)
		return 0;

	genfb_thunkbus_fbaddr = thunk_sdl_getfb(GENFB_THUNKBUS_FBSIZE);
	if (genfb_thunkbus_fbaddr == NULL)
		return 0;

	error = thunk_sdl_init(GENFB_THUNKBUS_WIDTH,
	    GENFB_THUNKBUS_HEIGHT, GENFB_THUNKBUS_DEPTH);
	if (error)
		return 0;

	return 1;
}
