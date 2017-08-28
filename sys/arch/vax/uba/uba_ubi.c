/*	$NetBSD: uba_ubi.c,v 1.3.30.1 2017/08/28 17:51:55 skrll Exp $	   */
/*
 * Copyright (c) 1982, 1986 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

/*
 * Copyright (c) 1996 Jonathan Stone.
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uba_ubi.c,v 1.3.30.1 2017/08/28 17:51:55 skrll Exp $");

#define _VAX_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>

#include <machine/nexus.h>
#include <machine/sgmap.h>

#include <dev/qbus/ubavar.h>

#include <vax/uba/uba_common.h>

#include "locators.h"

/* Some UBI-specific defines */
#define	UBASIZE		((UBAPAGES + UBAIOPAGES) * VAX_NBPG)
#define UMEM730     	(0xfc0000)
#define	UIOPAGE		(UMEM730 + (UBAPAGES * VAX_NBPG))

/*
 * The DW780, DW750 and DW730 are quite similar to their function from
 * a programmers point of view. Differencies are number of BDP's
 * and bus status/command registers, the latter are (partly) IPR's
 * on 750.
 */
static	int	dw730_match(device_t, cfdata_t, void *);
static	void	dw730_attach(device_t, device_t, void *);
static	void	dw730_init(struct uba_softc*);
#ifdef notyet
static	void	dw730_purge(struct uba_softc *, int);
#endif

CFATTACH_DECL_NEW(uba_ubi, sizeof(struct uba_vsoftc),
    dw730_match, dw730_attach, NULL, NULL);

extern	struct vax_bus_space vax_mem_bus_space;

int
dw730_match(device_t parent, cfdata_t cf, void *aux)
{
	struct sbi_attach_args *sa = (struct sbi_attach_args *)aux;

	if (cf->cf_loc[UBICF_TR] != sa->sa_nexnum &&
	    cf->cf_loc[UBICF_TR] != UBICF_TR_DEFAULT)
		return 0;
	/*
	 * The uba type is actually only telling where the uba
	 * space is in nexus space.
	 */
	if ((sa->sa_type & ~3) != NEX_UBA0)
		return 0;

	return 1;
}

void
dw730_attach(device_t parent, device_t self, void *aux)
{
	struct uba_vsoftc *sc = device_private(self);
	struct sbi_attach_args *sa = aux;

	printf(": DW730\n");

	/*
	 * Fill in bus specific data.
	 */
	sc->uv_sc.uh_dev = self;
	sc->uv_sc.uh_ubainit = dw730_init;
#ifdef notyet
	sc->uv_sc.uh_ubapurge = dw730_purge;
#endif
	sc->uv_sc.uh_iot = &vax_mem_bus_space;
	sc->uv_sc.uh_dmat = &sc->uv_dmat;
	sc->uv_sc.uh_type = UBA_UBA;
	sc->uv_sc.uh_nr = sa->sa_type == NEX_UBA1;

	/*
	 * Fill in variables used by the sgmap system.
	 */
	sc->uv_size = UBAPAGES * VAX_NBPG;
	sc->uv_uba = (void *)sa->sa_ioh; /* Map registers is in adaptor */

	uba_dma_init(sc);
	uba_attach(&sc->uv_sc, UIOPAGE);
}

void
dw730_init(struct uba_softc *sc)
{
	mtpr(0, PR_IUR);
	DELAY(500000);
}

#ifdef notyet
void
dw730_purge(struct uba_softc sc, int bdp)
{
	sc->uh_uba->uba_dpr[bdp] |= UBADPR_PURGE | UBADPR_NXM | UBADPR_UCE;
}
#endif
