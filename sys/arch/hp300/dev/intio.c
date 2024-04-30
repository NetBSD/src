/*	$NetBSD: intio.c,v 1.33 2024/04/30 05:06:08 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: intio.c,v 1.33 2024/04/30 05:06:08 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/hp300spu.h>

#include <hp300/dev/intioreg.h>
#include <hp300/dev/intiovar.h>

struct intio_softc {
	device_t sc_dev;
	struct bus_space_tag sc_tag;
};

static int	intiomatch(device_t, cfdata_t, void *);
static void	intioattach(device_t, device_t, void *);
static int	intioprint(void *, const char *);

CFATTACH_DECL_NEW(intio, sizeof(struct intio_softc),
    intiomatch, intioattach, NULL, NULL);

#if defined(HP320) || defined(HP330) || defined(HP340) || defined(HP345) || \
    defined(HP350) || defined(HP360) || defined(HP370) || defined(HP375) || \
    defined(HP380) || defined(HP385)
#define	HAVE_INTIO_FB
#endif

#if defined(HP382) || defined(HP400) || defined(HP425) || defined(HP433)
#define	HAVE_INTIO_FRODO
#endif

#define	INTIO_3xx_BUILTINS						\
	(__BIT(HP_320) | __BIT(HP_330) | __BIT(HP_340) |		\
	 __BIT(HP_345) | __BIT(HP_345) | __BIT(HP_350) |		\
	 __BIT(HP_360) | __BIT(HP_370) | __BIT(HP_375) |		\
	 __BIT(HP_380) | __BIT(HP_385))

#define	INTIO_362_BUILTINS	__BIT(HP_362)
#define	INTIO_382_BUILTINS	__BIT(HP_382)

#define	INTIO_4xx_BUILTINS						\
	(__BIT(HP_400) | __BIT(HP_425) | __BIT(HP_433))

#define	INTIO_ALL_BUILTINS						\
	(INTIO_3xx_BUILTINS | INTIO_362_BUILTINS |			\
	 INTIO_382_BUILTINS | INTIO_4xx_BUILTINS)

static const struct intio_builtins intio_builtins[] = {
	{ "rtc",	RTC_BASE,	-1,
	  INTIO_ALL_BUILTINS },

#ifdef HAVE_INTIO_FRODO
	{ "frodo",	FRODO_BASE,	5,
	  INTIO_382_BUILTINS | INTIO_4xx_BUILTINS },
#endif

	{ "hil",	HIL_BASE,	1,
	  INTIO_ALL_BUILTINS },

	{ "hpib",	HPIB_BASE,	3,
	  INTIO_ALL_BUILTINS },

	{ "dma",	DMA_BASE,	1,
	  INTIO_ALL_BUILTINS },

#ifdef HAVE_INTIO_FB
	{ "fb",		FB_BASE,	-1,
	  INTIO_3xx_BUILTINS },
#endif
};
#define	nintio_builtins		__arraycount(intio_builtins)

static int intio_matched = 0;
extern void *internalhpib;

static int
intiomatch(device_t parent, cfdata_t cf, void *aux)
{

	/* Allow only one instance. */
	if (intio_matched)
		return 0;

	intio_matched = 1;
	return 1;
}

static void
intioattach(device_t parent, device_t self, void *aux)
{
	struct intio_softc *sc = device_private(self);
	struct intio_attach_args ia;
	bus_space_tag_t bst = &sc->sc_tag;
	const uint32_t spumask = 1U << machineid;
	int i;

	sc->sc_dev = self;
	aprint_normal("\n");

	KASSERT(spumask != 0);

	memset(bst, 0, sizeof(struct bus_space_tag));
	bst->bustype = HP300_BUS_SPACE_INTIO;

	memset(&ia, 0, sizeof(ia));

	for (i = 0; i < nintio_builtins; i++) {

		/*
		 * If the device doesn't exist on this specific model,
		 * skip it.
		 */
		if ((intio_builtins[i].ib_spumask & spumask) == 0)
			continue;

		/*
		 * Internal HP-IB doesn't always return a device ID,
		 * so we rely on the sysflags.
		 */
		if (intio_builtins[i].ib_offset == HPIB_BASE && !internalhpib)
			continue;

		strlcpy(ia.ia_modname, intio_builtins[i].ib_modname,
		    sizeof(ia.ia_modname));
		ia.ia_bst = bst;
		ia.ia_iobase = intio_builtins[i].ib_offset;
		ia.ia_addr =
		    (bus_addr_t)(intiobase + intio_builtins[i].ib_offset);
		ia.ia_ipl = intio_builtins[i].ib_ipl;
		config_found(self, &ia, intioprint, CFARGS_NONE);
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
