/* $NetBSD: adm5120_obio.c,v 1.1.62.2 2010/01/14 00:40:35 matt Exp $ */

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm5120_obio.c,v 1.1.62.2 2010/01/14 00:40:35 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/cache.h>
#include <mips/cpuregs.h>

#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>
#include <mips/adm5120/include/adm5120_mainbusvar.h>
#include <mips/adm5120/include/adm5120_obiovar.h>

#include "locators.h"

#ifdef OBIO_DEBUG
int obio_debug = 1;
#define	OBIO_DPRINTF(__fmt, ...)		\
do {						\
	if (obio_debug)			\
		printf((__fmt), __VA_ARGS__);	\
} while (/*CONSTCOND*/0)
#else /* !OBIO_DEBUG */
#define	OBIO_DPRINTF(__fmt, ...)	do { } while (/*CONSTCOND*/0)
#endif /* OBIO_DEBUG */

static int	obio_match(device_t, cfdata_t, void *);
static void	obio_attach(device_t, device_t, void *);
static int	obio_submatch(device_t, cfdata_t, const int *, void *);
static int	obio_print(void *, const char *);

CFATTACH_DECL_NEW(obio, 0, obio_match, obio_attach, NULL, NULL);

/* There can be only one. */
int	obio_found;

struct obiodev {
	const char	*od_name;
	bus_addr_t	od_addr;
	int		od_irq;
	uint32_t	od_gpio_mask;
};

const struct obiodev obiodevs[] = {
	{"uart",	ADM5120_BASE_UART0,	1, 0x0},
	{"uart",	ADM5120_BASE_UART1,	2, 0x0},
	{"admsw",	ADM5120_BASE_SWITCH,	9, 0x0},
	{"ahci",	ADM5120_BASE_USB,	3, 0x0},
	{"admflash",	ADM5120_BASE_SRAM0,	0, 0x0},
	{NULL,		0,			0, 0x0},
};

static int
obio_match(device_t parent, cfdata_t match, void *aux)
{
	return !obio_found;
}

static void
obio_attach_args_create(struct obio_attach_args *oa, const struct obiodev *od,
    void *gpio, bus_dma_tag_t dmat, bus_space_tag_t st)
{
	oa->oba_name = od->od_name;
	oa->oba_addr = od->od_addr;
	oa->oba_irq = od->od_irq;
	oa->oba_dt = dmat;
	oa->oba_st = st;
	oa->oba_gpio = gpio;
	oa->oba_gpio_mask = od->od_gpio_mask;
}

static void
obio_attach(device_t parent, device_t self, void *aux)
{
	struct mainbus_attach_args *ma = (struct mainbus_attach_args *)aux;
	struct obio_attach_args oa;
	const struct obiodev *od;

	obio_found = 1;
	printf("\n");

	OBIO_DPRINTF("%s: %d\n", __func__, __LINE__);

	OBIO_DPRINTF("%s: %d\n", __func__, __LINE__);
	for (od = obiodevs; od->od_name != NULL; od++) {
		OBIO_DPRINTF("%s: %d\n", __func__, __LINE__);
		obio_attach_args_create(&oa, od, ma->ma_gpio, ma->ma_dmat,
		    ma->ma_obiot);
		OBIO_DPRINTF("%s: %d\n", __func__, __LINE__);
		(void)config_found_sm_loc(self, "obio", NULL, &oa, obio_print,
		    obio_submatch);
	}
	OBIO_DPRINTF("%s: %d\n", __func__, __LINE__);
}

static int
obio_submatch(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct obio_attach_args *oa = aux;

	if (cf->cf_loc[OBIOCF_ADDR] != OBIOCF_ADDR_DEFAULT &&
	    cf->cf_loc[OBIOCF_ADDR] != oa->oba_addr)
		return 0;

	return config_match(parent, cf, aux);
}

static int
obio_print(void *aux, const char *pnp)
{
	struct obio_attach_args *oa = aux;

	if (pnp != NULL)
		aprint_normal("%s at %s", oa->oba_name, pnp);
	if (oa->oba_addr != OBIOCF_ADDR_DEFAULT)
		aprint_normal(" addr 0x%"PRIxBUSADDR, oa->oba_addr);
	if (oa->oba_gpio_mask != OBIOCF_GPIO_MASK_DEFAULT)
		aprint_normal(" gpio_mask 0x%02x", oa->oba_gpio_mask);

	return UNCONF;
}
