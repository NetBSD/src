/*	$NetBSD: obio.c,v 1.8 2003/07/15 02:54:50 lukem Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
__KERNEL_RCSID(0, "$NetBSD: obio.c,v 1.8 2003/07/15 02:54:50 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>

#include <machine/bus.h>
#include <machine/pio.h>
#include <machine/intr.h>
#include <machine/platform.h>

#include <prep/dev/obiovar.h>

#include <dev/isa/isareg.h>

#include "wdc_obio.h"

static int obio_found = 0;

static int	obio_match(struct device *, struct cfdata *, void *);
static void	obio_attach(struct device *, struct device *, void *);
static int	obio_print(void *, const char *);
static int	obio_search(struct device *, struct cfdata *, void *);

CFATTACH_DECL(obio, sizeof(struct device),
    obio_match, obio_attach, NULL, NULL);

extern struct cfdriver obio_cd;

static int
obio_match(struct device *parent, struct cfdata *cf, void *aux)
{

	if (obio_found)
		return 0;
	return 1;
}

static void
obio_attach(struct device *parent, struct device *self, void *aux)
{

	obio_found = 1;

	printf("\n");

	(void)config_search(obio_search, self, aux);
}

static int
obio_search(struct device *parent, struct cfdata *cf, void *aux)
{
	struct obio_attach_args oa;
	const char **p;

	p = platform->obiodevs;
	if (p == NULL)
		return 0;

	for (; *p != NULL; p++) {
		if (strcmp(cf->cf_name, *p) == 0) {
			oa.oa_iot = &prep_isa_io_space_tag;
			oa.oa_memt = &prep_isa_mem_space_tag;
			oa.oa_iobase = cf->cf_iobase;
			oa.oa_iosize = cf->cf_iosize;
			oa.oa_maddr = cf->cf_maddr;
			oa.oa_msize = cf->cf_msize;
			oa.oa_irq = cf->cf_irq == 2 ? 9 : cf->cf_irq;

			if (config_match(parent, cf, &oa) > 0)
				config_attach(parent, cf, &oa, obio_print);
		}
	}

	return 0;
}

static int
obio_print(void *args, const char *name)
{
	struct obio_attach_args *oa = args;

	if (oa->oa_iosize)
		aprint_normal(" port 0x%x", oa->oa_iobase);
	if (oa->oa_iosize > 1)
		aprint_normal("-0x%x", oa->oa_iobase + oa->oa_iosize - 1);
	if (oa->oa_msize)
		aprint_normal(" mem 0x%x", oa->oa_maddr);
	if (oa->oa_msize > 1)
		aprint_normal("-0x%x", oa->oa_maddr + oa->oa_msize - 1);
	if (oa->oa_irq != IRQUNK)
		aprint_normal(" irq %d", oa->oa_irq);
	return (UNCONF);
}

/*
 * Set up an interrupt handler to start being called.
 */
void *
obio_intr_establish(int irq, int type, int level, int (*ih_fun)(void *),
    void *ih_arg)
{

	return (void *)intr_establish(irq, type, level, ih_fun, ih_arg);
}

/*
 * Deregister an interrupt handler.
 */
void
obio_intr_disestablish(void *arg)
{

	intr_disestablish(arg);
}

/*
 * obio bus resource mapping
 */
#if NWDC_OBIO > 0
static bus_space_handle_t wdc0_cmd;
static bus_space_handle_t wdc0_ctl;
static bus_space_handle_t wdc1_cmd;
static bus_space_handle_t wdc1_ctl;
#endif

void
obio_reserve_resource_map(void)
{
	const char **p;

	for (p = platform->obiodevs; *p != NULL; p++) {
#if NWDC_OBIO > 0
		if (strcmp(*p, "wdc") == 0) {
			bus_space_map(&prep_isa_io_space_tag, IO_WD1, 8,
			    0, &wdc0_cmd);
			bus_space_map(&prep_isa_io_space_tag, IO_WD1 + 0x206, 1,
			    0, &wdc0_ctl);
			bus_space_map(&prep_isa_io_space_tag, IO_WD2, 8,
			    0, &wdc1_cmd);
			bus_space_map(&prep_isa_io_space_tag, IO_WD2 + 0x206, 1,
			    0, &wdc1_ctl);
		}
#endif
	}
}

void
obio_reserve_resource_unmap(void)
{
	const char **p;

	for (p = platform->obiodevs; *p != NULL; p++) {
#if NWDC_OBIO > 0
		if (strcmp(*p, "wdc") == 0) {
			bus_space_unmap(&prep_isa_io_space_tag, wdc0_cmd, 8);
			bus_space_unmap(&prep_isa_io_space_tag, wdc0_ctl, 1);
			bus_space_unmap(&prep_isa_io_space_tag, wdc1_cmd, 8);
			bus_space_unmap(&prep_isa_io_space_tag, wdc1_ctl, 1);
		}
#endif
	}
}
