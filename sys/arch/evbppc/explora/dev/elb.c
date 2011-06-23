/*	$NetBSD: elb.c,v 1.6.32.1 2011/06/23 14:19:08 cherry Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Juergen Hannken-Illjes.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: elb.c,v 1.6.32.1 2011/06/23 14:19:08 cherry Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/extent.h>

#include <machine/explora.h>
#define _POWERPC_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <powerpc/ibm4xx/dcr403cgx.h>

#include <evbppc/explora/dev/elbvar.h>

struct elb_dev {
	const char *elb_name;
	int elb_addr;
	int elb_addr2;
	int elb_irq;
	bus_space_tag_t elb_bt;
};

static int	elb_match(device_t, cfdata_t, void *);
static void	elb_attach(device_t, device_t, void *);
static int	elb_print(void *, const char *);

static struct powerpc_bus_space elb_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE | 1,	/* stride 1 */
	0x00000000,
	BASE_PCKBC,
	BASE_PCKBC + 0x6ff
};
static struct powerpc_bus_space elb_fb_tag = {
	_BUS_SPACE_LITTLE_ENDIAN | _BUS_SPACE_MEM_TYPE,
	0x00000000,
	BASE_FB,
	BASE_FB2 + SIZE_FB - 1
};

static char elb_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));
static char elbfb_ex_storage[EXTENT_FIXED_STORAGE_SIZE(8)]
    __attribute__((aligned(8)));

static bool elb_tag_init_done;
static bool elbfb_tag_init_done;

/*
 * DMA struct, nothing special.
 */
static struct powerpc_bus_dma_tag elb_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create, 
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dma_phys_to_bus_mem_generic,
	_bus_dma_bus_mem_to_phys_generic,
};

static const struct elb_dev elb_devs[] = {
	{ "cpu",	0,		0,		-1, NULL },
	{ "pckbc",	BASE_PCKBC,	BASE_PCKBC2,	31, &elb_tag },
	{ "com",	BASE_COM,	0,		30, &elb_tag },
	{ "lpt",	BASE_LPT,	0,		-1, &elb_tag },
	{ "fb",		BASE_FB,	BASE_FB2,	-1, &elb_fb_tag },
	{ "le",		BASE_LE,	0,		28, &elb_fb_tag },
};

CFATTACH_DECL_NEW(elb, 0,
    elb_match, elb_attach, NULL, NULL);

/*
 * Probe for the elb; always succeeds.
 */
static int
elb_match(device_t parent, cfdata_t cf, void *aux)
{
	return (1);
}

/*
 * Attach the Explora local bus.
 */
static void
elb_attach(device_t parent, device_t self, void *aux)
{
	struct elb_attach_args eaa;
	const struct elb_dev *elb;
	size_t i;

	printf("\n");
	for (i = 0, elb = elb_devs; i < __arraycount(elb_devs); i++, elb++) {
		eaa.elb_name = elb->elb_name;
		eaa.elb_bt = elb_get_bus_space_tag(elb->elb_addr);
		eaa.elb_dmat = &elb_bus_dma_tag;
		eaa.elb_base = elb->elb_addr;
		eaa.elb_base2 = elb->elb_addr2;
		eaa.elb_irq = elb->elb_irq;

		(void) config_found(self, &eaa, elb_print);
	}
}

static int
elb_print(void *aux, const char *pnp)
{
	struct elb_attach_args *eaa = aux;

	if (pnp)
		aprint_normal("%s at %s", eaa->elb_name, pnp);
	if (eaa->elb_irq != -1)
		aprint_normal(" irq %d", eaa->elb_irq);

	return (UNCONF);
}

bus_space_tag_t
elb_get_bus_space_tag(bus_addr_t addr)
{

	if ((addr & 0xff000000) == 0x74000000) {
		if (!elb_tag_init_done) {
			if (bus_space_init(&elb_tag, "elbtag",
			    elb_ex_storage, sizeof(elb_ex_storage)))
				panic("elb_get_bus_space_tag: elb_tag");

			elb_tag_init_done = true;
		}
		return (&elb_tag);
	} else {
		if (!elbfb_tag_init_done) {
			if (bus_space_init(&elb_fb_tag, "elbfbtag",
			    elbfb_ex_storage, sizeof(elbfb_ex_storage)))
				panic("elb_get_bus_space_tag: elb_fb_tag");

			elbfb_tag_init_done = true;
		}
		return (&elb_fb_tag);
	}
}
