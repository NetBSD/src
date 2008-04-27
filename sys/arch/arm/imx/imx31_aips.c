/*	$Id: imx31_aips.c,v 1.2 2008/04/27 18:58:44 matt Exp $	*/

/* derived from:	*/
/*	$NetBSD: imx31_aips.c,v 1.2 2008/04/27 18:58:44 matt Exp $ */

/*
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
 *
 * Autoconfiguration support for the Intel PXA2[15]0 application
 * processor. This code is derived from arm/sa11x0/sa11x0.c
 */

/*-
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
 */
/*-
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$Id: imx31_aips.c,v 1.2 2008/04/27 18:58:44 matt Exp $");

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

#include <machine/intr.h>

#include <arm/imx/imx31var.h>
#include <arm/imx/imxuartreg.h>
#include <arm/imx/imxuartvar.h>

struct imxaips_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bust;
};

/* prototypes */
static int	imxaips_match(device_t , cfdata_t, void *);
static void	imxaips_attach(device_t , device_t , void *);
static int 	imxaips_search(device_t , cfdata_t, const int *, void *);
static int	imxaips_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(aips, sizeof(struct imxaips_softc),
    imxaips_match, imxaips_attach, NULL, NULL);

static int
imxaips_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
imxaips_attach(device_t parent, device_t self, void *aux)
{
	struct imxaips_softc * const sc = (struct imxaips_softc *)self;
	struct ahb_attach_args * const ahba = aux;

	sc->sc_bust = ahba->ahba_memt;

	aprint_normal(": imx31 AHB-Lite 2.v6 to IP bus interface\n");

	/*
	 * Attach all other devices
	 */
	config_search_ia(imxaips_search, self, "aips", sc);
}

static int
imxaips_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct imxaips_softc * const sc = aux;
	struct aips_attach_args aipsa;

	aipsa.aipsa_name = "aisp";
	aipsa.aipsa_memt = sc->sc_bust;
	aipsa.aipsa_addr = cf->cf_loc[AIPSCF_ADDR];
	aipsa.aipsa_size = cf->cf_loc[AIPSCF_SIZE];
	aipsa.aipsa_intr = cf->cf_loc[AIPSCF_INTR];

	if (config_match(parent, cf, &aipsa))
		config_attach(parent, cf, &aipsa, imxaips_print);

	return 0;
}

static int
imxaips_print(void *aux, const char *name)
{
	struct aips_attach_args *aipsa = aux;

	if (aipsa->aipsa_addr != AIPSCF_ADDR_DEFAULT) {
		aprint_normal(" addr 0x%lx", aipsa->aipsa_addr);
		if (aipsa->aipsa_size > AIPSCF_SIZE_DEFAULT)
			aprint_normal("-0x%lx",
			    aipsa->aipsa_addr + aipsa->aipsa_size-1);
	}
	if (aipsa->aipsa_intr != AIPSCF_INTR_DEFAULT)
		aprint_normal(" intr %d", aipsa->aipsa_intr);

	return (UNCONF);
}
