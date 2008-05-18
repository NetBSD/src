/*	$NetBSD: grf_subr.c,v 1.16.2.1 2008/05/18 12:31:55 yamt Exp $	*/

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

/*
 * Subroutines common to all framebuffer devices.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: grf_subr.c,v 1.16.2.1 2008/05/18 12:31:55 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <hp300/dev/grfioctl.h>
#include <hp300/dev/grfvar.h>

static int	grfdevprint(void *, const char *);

void
grfdev_attach(struct grfdev_softc *sc,
    int (*init)(struct grf_data *, int, uint8_t *),
    void *regs, struct grfsw *sw)
{
	struct grfdev_attach_args ga;
	struct grf_data *gp;

	if (sc->sc_isconsole)
		sc->sc_data = gp = &grf_cn;
	else {
		sc->sc_data = malloc(sizeof(struct grf_data),
		    M_DEVBUF, M_NOWAIT | M_ZERO);
		if (sc->sc_data == NULL) {
			aprint_error(": can't allocate grf data\n");
			return;
		}

		/* Initialize the framebuffer hardware. */
		if ((*init)(sc->sc_data, sc->sc_scode, regs) == 0) {
			aprint_error(": init failed\n");
			free(sc->sc_data, M_DEVBUF);
			return;
		}

		gp = sc->sc_data;
		gp->g_flags = GF_ALIVE;
		gp->g_sw = sw;
		gp->g_display.gd_id = gp->g_sw->gd_swid;
	}

	/* Announce ourselves. */
	printf(": %d x %d ", gp->g_display.gd_dwidth,
	    gp->g_display.gd_dheight);
	if (gp->g_display.gd_colors == 2)
		printf("monochrome");
	else
		printf("%d color", gp->g_display.gd_colors);
	printf(" %s display\n", gp->g_sw->gd_desc);

	/* Attach a grf. */
	ga.ga_scode = sc->sc_scode;	/* XXX */
	ga.ga_isconsole = sc->sc_isconsole;
	ga.ga_data = (void *)sc->sc_data;
	(void)config_found(sc->sc_dev, &ga, grfdevprint);
}

static int
grfdevprint(void *aux, const char *pnp)
{
	/* struct grfdev_attach_args *ga = aux; */

	/* Only grf's can attach to grfdev's... easy. */
	if (pnp)
		aprint_normal("grf at %s", pnp);

	return (UNCONF);
}
