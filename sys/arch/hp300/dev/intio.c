/*	$NetBSD: intio.c,v 1.25.10.1 2007/03/12 05:47:44 rmind Exp $	*/

/*-
 * Copyright (c) 1996, 1998, 2001 The NetBSD Foundation, Inc.
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

/*
 * Autoconfiguration support for hp300 internal i/o space.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intio.c,v 1.25.10.1 2007/03/12 05:47:44 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/hp300spu.h>

#include <hp300/dev/intioreg.h>
#include <hp300/dev/intiovar.h>

struct intio_softc {
	struct device sc_dev;
	struct bus_space_tag sc_tag;
};

static int	intiomatch(struct device *, struct cfdata *, void *);
static void	intioattach(struct device *, struct device *, void *);
static int	intioprint(void *, const char *);

CFATTACH_DECL(intio, sizeof(struct intio_softc),
    intiomatch, intioattach, NULL, NULL);

#if defined(HP320) || defined(HP330) || defined(HP340) || defined(HP345) || \
    defined(HP350) || defined(HP360) || defined(HP370) || defined(HP375) || \
    defined(HP380) || defined(HP385)
static const struct intio_builtins intio_3xx_builtins[] = {
	{ "rtc",	RTC_BASE,	-1},
	{ "hil",	HIL_BASE,	1},
	{ "hpib",	HPIB_BASE,	3},
	{ "dma",	DMA_BASE,	1},
	{ "fb",		FB_BASE,	-1},
};
#define nintio_3xx_builtins	__arraycount(intio_3xx_builtins)
#endif

#if defined(HP362) || defined(HP382)
static const struct intio_builtins intio_3x2_builtins[] = {
	{ "rtc",	RTC_BASE,	-1},
	{ "frodo",	FRODO_BASE,	5},
	{ "hil",	HIL_BASE,	1},
	{ "hpib",	HPIB_BASE,	3},
	{ "dma",	DMA_BASE,	1},
};
#define nintio_3x2_builtins	__arraycount(intio_3x2_builtins)
#endif

#if defined(HP400) || defined(HP425) || defined(HP433)
static const struct intio_builtins intio_4xx_builtins[] = {
	{ "rtc",	RTC_BASE,	-1},
	{ "frodo",	FRODO_BASE,	5},
	{ "hil",	HIL_BASE,	1},
	{ "hpib",	HPIB_BASE,	3},
	{ "dma",	DMA_BASE,	1},
};
#define nintio_4xx_builtins	__arraycount(intio_4xx_builtins)
#endif

static int intio_matched = 0;
extern void *internalhpib;

static int
intiomatch(struct device *parent, struct cfdata *match, void *aux)
{
	/* Allow only one instance. */
	if (intio_matched)
		return 0;

	intio_matched = 1;
	return 1;
}

static void
intioattach(struct device *parent, struct device *self, void *aux)
{
	struct intio_softc *sc = (struct intio_softc *)self;
	struct intio_attach_args ia;
	const struct intio_builtins *ib;
	bus_space_tag_t bst = &sc->sc_tag;
	int ndevs;
	int i;

	printf("\n");

	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_INTIO;

	switch (machineid) {
#if defined(HP320) || defined(HP330) || defined(HP340) || defined(HP345) || \
    defined(HP350) || defined(HP360) || defined(HP370) || defined(HP375) || \
    defined(HP380) || defined(HP385)
	case HP_320:
	case HP_330:
	case HP_340:
	case HP_345:
	case HP_350:
	case HP_360:
	case HP_370:
	case HP_375:
	case HP_380:
	case HP_385:
		ib = intio_3xx_builtins;
		ndevs = nintio_3xx_builtins;
		break;
#endif
#if defined(HP362) || defined(HP382)
	case HP_362:
	case HP_382:
		ib = intio_3x2_builtins;
		ndevs = nintio_3x2_builtins;
		break;
#endif
#if defined(HP400) || defined(HP425) || defined(HP433)
	case HP_400:
	case HP_425:
	case HP_433:
		ib = intio_4xx_builtins;
		ndevs = nintio_4xx_builtins;
		break;
#endif
	default:
		return;
	}

	memset(&ia, 0, sizeof(ia));

	for (i = 0; i < ndevs; i++) {

		/*
		 * Internal HP-IB doesn't always return a device ID,
		 * so we rely on the sysflags.
		 */
		if (ib[i].ib_offset == HPIB_BASE && !internalhpib)
			continue;

		strlcpy(ia.ia_modname, ib[i].ib_modname,
		    sizeof(ia.ia_modname));
		ia.ia_bst = bst;
		ia.ia_iobase = ib[i].ib_offset;
		ia.ia_addr = (bus_addr_t)(intiobase + ib[i].ib_offset);
		ia.ia_ipl = ib[i].ib_ipl;
		config_found(self, &ia, intioprint);
	}
}

static int
intioprint(void *aux, const char *pnp)
{
	struct intio_attach_args *ia = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s", ia->ia_modname, pnp);
	if (ia->ia_iobase != 0 && pnp == NULL) {
		aprint_normal(" addr 0x%lx", INTIOBASE + ia->ia_iobase);
		if (ia->ia_ipl != -1)
			aprint_normal(" ipl %d", ia->ia_ipl);
	}
	return UNCONF;
}
