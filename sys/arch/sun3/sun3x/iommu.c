/*	$NetBSD: iommu.c,v 1.17.40.1 2014/08/20 00:03:26 tls Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iommu.c,v 1.17.40.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/proc.h>

#include <machine/autoconf.h>

#include <sun3/sun3x/iommu.h>
#include <sun3/sun3x/obio.h>

#define IOMMU_SIZE	(IOMMU_NENT * sizeof(iommu_pde_t))

static int  iommu_match(device_t, cfdata_t, void *);
static void iommu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(iommu, 0,
    iommu_match, iommu_attach, NULL, NULL);

static iommu_pde_t *iommu_va;

static int
iommu_match(device_t parent, cfdata_t cf, void *args)
{
	/* This driver only supports one instance. */
	if (iommu_va)
		return 0;

	return 1;
}

static void
iommu_attach(device_t parent, device_t self, void *args)
{
	struct confargs *ca = args;

	iommu_va = (iommu_pde_t *)bus_mapin(ca->ca_bustype, ca->ca_paddr,
	    IOMMU_SIZE);

	aprint_normal("\n");
}

void
iommu_enter(uint32_t sa, uint32_t pa)
{
	int pn;

#ifdef	DIAGNOSTIC
	if (sa > (1<<24))
		panic("%s: bad sa", __func__);
#endif

	/* Convert the slave address into a page index. */
	pn = IOMMU_BTOP(sa);

	/* Mask the physical address to insure it is page aligned */
	pa &= IOMMU_PDE_PA;
	pa |= IOMMU_PDE_DT_VALID;

	iommu_va[pn].addr.raw = pa;
}

void
iommu_remove(uint32_t sa, uint32_t len)
{
	int pn;

#ifdef	DIAGNOSTIC
	if (sa > (1<<24))
		panic("%s: bad sa", __func__);
#endif

	pn = IOMMU_BTOP(sa);
	while (len > 0) {
		iommu_va[pn++].addr.raw = IOMMU_PDE_DT_INVALID;
		len -= IOMMU_PAGE_SIZE;
	}
}
