/*	$NetBSD: iommu.c,v 1.5 1997/04/25 18:45:39 gwr Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeremy Cooper.
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
 * Set-up functions for the Sun3x DVMA I/O Mapper.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/autoconf.h>
#include <machine/obio.h>
#include "iommu.h"

#define IOMMU_SIZE	(IOMMU_NENT * sizeof(iommu_pde_t))

static int  iommu_match __P((struct device *, struct cfdata *, void *));
static void iommu_attach __P((struct device *, struct device *, void *));

struct cfattach iommu_ca = {
	sizeof(struct device), iommu_match, iommu_attach
};

struct cfdriver iommu_cd = {
	NULL, "iommu", DV_DULL
};

static iommu_pde_t *iommu_va;

static int
iommu_match(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{

	/* This driver only supports one unit. */
	if (cf->cf_unit != 0)
		return (0);

	return (1);
}

static void
iommu_attach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct confargs *ca = args;

	iommu_va = (iommu_pde_t *)
		bus_mapin(ca->ca_bustype, ca->ca_paddr, IOMMU_SIZE);

	printf(": range %dMB\n", IOMMU_NENT * IOMMU_PAGE_SIZE / (1024 * 1024));
}

void
iommu_enter(sa, pa)
	u_int32_t sa;	/* slave address */
	u_int32_t pa;	/* phys. address */
{
	int pn;

#ifdef	DIAGNOSTIC
	if (sa > (1<<24))
		panic("iommu_enter: bad sa");
#endif

	/* Convert the slave address into a page index. */
	pn = IOMMU_BTOP(sa);

	/* Mask the physical address to insure it is page aligned */
	pa &= IOMMU_PDE_PA;
	pa |= IOMMU_PDE_DT_VALID;

	iommu_va[pn].addr.raw = pa;
}

void
iommu_remove(sa, len)
	u_int32_t sa;	/* slave address */
	u_int32_t len;
{
	int pn;

#ifdef	DIAGNOSTIC
	if (sa > (1<<24))
		panic("iommu_enter: bad sa");
#endif

	pn = IOMMU_BTOP(sa);
	while (len > 0) {
		iommu_va[pn++].addr.raw = IOMMU_PDE_DT_INVALID;
		len -= IOMMU_PAGE_SIZE;
	}
}
