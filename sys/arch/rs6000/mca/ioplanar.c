/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ioplanar.c,v 1.1.2.2 2007/12/17 19:09:41 garbled Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/mca/mcavar.h>
#include <dev/mca/mcadevs.h>

#include <rs6000/ioplanar/ioplanarvar.h>

static int	ioplanar_match(struct device *, struct cfdata *, void *);
static void	ioplanar_attach(struct device *, struct device *, void *);
static int	ioplanar_print(void *, const char *);
static int	ioplanar_search(struct device *, struct cfdata *,
				const int *, void *);

CFATTACH_DECL(ioplanar, sizeof(struct ioplanar_softc),
    ioplanar_match, ioplanar_attach, NULL, NULL);

struct ioplanar_softc *ioplanar_softc;
extern struct cfdriver ioplanar_cd;

#define RAINBOW_DEVS	7
int rainbow_map[RAINBOW_DEVS] =
	{ IOP_COM0, IOP_COM1, IOP_LPD, IOP_KBD_2,
	  IOP_TABLET_2, IOP_MOUSE, IOP_FDC_2 };

static int
ioplanar_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mca_attach_args *ma = aux;

	switch (ma->ma_id) {
	case MCA_PRODUCT_IBM_SIO_RAINBOW:
		return 1;
	}

	return 0;
}

static void
ioplanar_attach(struct device *parent, struct device *self, void *aux)
{
	struct ioplanar_softc *sc = (struct ioplanar_softc *)self;
	struct mca_attach_args *ma = aux;

	printf("\n");

	ioplanar_softc = sc;
	sc->sc_ic = ma->ma_mc;
	sc->sc_iot = ma->ma_iot;
	sc->sc_memt = ma->ma_memt;
	sc->sc_dmat = ma->ma_dmat;
	sc->sc_devid = ma->ma_id;

	(void)config_search_ia(ioplanar_search, self, "ioplanar", aux);
}

static int
ioplanar_search(struct device *parent, struct cfdata *cf,
    const int *ldesc, void *aux)
{
	struct ioplanar_dev_attach_args idaa;
	struct mca_attach_args *ma = aux;
	int i;

	idaa.idaa_mc = ma->ma_mc;
	idaa.idaa_iot = ma->ma_iot;
	idaa.idaa_memt = ma->ma_memt;  
	idaa.idaa_dmat = ma->ma_dmat;
	idaa.idaa_devid = ma->ma_id;

	switch (ma->ma_id) {
	case MCA_PRODUCT_IBM_SIO_RAINBOW:
		for (i=0; i < RAINBOW_DEVS; i++) {
			idaa.idaa_device = rainbow_map[i];
			if (config_match(parent, cf, &idaa) > 0)
				config_attach(parent, cf, &idaa,
				    ioplanar_print);
		}
		break;
	default:
		return 0;
	}
	return 0;
}

static int
ioplanar_print(void *args, const char *name)
{
	/*struct ioplanar_dev_attach_args *idaa = args;*/

	aprint_normal(": ");
	return (UNCONF);
}
