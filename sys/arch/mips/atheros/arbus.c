/* $Id: arbus.c,v 1.10.80.1 2010/01/10 02:48:46 matt Exp $ */
/*
 * Copyright (c) 2006 Urbana-Champaign Independent Media Center.
 * Copyright (c) 2006 Garrett D'Amore.
 * All rights reserved.
 *
 * This code was written by Garrett D'Amore for the Champaign-Urbana
 * Community Wireless Network Project.
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgements:
 *      This product includes software developed by the Urbana-Champaign
 *      Independent Media Center.
 *	This product includes software developed by Garrett D'Amore.
 * 4. Urbana-Champaign Independent Media Center's name and Garrett
 *    D'Amore's name may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER AND GARRETT D'AMORE ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE URBANA-CHAMPAIGN INDEPENDENT
 * MEDIA CENTER OR GARRETT D'AMORE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: arbus.c,v 1.10.80.1 2010/01/10 02:48:46 matt Exp $");

#include "locators.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define	_MIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <mips/atheros/include/ar5312reg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

static int arbus_match(device_t, struct cfdata *, void *);
static void arbus_attach(device_t, device_t, void *);
static int arbus_print(void *, const char *);
static void arbus_bus_mem_init(bus_space_tag_t, void *);

struct arbus_intrhand {
	int		ih_cirq;
	int		ih_mirq;
	void		*ih_cookie;
};

CFATTACH_DECL_NEW(arbus, 0, arbus_match, arbus_attach, NULL, NULL);

struct mips_bus_space	arbus_mbst;
struct mips_bus_dma_tag	arbus_mdt = {
	._dmamap_ops = _BUS_DMAMAP_OPS_INITIALIZER,
	._dmamem_ops = _BUS_DMAMEM_OPS_INITIALIZER,
	._dmatag_ops = _BUS_DMATAG_OPS_INITIALIZER,
};

void
arbus_init(void)
{
	static bool done = false;
	if (done)
		return;
	done = true;

	arbus_bus_mem_init(&arbus_mbst, NULL);
}

/* this primarily exists so we can get to the console... */
bus_space_tag_t
arbus_get_bus_space_tag(void)
{
	arbus_init();
	return (&arbus_mbst);
}

bus_dma_tag_t
arbus_get_bus_dma_tag(void)
{
	arbus_init();
	return (&arbus_mdt);
}

int
arbus_match(device_t parent, cfdata_t match, void *aux)
{

	return 1;
}

void
arbus_attach(device_t parent, device_t self, void *aux)
{
	struct arbus_attach_args aa;
	const struct ar531x_device *devices;
	int i;

	printf("\n");
	int locs[ARBUSCF_NLOCS];

	arbus_init();

	for (i = 0, devices = ar531x_get_devices(); devices[i].name; i++) {

		aa.aa_name = devices[i].name;
		aa.aa_size = devices[i].size;
		aa.aa_dmat = &arbus_mdt;
		aa.aa_bst = &arbus_mbst;
		aa.aa_cirq = devices[i].cirq;
		aa.aa_mirq = devices[i].mirq;
		aa.aa_addr = devices[i].addr;

		locs[ARBUSCF_ADDR] = aa.aa_addr;

		if (ar531x_enable_device(&devices[i]) != 0) {
			continue;
		}

		(void) config_found_sm_loc(self, "arbus", locs, &aa,
		    arbus_print, config_stdsubmatch);
	}
}

int
arbus_print(void *aux, const char *pnp)
{
	struct arbus_attach_args *aa = aux;

	if (pnp)
		aprint_normal("%s at %s", aa->aa_name, pnp);

	if (aa->aa_addr)
		aprint_normal(" addr 0x%" PRIxBUSADDR, aa->aa_addr);

	if (aa->aa_cirq >= 0)
		aprint_normal(" cpu irq %d", aa->aa_cirq);

	if (aa->aa_mirq >= 0)
		aprint_normal(" misc irq %d", aa->aa_mirq);

	return (UNCONF);
}

void *
arbus_intr_establish(int cirq, int mirq, int (*handler)(void *), void *arg)
{
	struct arbus_intrhand	*ih;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return NULL;

	ih->ih_cirq = ih->ih_mirq = -1;
	ih->ih_cookie = NULL;

	if (mirq >= 0) {
		ih->ih_mirq = mirq;
		ih->ih_cookie = ar531x_misc_intr_establish(mirq, handler, arg);
	} else if (cirq >= 0) {
		ih->ih_cirq = cirq;
		ih->ih_cookie = ar531x_cpu_intr_establish(cirq, handler, arg);
	} else
		return ih;

	if (ih->ih_cookie == NULL) {
		free(ih, M_DEVBUF);
		return NULL;
	}
	return ih;
}

void
arbus_intr_disestablish(void *arg)
{
	struct arbus_intrhand	*ih = arg;
	if (ih->ih_mirq >= 0)
		ar531x_misc_intr_disestablish(ih->ih_cookie);
	else if (ih->ih_cirq >= 0)
		ar531x_cpu_intr_disestablish(ih->ih_cookie);
	free(ih, M_DEVBUF);
}

/*
 * CPU memory/register stuff
 */

#define CHIP	   		arbus
#define	CHIP_MEM		/* defined */
#define	CHIP_W1_BUS_START(v)	0x00000000UL
#define CHIP_W1_BUS_END(v)	0x1fffffffUL
#define	CHIP_W1_SYS_START(v)	CHIP_W1_BUS_START(v)
#define	CHIP_W1_SYS_END(v)	CHIP_W1_BUS_END(v)

#include <mips/mips/bus_space_alignstride_chipdep.c>
