/*	$NetBSD: omap_ocp.c,v 1.1.52.1 2008/06/02 13:21:55 mjf Exp $ */

/*
 * Autoconfiguration support for the Texas Instruments OMAP OCP bus.
 * Based on arm/xscale/pxa2x0.c which in turn was derived
 * from arm/sa11x0/sa11x0.c
 *
 * Copyright (c) 2002, 2005  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	This product includes software developed for the NetBSD Project by
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 1997, 1998, 2001, The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro, Ichiro FUKUHARA and Paul Kranenburg.
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
 *
 * Copyright (c) 1999
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap_ocp.c,v 1.1.52.1 2008/06/02 13:21:55 mjf Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_ocp.h>

struct ocp_softc {
	struct device sc_dev;
	bus_dma_tag_t sc_dmac;
};

/* prototypes */
static int	ocp_match(struct device *, struct cfdata *, void *);
static void	ocp_attach(struct device *, struct device *, void *);
static int 	ocp_search(struct device *, struct cfdata *,
			     const int *, void *);
static int	ocp_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(ocp, sizeof(struct ocp_softc),
    ocp_match, ocp_attach, NULL, NULL);

static int ocp_attached;

static int
ocp_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (ocp_attached)
		return 0;
	return 1;
}

static void
ocp_attach(struct device *parent, struct device *self, void *aux)
{
	struct ocp_softc *sc = (struct ocp_softc *)self;

	ocp_attached = 1;

#if NOMAPDMAC > 0
#error DMA not implemented
	sc->sc_dmac = &omap_bus_dma_tag;
#else
	sc->sc_dmac = NULL;
#endif

	aprint_normal(": OCP Bus\n");
	aprint_naive("\n");

	/*
	 * Attach all our devices
	 */
	config_search_ia(ocp_search, self, "ocp", NULL);
}

static int
ocp_search(struct device *parent, struct cfdata *cf,
	     const int *ldesc, void *aux)
{
	struct ocp_softc *sc = (struct ocp_softc *)parent;
	struct ocp_attach_args aa;

	switch (cf->cf_loc[OCPCF_MULT]) {
	case 1:
		aa.ocp_iot = &omap_bs_tag;
		break;
	case 2:
		aa.ocp_iot = &omap_a2x_bs_tag;
		break;
	case 4:
		aa.ocp_iot = &omap_a4x_bs_tag;
		break;
	default:
		panic("Unsupported OCP multiplier.");
		break;
	}
	aa.ocp_dmac = sc->sc_dmac;
	aa.ocp_addr = cf->cf_loc[OCPCF_ADDR];
	aa.ocp_size = cf->cf_loc[OCPCF_SIZE];
	aa.ocp_intr = cf->cf_loc[OCPCF_INTR];

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, ocp_print);

	return 0;
}

static int
ocp_print(void *aux, const char *name)
{
	struct ocp_attach_args *sa = (struct ocp_attach_args*)aux;

	if (sa->ocp_addr != OCPCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", sa->ocp_addr);
		if (sa->ocp_size > OCPCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx", sa->ocp_addr + sa->ocp_size-1);
	}
	if (sa->ocp_intr != OCPCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->ocp_intr);

	return (UNCONF);
}
