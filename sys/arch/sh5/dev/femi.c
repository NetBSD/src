/*	$NetBSD: femi.c,v 1.9 2003/07/15 03:35:58 lukem Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * SH-5 FEMI Bus Attachment Glue
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: femi.c,v 1.9 2003/07/15 03:35:58 lukem Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <sh5/dev/superhywayvar.h>
#include <sh5/dev/femivar.h>

#include "locators.h"

static int femimatch(struct device *, struct cfdata *, void *);
static void femiattach(struct device *, struct device *, void *);
static int femiprint(void *, const char *);
static int femisubmatch(struct device *, struct cfdata *, void *);

struct femi_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
	bus_dma_tag_t sc_dmat;
	bus_addr_t sc_base;
	bus_addr_t sc_top;
};

CFATTACH_DECL(femi, sizeof(struct femi_softc),
    femimatch, femiattach, NULL, NULL);
extern struct cfdriver femi_cd;


#define	FEMI_MODULE_ID	0x2185


/*ARGSUSED*/
static int
femimatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct superhyway_attach_args *sa = args;
	bus_space_handle_t bh;
	u_int64_t vcr;

	if (strcmp(sa->sa_name, femi_cd.cd_name))
		return (0);

	sa->sa_pport = 0;

	bus_space_map(sa->sa_bust,
	    SUPERHYWAY_PPORT_TO_BUSADDR(cf->cf_loc[SUPERHYWAYCF_PPORT]),
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	if (SUPERHYWAY_VCR_MOD_ID(vcr) != FEMI_MODULE_ID)
		return (0);

	sa->sa_pport = cf->cf_loc[SUPERHYWAYCF_PPORT];

	return (1);
}

/*ARGSUSED*/
static void
femiattach(struct device *parent, struct device *self, void *args)
{
	struct superhyway_attach_args *sa = args;
	struct femi_softc *sc = (struct femi_softc *)self;
	bus_space_handle_t bh;
	u_int64_t vcr;

	bus_space_map(sa->sa_bust, SUPERHYWAY_PPORT_TO_BUSADDR(sa->sa_pport),
	    SUPERHYWAY_REG_SZ, 0, &bh);
	vcr = bus_space_read_8(sa->sa_bust, bh, SUPERHYWAY_REG_VCR);
	bus_space_unmap(sa->sa_bust, bh, SUPERHYWAY_REG_SZ);

	sc->sc_bust = sa->sa_bust;
	sc->sc_dmat = sa->sa_dmat;
	sc->sc_base = 0;
	sc->sc_top = 0x08000000;

	printf(": Flash/External Memory Interface, Version 0x%x\n",
	    (int)SUPERHYWAY_VCR_MOD_VERS(vcr));

	printf("%s: Base Address: 0x%x, Length: 0x%x\n",
	    sc->sc_dev.dv_xname, sc->sc_base, sc->sc_top - sc->sc_base);

	config_search(femisubmatch, self, NULL);
}

static int
femiprint(void *arg, const char *cp)
{
	struct femi_attach_args *fa = arg;

	aprint_normal(" offset 0x%x", fa->fa_offset - fa->_fa_base);

	return (UNCONF);
}

static int
femisubmatch(struct device *dev, struct cfdata *cf, void *arg)
{
	struct femi_softc *sc = (struct femi_softc *) dev;
	struct femi_attach_args fa;

	fa.fa_bust = sc->sc_bust;
	fa.fa_dmat = sc->sc_dmat;
	fa.fa_offset = (bus_addr_t) cf->cf_loc[FEMICF_OFFSET];
	fa.fa_offset += sc->sc_base;
	fa._fa_base = sc->sc_base;

	if (config_match(dev, cf, &fa)) {
		config_attach(dev, cf, &fa, femiprint);
		return (1);
	}

	return (0);
}
