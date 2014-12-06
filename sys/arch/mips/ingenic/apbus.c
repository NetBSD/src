/*	$NetBSD: apbus.c,v 1.1 2014/12/06 14:34:56 macallan Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
 
/* catch-all for on-chip peripherals */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apbus.c,v 1.1 2014/12/06 14:34:56 macallan Exp $");

#include "locators.h"
#define	_MIPS_BUS_DMA_PRIVATE

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/systm.h>

#include <mips/ingenic/ingenic_var.h>
#include <mips/ingenic/ingenic_regs.h>

static int apbus_match(device_t, cfdata_t, void *);
static void apbus_attach(device_t, device_t, void *);
static int apbus_print(void *, const char *);
static void apbus_bus_mem_init(bus_space_tag_t, void *);

CFATTACH_DECL_NEW(apbus, 0, apbus_match, apbus_attach, NULL, NULL);

static struct mips_bus_space	apbus_mbst;
bus_space_tag_t	apbus_memt = NULL;

struct mips_bus_dma_tag	apbus_dmat = {
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};

static const char *apbus_devs[] = {
	"dwctwo",
	"jzgpio",
	"jzfb",
	NULL
};

void
apbus_init(void)
{
	static bool done = false;
	if (done)
		return;
	done = true;

	apbus_bus_mem_init(&apbus_mbst, NULL);
	apbus_memt = &apbus_mbst;
}

int
apbus_match(device_t parent, cfdata_t match, void *aux)
{
	struct mainbusdev {
		const char *md_name;
	} *aa = aux;
	if (strcmp(aa->md_name, "apbus") == 0) return 1;
	return 0;
}

void
apbus_attach(device_t parent, device_t self, void *aux)
{
	aprint_normal("\n");

	/* should have been called early on */
	apbus_init();

printf("core ctrl:   %08x\n", MFC0(12, 2));
printf("core status: %08x\n", MFC0(12, 3));
printf("REIM: %08x\n", MFC0(12, 4));
printf("ID: %08x\n", MFC0(15, 1));

	for (const char **adv = apbus_devs; *adv != NULL; adv++) {
		struct apbus_attach_args aa;
		aa.aa_name = *adv;
		aa.aa_addr = 0;
		aa.aa_dmat = &apbus_dmat;
		aa.aa_bst = apbus_memt;

		(void) config_found_ia(self, "apbus", &aa, apbus_print);
	}
}

int
apbus_print(void *aux, const char *pnp)
{
	struct apbus_attach_args *aa = aux;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);

	if (aa->aa_addr)
		aprint_normal(" addr 0x%" PRIxBUSADDR, aa->aa_addr);

	return (UNCONF);
}

#define CHIP	   		apbus
#define	CHIP_MEM		/* defined */
#define	CHIP_W1_BUS_START(v)	0x10000000UL
#define CHIP_W1_BUS_END(v)	0x20000000UL
#define	CHIP_W1_SYS_START(v)	0x10000000UL
#define	CHIP_W1_SYS_END(v)	0x20000000UL

#include <mips/mips/bus_space_alignstride_chipdep.c>
