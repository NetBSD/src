/*	$NetBSD: omap_tipb.c,v 1.5.12.1 2014/08/20 00:02:47 tls Exp $ */

/*
 * Autoconfiguration support for the Texas Instruments OMAP TIPB.
 * Based on arm/xscale/pxa2x0.c which in turn was derived from
 * arm/sa11x0/sa11x0.c.  The code to do the early attach was initially derived
 * from arch/sparc/dev/obio.c.
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
__KERNEL_RCSID(0, "$NetBSD: omap_tipb.c,v 1.5.12.1 2014/08/20 00:02:47 tls Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <sys/bus.h>

#include <arm/cpufunc.h>
#include <arm/mainbus/mainbus.h>
/* 
 * XXX. Do we really need this ? #include <arm/omap/omap_reg.h> 
 * Atleast commenting this makes it more generic.
 */
#include <arm/omap/omap_tipb.h>

struct tipb_softc {
	device_t sc_dev;
	bus_dma_tag_t sc_dmac;
};

/* prototypes */
static int	tipb_match(device_t, cfdata_t, void *);
static void	tipb_attach(device_t, device_t, void *);
static int 	tipb_search(device_t, cfdata_t, const int *, void *);
static int	tipb_print(void *, const char *);

/* attach structures */
CFATTACH_DECL_NEW(tipb, sizeof(struct tipb_softc),
    tipb_match, tipb_attach, NULL, NULL);

static int tipb_attached;

/*
 * There are some devices that need to be set up before all the others.  The
 * earlies array contains their names.  tipb_attach() and tipb_search() work
 * together to attach these devices in the order they appear in this array
 * before any other TIPB devices are attached.
 */
static const char * const earlies[] = {
	OMAP_INTC_DEVICE,
	"omapmputmr",
	NULL
};

static int
tipb_match(device_t parent, cfdata_t match, void *aux)
{
	if (tipb_attached)
		return 0;
	return 1;
}

static void
tipb_attach(device_t parent, device_t self, void *aux)
{
	struct tipb_softc *sc = device_private(self);

	tipb_attached = 1;

#if NOMAPDMAC > 0
#error DMA not implemented
	sc->sc_dmac = &omap_bus_dma_tag;
#else
	sc->sc_dmac = NULL;
#endif

	aprint_normal(": Texas Instruments Peripheral Bus\n");
	aprint_naive("\n");

	/*
	 * There are some devices that need to be set up before all the
	 * others.  The earlies array contains their names.  Find them and
	 * attach them in the order they appear in the array.
	 */
	const char *const *earlyp;
	for (earlyp = earlies; *earlyp != NULL; earlyp++)
		/*
		 * The bus search function is passed an aux argument that
		 * "describes the device that has been found".  The type of it
		 * is void *.  However, I want to pass a constant string, so
		 * use __UNCONST to convince the compiler that this is ok.
		 */
		config_search_ia(tipb_search, self, "tipb",
				 __UNCONST(*earlyp));

	/*
	 * Attach all other devices
	 */
	config_search_ia(tipb_search, self, "tipb", NULL);
}

static int
tipb_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct tipb_softc *sc = device_private(parent);
	struct tipb_attach_args aa;
	const char *const name = (const char *const)aux;

	/* Check whether we're looking for a specifically named device */
	if (name != NULL && strcmp(name, cf->cf_name) != 0)
		return (0);

	switch (cf->cf_loc[TIPBCF_MULT]) {
	case 1:
		aa.tipb_iot = &omap_bs_tag;
		break;
	case 2:
		aa.tipb_iot = &omap_a2x_bs_tag;
		break;
	case 4:
		aa.tipb_iot = &omap_a4x_bs_tag;
		break;
	default:
		panic("Unsupported TIPB multiplier.");
		break;
	}
	aa.tipb_dmac = sc->sc_dmac;
	aa.tipb_addr = cf->cf_loc[TIPBCF_ADDR];
	aa.tipb_size = cf->cf_loc[TIPBCF_SIZE];
	aa.tipb_intr = cf->cf_loc[TIPBCF_INTR];
	aa.tipb_mult = cf->cf_loc[TIPBCF_MULT];

	if (config_match(parent, cf, &aa))
		config_attach(parent, cf, &aa, tipb_print);

	return 0;
}

static int
tipb_print(void *aux, const char *name)
{
	struct tipb_attach_args *sa = (struct tipb_attach_args*)aux;

	if (sa->tipb_addr != TIPBCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%08lx", sa->tipb_addr);
		if (sa->tipb_size > TIPBCF_SIZE_DEFAULT)
			aprint_normal("-0x%08lx", sa->tipb_addr + sa->tipb_size-1);
	}
	if (sa->tipb_intr != TIPBCF_INTR_DEFAULT)
		aprint_normal(" intr %d", sa->tipb_intr);

	return (UNCONF);
}
