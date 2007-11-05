/*	$Id: omap2_obio.c,v 1.1.2.3 2007/11/05 22:01:56 matt Exp $	*/

/* adapted from: */
/*	$NetBSD: omap2_obio.c,v 1.1.2.3 2007/11/05 22:01:56 matt Exp $ */


/*
 * Autoconfiguration support for the Texas Instruments OMAP "On Board" I/O.
 *
 * Based on arm/omap/omap_emifs.c which in turn was derived
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
 * Copyright (c) 2001, The NetBSD Foundation, Inc.  All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro and Ichiro FUKUHARA.
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *
 * Copyright (c) 1997,1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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

#include "opt_omap.h"
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap2_obio.c,v 1.1.2.3 2007/11/05 22:01:56 matt Exp $");

#include "locators.h"
#include "obio.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
#include <arm/omap/omap_var.h>

#include <arm/omap/omap2430obioreg.h>
#include <arm/omap/omap2430obiovar.h>

typedef struct {
	boolean_t	cs_valid;
	ulong		cs_addr;
	ulong		cs_size;
} obio_csconfig_t;

struct obio_softc {
	struct device		sc_dev;
	bus_dma_tag_t		sc_dmat;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_base;
	bus_size_t		sc_size;
};


/* prototypes */
static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int 	obio_search(device_t, cfdata_t, const int *, void *);
static int	obio_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(obio, sizeof(struct obio_softc),
	obio_match, obio_attach, NULL, NULL);

static int obio_attached[NOBIO];

static int
obio_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbus_attach_args *mb = aux;


#if defined(OMAP2)
	if ((mb->mb_iobase == OMAP2430_OBIO_0_BASE)
	&&  (mb->mb_iosize == OMAP2430_OBIO_0_SIZE)
	&&  (obio_attached[0] == 0))
		return 1;
	if ((mb->mb_iobase == OMAP2430_OBIO_1_BASE)
	&&  (mb->mb_iosize == OMAP2430_OBIO_1_SIZE)
	&&  (obio_attached[1] == 0))
		return 1;
#else
# error unknown OMAP implementation
#endif

	return 0;
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct obio_softc *sc = device_private(self);
	struct mainbus_attach_args *mb = (struct mainbus_attach_args *)aux;

	sc->sc_iot = &omap_bs_tag;

	aprint_normal(": On-Board IO\n");

	sc->sc_ioh = 0;
#ifdef NOTYET
	sc->sc_dmat = &omap_bus_dma_tag;
#endif
	sc->sc_base = mb->mb_iobase;
	sc->sc_size = mb->mb_iosize;

#if defined(OMAP2)
	if (mb->mb_iobase == OMAP2430_OBIO_0_BASE)
		obio_attached[0] = 1;
	if (mb->mb_iobase == OMAP2430_OBIO_1_BASE)
		obio_attached[1] = 1;
#endif

	/*
	 * Attach all our devices
	 */
	config_search_ia(obio_search, self, "obio", NULL);
}

static int
obio_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_softc *sc = (struct obio_softc *)parent;
	struct obio_attach_args aa;

	/* Set up the attach args. */
	if (cf->cf_loc[OBIOCF_NOBYTEACC] == 1) {
		if (cf->cf_loc[OBIOCF_MULT] == 1)
			aa.obio_iot = &nobyteacc_bs_tag;
		else
			panic("nobyteacc specified for device with "
				"non-byte multiplier\n");
	} else {
		switch (cf->cf_loc[OBIOCF_MULT]) {
		case 1:
			aa.obio_iot = &omap_bs_tag;
			break;
		case 2:
			aa.obio_iot = &omap_a2x_bs_tag;
			break;
		case 4:
			aa.obio_iot = &omap_a4x_bs_tag;
			break;
		default:
			panic("Unsupported EMIFS multiplier.");
			break;
		}
	}

	aa.obio_dmat = sc->sc_dmat;
	aa.obio_addr = cf->cf_loc[OBIOCF_ADDR];
	aa.obio_size = cf->cf_loc[OBIOCF_SIZE];
	aa.obio_intr = cf->cf_loc[OBIOCF_INTR];
	aa.obio_intrbase = cf->cf_loc[OBIOCF_INTRBASE];

#if defined(OMAP2)
	if ((aa.obio_addr >= sc->sc_base)
	&&  (aa.obio_addr < (sc->sc_base + sc->sc_size))) {
		/* XXX
		 * if size was specified, then check it too
		 * otherwise just assume it is OK
		 */
		if ((aa.obio_size != OBIOCF_SIZE_DEFAULT)
		&&  ((aa.obio_addr + aa.obio_size)
			>= (sc->sc_base + sc->sc_size)))
				return 1;		/* NG */
		if (config_match(parent, cf, &aa)) {
			config_attach(parent, cf, &aa, obio_print);
			return 0;			/* love it */
		}
	}
#endif

	return UNCONF;	/* NG */
}

static int
obio_print(void *aux, const char *name)
{
	struct obio_attach_args *sa = (struct obio_attach_args*)aux;

	if (sa->obio_addr != OBIOCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", sa->obio_addr);
		if (sa->obio_size != OBIOCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx",
				sa->obio_addr + sa->obio_size-1);
	}
	if (sa->obio_intr != OBIOCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->obio_intr);
	if (sa->obio_intr != OBIOCF_INTRBASE_DEFAULT)
		aprint_normal(" intrbase %d", sa->obio_intrbase);

	return UNCONF;
}
