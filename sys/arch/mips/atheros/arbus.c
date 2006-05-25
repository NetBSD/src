/* $Id: arbus.c,v 1.5 2006/05/25 03:19:43 gdamore Exp $ */
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
__KERNEL_RCSID(0, "$NetBSD: arbus.c,v 1.5 2006/05/25 03:19:43 gdamore Exp $");

#include "locators.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#define	_MIPS_BUS_DMA_PRIVATE
#include <machine/bus.h>
#include <mips/atheros/include/ar531xreg.h>
#include <mips/atheros/include/ar531xvar.h>
#include <mips/atheros/include/arbusvar.h>

static int arbus_match(struct device *, struct cfdata *, void *);
static void arbus_attach(struct device *, struct device *, void *);
static int arbus_print(void *, const char *);
static void arbus_bus_mem_init(bus_space_tag_t, void *);
static void arbus_dma_init(struct device *, bus_dma_tag_t);

struct arbus_intrhand {
	int		ih_irq;
	int		ih_misc;
	void		*ih_cookie;
};

CFATTACH_DECL(arbus, sizeof(struct device), arbus_match, arbus_attach,
    NULL, NULL);

struct mips_bus_space	arbus_mbst;
struct mips_bus_dma_tag	arbus_mdt;

void
arbus_init(void)
{
	static int done = 0;
	if (done)
		return;
	done++;

	arbus_bus_mem_init(&arbus_mbst, NULL);
	arbus_dma_init(NULL, &arbus_mdt);
}

static struct {
	const char	*name;
	bus_addr_t	addr;
	int		irq;
	uint32_t	mask;
	uint32_t	reset;
	uint32_t	enable;
} arbus_devices[] = {
    {
	    "ae",
	    AR531X_ENET0_BASE,
	    ARBUS_IRQ_ENET0,
	    AR531X_BOARD_CONFIG_ENET0,
	    AR531X_RESET_ENET0 | AR531X_RESET_PHY0,
	    AR531X_ENABLE_ENET0
    },
    {
	    "ae",
	    AR531X_ENET1_BASE,
	    ARBUS_IRQ_ENET1,
	    AR531X_BOARD_CONFIG_ENET1,
	    AR531X_RESET_ENET1 | AR531X_RESET_PHY1,
	    AR531X_ENABLE_ENET1
    },
    {
	    "com",
	    AR531X_UART0_BASE,
	    ARBUS_IRQ_UART0,
	    AR531X_BOARD_CONFIG_UART0,
	    0,
	    0,
    },
    {
	    "com",
	    AR531X_UART1_BASE,
	    -1,
	    AR531X_BOARD_CONFIG_UART1,
	    0,
	    0,
    },
    {
	    "ath",
	    AR531X_WLAN0_BASE,
	    ARBUS_IRQ_WLAN0,
	    AR531X_BOARD_CONFIG_WLAN0,
	    AR531X_RESET_WLAN0 |
	    	AR531X_RESET_WARM_WLAN0_MAC |
	    	AR531X_RESET_WARM_WLAN0_BB,
	    AR531X_ENABLE_WLAN0
    },
    {
	    "ath",
	    AR531X_WLAN1_BASE,
	    ARBUS_IRQ_WLAN1,
	    AR531X_BOARD_CONFIG_WLAN1,
	    AR531X_RESET_WLAN1 |
	    	AR531X_RESET_WARM_WLAN1_MAC |
	    	AR531X_RESET_WARM_WLAN1_BB,
	    AR531X_ENABLE_WLAN1
    },
    {
	    "flash",
	    AR531X_FLASH_BASE,
	    -1,
	    0,
	    0,
	    0,
    },
#if 0
    {
	    "argpio",
	    AR531X_GPIO_BASE,
	    -1,
	    0,
	    0,
	    0
    },
#endif
    { NULL }
};

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
arbus_match(struct device *parent, struct cfdata *match, void *aux)
{

	return 1;
}

void
arbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct arbus_attach_args aa;
	struct ar531x_board_info *info;
	int i;

	printf("\n");
	int locs[ARBUSCF_NLOCS];

	info = ar531x_board_info();
	arbus_init();

	for (i = 0; arbus_devices[i].name; i++) {
		if (arbus_devices[i].mask &&
		    ((arbus_devices[i].mask & info->ab_config) == 0)) {
			continue;
		}
		aa.aa_name = arbus_devices[i].name;
		aa.aa_dmat = &arbus_mdt;
		aa.aa_bst = &arbus_mbst;
		aa.aa_irq = arbus_devices[i].irq;
		aa.aa_addr = arbus_devices[i].addr;

		if (aa.aa_addr < AR531X_UART0_BASE)
			aa.aa_size = 0x00100000;
		else if (aa.aa_addr < AR531X_FLASH_BASE)
			aa.aa_size = 0x1000;

		locs[ARBUSCF_ADDR] = aa.aa_addr;

		if (arbus_devices[i].reset) {
			/* put device into reset */
			PUTSYSREG(AR531X_SYSREG_RESETCTL,
			    GETSYSREG(AR531X_SYSREG_RESETCTL) |
			    arbus_devices[i].reset);

			/* this could probably be a tsleep */
			delay(15000);

			/* take it out of reset */
			PUTSYSREG(AR531X_SYSREG_RESETCTL,
			    GETSYSREG(AR531X_SYSREG_RESETCTL) &
			    ~arbus_devices[i].reset);

			delay(25);
		}

		if (arbus_devices[i].enable) {
			/* enable it */
			PUTSYSREG(AR531X_SYSREG_ENABLE,
			    GETSYSREG(AR531X_SYSREG_ENABLE) |
			    arbus_devices[i].enable);
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
		aprint_normal(" addr 0x%lx", aa->aa_addr);

	if (aa->aa_irq >= 0) {
		aprint_normal(" interrupt %d", ARBUS_IRQ_CPU(aa->aa_irq));

		if (ARBUS_IRQ_MISC(aa->aa_irq))
			aprint_normal(" irq %d", ARBUS_IRQ_MISC(aa->aa_irq));
	}

	return (UNCONF);
}

void *
arbus_intr_establish(int irq, int (*handler)(void *), void *arg)
{
	struct arbus_intrhand	*ih;

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return NULL;

	ih->ih_irq = irq;
	ih->ih_cookie = NULL;

	if (ARBUS_IRQ_MISC(irq)) {
		irq = ARBUS_IRQ_MISC(irq);
		ih->ih_misc = 1;
		ih->ih_cookie = ar531x_misc_intr_establish(irq, handler, arg);
	} else {
		irq = ARBUS_IRQ_CPU(irq);
		ih->ih_misc = 0;
		ih->ih_cookie = ar531x_intr_establish(irq, handler, arg);
	}

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
	if (ih->ih_misc)
		ar531x_misc_intr_disestablish(ih->ih_cookie);
	else
		ar531x_intr_disestablish(ih->ih_cookie);
	free(ih, M_DEVBUF);
}


void
arbus_dma_init(struct device *sc, bus_dma_tag_t pdt)
{
	bus_dma_tag_t	t;

	t = pdt;
	t->_cookie = sc;
	t->_wbase = 0;
	t->_physbase = 0;
	t->_wsize = MIPS_KSEG1_START - MIPS_KSEG0_START;
	t->_dmamap_create = _bus_dmamap_create;
	t->_dmamap_destroy = _bus_dmamap_destroy;
	t->_dmamap_load = _bus_dmamap_load;
	t->_dmamap_load_mbuf = _bus_dmamap_load_mbuf;
	t->_dmamap_load_uio = _bus_dmamap_load_uio;
	t->_dmamap_load_raw = _bus_dmamap_load_raw;
	t->_dmamap_unload = _bus_dmamap_unload;
	t->_dmamap_sync = _bus_dmamap_sync;
	t->_dmamem_alloc = _bus_dmamem_alloc;
	t->_dmamem_free = _bus_dmamem_free;
	t->_dmamem_map = _bus_dmamem_map;
	t->_dmamem_unmap = _bus_dmamem_unmap;
	t->_dmamem_mmap = _bus_dmamem_mmap;
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
