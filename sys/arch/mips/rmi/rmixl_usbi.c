/*	$NetBSD: rmixl_usbi.c,v 1.1.2.1 2009/12/14 07:21:41 cliff Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Cliff Neighbors
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
__KERNEL_RCSID(0, "$NetBSD: rmixl_usbi.c,v 1.1.2.1 2009/12/14 07:21:41 cliff Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_obiovar.h>
#include <mips/rmi/rmixl_usbivar.h>

#include <dev/usb/usb.h>   
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usb_mem.h>

/*
 * USB I/O register byte order is
 * LITTLE ENDIAN regardless of code model
 */
#define RMIXL_USBI_GEN_VADDR(o)	\
	(volatile uint32_t *)MIPS_PHYS_TO_KSEG1(	\
		rmixl_configuration.rc_io_pbase + RMIXL_IO_DEV_USB_B + (o))
#define RMIXL_USBI_GEN_READ(o)     le32toh(*RMIXL_USBI_GEN_VADDR(o))
#define RMIXL_USBI_GEN_WRITE(o,v)  *RMIXL_USBI_GEN_VADDR(o) = htole32(v)

static const char *rmixl_usbi_intrnames[RMIXL_UB_INTERRUPT_MAX+1] = {
	"int 0 (ohci0)",
	"int 1 (ohci1)",
	"int 2 (ehci)",
	"int 3 (device)",
	"int 4 (phy)",
	"int 5 (force)"
};

typedef struct rmixl_usbi_dispatch {
	int (*func)(void *);
	void *arg; 
	struct evcnt count;
} rmixl_usbi_dispatch_t;

typedef struct rmixl_usbi_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_eb_bst;
	bus_space_tag_t		sc_el_bst;
	bus_addr_t		sc_addr;
	bus_size_t		sc_size;
	bus_dma_tag_t		sc_dmat;
	rmixl_usbi_dispatch_t	sc_dispatch[RMIXL_UB_INTERRUPT_MAX + 1];
} rmixl_usbi_softc_t;


static int	rmixl_usbi_match(device_t, cfdata_t, void *);
static void	rmixl_usbi_attach(device_t, device_t, void *);
static int  	rmixl_usbi_print(void *, const char *);
static int  	rmixl_usbi_search(device_t, cfdata_t, const int *, void *);
static int      rmixl_usbi_intr(void *);

#ifdef RMIXL_USBI_DEBUG
int	rmixl_usbi_rdump(void);
rmixl_usbi_softc_t *rmixl_usbi_sc;
#endif


CFATTACH_DECL_NEW(rmixl_usbi, sizeof (rmixl_usbi_softc_t),
    rmixl_usbi_match, rmixl_usbi_attach, NULL, NULL);

int
rmixl_usbi_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

        if (obio->obio_addr == RMIXL_IO_DEV_USB_B)
                return 1;

        return 0;
}

void
rmixl_usbi_attach(device_t parent, device_t self, void *aux)
{
	rmixl_usbi_softc_t *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	void *ih;

#ifdef RMIXL_USBI_DEBUG
	rmixl_usbi_sc = sc;
#endif
	sc->sc_dev = self;
	sc->sc_eb_bst = obio->obio_eb_bst;
	sc->sc_el_bst = obio->obio_el_bst;
	sc->sc_addr = obio->obio_addr;
	sc->sc_size = obio->obio_size;
	sc->sc_dmat = obio->obio_29bit_dmat;	/* XXX */

	for (int intr=0; intr <= RMIXL_UB_INTERRUPT_MAX; intr++) {
		evcnt_attach_dynamic(&sc->sc_dispatch[intr].count,
			EVCNT_TYPE_INTR, NULL, "rmixl_usbi",
				rmixl_usbi_intrnames[intr]);
	}

	/* Disable all usb interface interrupts */
	RMIXL_USBI_GEN_WRITE(RMIXL_USB_INTERRUPT_ENABLE, 0);

	/* establish interrupt */
	ih = rmixl_intr_establish(obio->obio_intr, IPL_USB,
                RMIXL_INTR_LEVEL, RMIXL_INTR_HIGH, rmixl_usbi_intr, sc);
	if (ih == NULL)
		panic("%s: couldn't establish interrupt", device_xname(self));

	aprint_normal("\n");

	/* attach any children */
	config_search_ia(rmixl_usbi_search, self, "rmixl_usbi", NULL);
}

static int
rmixl_usbi_print(void *aux, const char *pnp)
{
	struct rmixl_usbi_attach_args *usbi = aux;

	if (usbi->usbi_addr != RMIXL_USBICF_ADDR_DEFAULT) {
		aprint_normal(" addr %#"PRIxBUSADDR, usbi->usbi_addr);
		if (usbi->usbi_size != RMIXL_USBICF_SIZE_DEFAULT)
			aprint_normal("-%#"PRIxBUSADDR,
				usbi->usbi_addr + (usbi->usbi_size - 1));
	}
	if (usbi->usbi_intr != RMIXL_USBICF_INTR_DEFAULT)
		aprint_normal(" intr %d", usbi->usbi_intr);

	aprint_normal("\n");

	return (UNCONF);
}

static int
rmixl_usbi_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct rmixl_usbi_softc *sc = device_private(parent);
	struct rmixl_usbi_attach_args usbi;

	usbi.usbi_eb_bst = sc->sc_eb_bst;
	usbi.usbi_el_bst = sc->sc_el_bst;
	usbi.usbi_addr = cf->cf_loc[RMIXL_USBICF_ADDR];
	usbi.usbi_size = cf->cf_loc[RMIXL_USBICF_SIZE];
	usbi.usbi_intr = cf->cf_loc[RMIXL_USBICF_INTR];
	usbi.usbi_dmat = sc->sc_dmat;

	if (config_match(parent, cf, &usbi) > 0)
		config_attach(parent, cf, &usbi, rmixl_usbi_print);

	return 0;
}


void
rmixl_usbi_intr_disestablish(void *uh, void *ih)
{
	rmixl_usbi_softc_t *sc = uh;
	u_int intr;

	for (intr=0; intr <= RMIXL_UB_INTERRUPT_MAX; intr++) {
		if (ih == &sc->sc_dispatch[intr]) {
			uint32_t r;

			/* disable this interrupt in the usb interface */
			r = RMIXL_USBI_GEN_READ(RMIXL_USB_INTERRUPT_ENABLE);
			r &= 1 << intr;
			RMIXL_USBI_GEN_WRITE(RMIXL_USB_INTERRUPT_ENABLE, r);

			/* free the dispatch slot */
			sc->sc_dispatch[intr].func = NULL;
			sc->sc_dispatch[intr].arg = NULL;

			break;
		}
	}
}

void *
rmixl_usbi_intr_establish(void *uh, u_int intr, int (func)(void *), void *arg)
{
	rmixl_usbi_softc_t *sc = uh;
	uint32_t r;
	void *ih = NULL;
	int s;

	s = splusb();

	if (intr > RMIXL_UB_INTERRUPT_MAX) {
		aprint_error_dev(sc->sc_dev, "invalid intr %d\n", intr);
		goto out;
	}

	if (sc->sc_dispatch[intr].func != NULL) {
		aprint_error_dev(sc->sc_dev, "intr %dq busy\n", intr);
		goto out;
	}

	sc->sc_dispatch[intr].func = func;
	sc->sc_dispatch[intr].arg = arg;
	ih = &sc->sc_dispatch[intr];

	/* enable this interrupt in the usb interface */
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_INTERRUPT_ENABLE);
	r |= 1 << intr;
	RMIXL_USBI_GEN_WRITE(RMIXL_USB_INTERRUPT_ENABLE, r);

 out:
	splx(s);
	return ih;
}

static int
rmixl_usbi_intr(void *arg)
{
	rmixl_usbi_softc_t *sc = arg;
	uint32_t r;
	int intr;
	int rv = 0;

	r = RMIXL_USBI_GEN_READ(RMIXL_USB_INTERRUPT_STATUS);
	if (r != 0) {
		for (intr=0; intr <= RMIXL_UB_INTERRUPT_MAX; intr++) {
			uint32_t bit = 1 << intr;
			if ((r & bit) != 0) {
				int (*f)(void *) = sc->sc_dispatch[intr].func;
				void *a = sc->sc_dispatch[intr].arg;
				if (f != NULL) {
					(void)(*f)(a);
					sc->sc_dispatch[intr].count.ev_count++;
					rv = 1;
				}
			}
		}
	}

	return rv;
}

#ifdef RMIXL_USBI_DEBUG
int
rmixl_usbi_rdump(void)
{
	rmixl_usbi_softc_t *sc = rmixl_usbi_sc;
	uint32_t r;

	if (sc == NULL)
		return -1;

	printf("\n%s:\n", __func__);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_GEN_CTRL1);
	printf(" USB_GEN_CTRL1 %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_GEN_CTRL2);
	printf(" USB_GEN_CTRL2 %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_GEN_CTRL3);
	printf(" USB_GEN_CTRL3 %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_IOBM_TIMER);
	printf(" USB_IOBM_TIMER %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_VBUS_TIMER);
	printf(" USB_VBUS_TIMER %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_BYTESWAP_EN);
	printf(" USB_BYTESWAP_EN %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_COHERENT_MEM_BASE);
	printf(" USB_COHERENT_MEM_BASE %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_COHERENT_MEM_LIMIT);
	printf(" USB_COHERENT_MEM_LIMIT %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_L2ALLOC_MEM_BASE);
	printf(" USB_L2ALLOC_MEM_BASE %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_L2ALLOC_MEM_LIMIT);
	printf(" USB_L2ALLOC_MEM_LIMIT %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_READEX_MEM_BASE);
	printf(" USB_READEX_MEM_BASE %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_READEX_MEM_LIMIT);
	printf(" USB_READEX_MEM_LIMIT %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_PHY_STATUS);
	printf(" USB_PHY_STATUS %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_INTERRUPT_STATUS);
	printf(" USB_INTERRUPT_STATUS %#x\n", r);
	r = RMIXL_USBI_GEN_READ(RMIXL_USB_INTERRUPT_ENABLE);
	printf(" USB_INTERRUPT_ENABLE %#x\n", r);

	return 0;
}
#endif
