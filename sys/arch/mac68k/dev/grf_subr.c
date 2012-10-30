/*	$NetBSD: grf_subr.c,v 1.20.34.1 2012/10/30 17:19:55 yamt Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: grf_subr.c,v 1.20.34.1 2012/10/30 17:19:55 yamt Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/grfioctl.h>

#include <mac68k/nubus/nubus.h>
#include <mac68k/dev/grfvar.h>

void
grf_establish(struct grfbus_softc *sc, nubus_slot *sp,
    int (*g_mode)(struct grf_softc *, int, void *))
{
	struct grfmode *gm = &sc->curr_mode;
	struct grfbus_attach_args ga;

	/* Print hardware characteristics. */
	printf("%s: %d x %d, ", device_xname(sc->sc_dev), gm->width, gm->height);
	if (gm->psize == 1)
		printf("monochrome\n");
	else
		printf("%d color\n", 1 << gm->psize);

	/* Attach grf semantics to the hardware. */
	ga.ga_name = "grf";
	ga.ga_grfmode = gm;
	ga.ga_slot = sp;
	ga.ga_tag = sc->sc_tag;
	ga.ga_handle = sc->sc_handle;
	ga.ga_phys = sc->sc_basepa;
	ga.ga_mode = g_mode;
	(void)config_found(sc->sc_dev, &ga, grfbusprint);
}

int
grfbusprint(void *aux, const char *name)
{
	struct grfbus_attach_args *ga = aux;

	if (name)
		aprint_normal("%s at %s", ga->ga_name, name);

	return (UNCONF);
}
