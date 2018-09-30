/*	$NetBSD: rmixl_iobus.c,v 1.3.54.1 2018/09/30 01:45:45 pgoyette Exp $	*/

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

/*
 * RMI Peripherals IO Bus support
 * - interface to NOR, NAND, PCMCIA Memory controlers, &etc.
 * - manages the 10 Chip Selects
 * - manages the "Flash" interrupts
 * - manages the "Flash" errors
 */

/*
 * iobus control registers are accessed as 32 bits.
 * ALEn and CLEn NAND control registers are defined as 8 bits wide
 * but that seems to be a documentation error.
 *
 * iobus data access may be as 1 or 2 or 4 bytes, even if device is 1 byte wide;
 * the controller will sequence the bytes, in big-endian order.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_iobus.c,v 1.3.54.1 2018/09/30 01:45:45 pgoyette Exp $");

#include "locators.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>
#include <mips/rmi/rmixl_intr.h>
#include <mips/rmi/rmixl_obiovar.h>
#include <mips/rmi/rmixl_iobusvar.h>
// #include <mips/rmi/rmixl_gpiovar.h>

typedef struct {
	bool		cs_allocated;
	uint32_t	cs_addr; /* base address on the Peripherals I/O Bus */
	uint32_t	cs_mask; /* address mask on the Peripherals I/O Bus */
	uint32_t	cs_dev_parm; 
} rmixl_iobus_csconfig_t;
 
typedef struct rmixl_iobus_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_obio_bst;	/* for iobus device controler access */
	bus_space_handle_t	sc_obio_bsh;	/*  "   "     "      "         "     */
	bus_addr_t		sc_obio_addr;
	bus_size_t		sc_obio_size;
	bus_space_tag_t		sc_iobus_bst;	/* for iobus access */
	rmixl_iobus_csconfig_t	sc_csconfig[RMIXL_FLASH_NCS];
} rmixl_iobus_softc_t;


static int	rmixl_iobus_match(device_t, cfdata_t, void *);
static void	rmixl_iobus_attach(device_t, device_t, void *);
static void	rmixl_iobus_csconfig_init(struct rmixl_iobus_softc *);
static int  	rmixl_iobus_print(void *, const char *);
static int  	rmixl_iobus_search(device_t, cfdata_t, const int *, void *);
#ifdef NOTYET
static int      rmixl_iobus_intr(void *);
#endif

#ifdef RMIXL_IOBUS_DEBUG
rmixl_iobus_softc_t *rmixl_iobus_sc;
#endif


CFATTACH_DECL_NEW(rmixl_iobus, sizeof (rmixl_iobus_softc_t),
    rmixl_iobus_match, rmixl_iobus_attach, NULL, NULL);

int
rmixl_iobus_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;

        if (obio->obio_addr == RMIXL_IO_DEV_FLASH)
		return rmixl_probe_4((volatile uint32_t *)
			RMIXL_IOREG_VADDR(obio->obio_addr));

        return 0;
}

void
rmixl_iobus_attach(device_t parent, device_t self, void *aux)
{
	rmixl_iobus_softc_t *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	struct rmixl_config *rcp = &rmixl_configuration;
	uint64_t r;
	int err;

#ifdef RMIXL_IOBUS_DEBUG
	rmixl_iobus_sc = sc;
#endif
	sc->sc_dev = self;
	sc->sc_obio_bst = obio->obio_eb_bst;
	sc->sc_obio_addr = obio->obio_addr;
	sc->sc_obio_size = 0x1000;

	err = bus_space_map(sc->sc_obio_bst, sc->sc_obio_addr,
		sc->sc_obio_size, 0, &sc->sc_obio_bsh);
	if (err != 0) {
		aprint_error_dev(self,
			"bus space map err %d, iobus space\n", err);
		return;
	}

	r = RMIXL_IOREG_READ(RMIXL_SBC_FLASH_BAR);
	KASSERT((r & 1) != 0);	/* BAR is enabled */
	rcp->rc_flash_pbase = RMIXL_FLASH_BAR_TO_BA(r);
	rcp->rc_flash_mask  = RMIXL_FLASH_BAR_TO_MASK(r);

	aprint_normal("\n");
	aprint_debug_dev(self,
		"Flash BAR pbase %#" PRIx64 " mask %#" PRIx64 "\n",
		rcp->rc_flash_pbase, rcp->rc_flash_mask);

	/* initialize iobus bus space */
	rmixl_iobus_bus_mem_init(&rcp->rc_iobus_memt, rcp);
	sc->sc_iobus_bst = (bus_space_tag_t)&rcp->rc_iobus_memt;

	/* disable all Flash interrupts */
	bus_space_write_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		RMIXL_FLASH_INT_MASK, 0);

	/* write-1-to-clear Flash interrupt status */
	bus_space_write_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		RMIXL_FLASH_INT_STATUS, ~0);

	rmixl_iobus_csconfig_init(sc);

	/* attach any children */
	config_search_ia(rmixl_iobus_search, self, "rmixl_iobus", NULL);
}

static void
rmixl_iobus_csconfig_init(struct rmixl_iobus_softc *sc)
{    
	rmixl_iobus_csconfig_t *cs = &sc->sc_csconfig[0];

	for (int i=0; i < RMIXL_FLASH_NCS; i++) {
		memset(cs, 0, sizeof(rmixl_iobus_csconfig_t));
		cs->cs_addr = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
				RMIXL_FLASH_CSBASE_ADDRn(i)) << 16;
		cs->cs_mask = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
				RMIXL_FLASH_CSADDR_MASKn(i)) << 16;
		cs->cs_mask |= __BITS(15,0);
		cs->cs_dev_parm = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
				RMIXL_FLASH_CSDEV_PARMn(i));
		aprint_debug_dev(sc->sc_dev,
			"CS#%d: addr 0x%08x mask 0x%08x parm 0x%08x\n",
			i, cs->cs_addr, cs->cs_mask, cs->cs_dev_parm);
		cs++;
	}
}


static int
rmixl_iobus_print(void *aux, const char *pnp)
{
	struct rmixl_iobus_attach_args *ia = aux;

	if (ia->ia_cs != RMIXL_IOBUSCF_CS_DEFAULT)
		aprint_normal(" CS#%d", ia->ia_cs);
	if (ia->ia_iobus_addr != RMIXL_IOBUSCF_ADDR_DEFAULT) {
		aprint_normal(" addr %#" PRIxBUSADDR, ia->ia_iobus_addr);
		if (ia->ia_iobus_size != RMIXL_IOBUSCF_SIZE_DEFAULT)
			aprint_normal("-%#" PRIxBUSSIZE,
				ia->ia_iobus_addr + (ia->ia_iobus_size - 1));
	}
	if (ia->ia_iobus_intr != RMIXL_IOBUSCF_INTR_DEFAULT)
		aprint_normal(" intr %d", ia->ia_iobus_intr);

	return UNCONF;
}

static int
rmixl_iobus_search(device_t parent, cfdata_t cf, const int *ldesc, void *aux)
{
	struct rmixl_iobus_softc *sc = device_private(parent);
	struct rmixl_iobus_attach_args ia;
	rmixl_iobus_csconfig_t *cs;

	ia.ia_obio_bst = sc->sc_obio_bst;
	ia.ia_obio_bsh = sc->sc_obio_bsh;
	ia.ia_iobus_bst = sc->sc_iobus_bst;
	ia.ia_iobus_addr = (bus_addr_t)cf->cf_loc[RMIXL_IOBUSCF_ADDR];
	ia.ia_iobus_size = (bus_size_t)cf->cf_loc[RMIXL_IOBUSCF_SIZE];
	ia.ia_iobus_intr = cf->cf_loc[RMIXL_IOBUSCF_INTR];
	ia.ia_cs = cf->cf_loc[RMIXL_IOBUSCF_CS];

	if (ia.ia_cs != RMIXL_IOBUSCF_CS_DEFAULT) {
		/* CS is configured */
		cs = &sc->sc_csconfig[ia.ia_cs];

		/* ensure exclusive use of chip select */
		if (cs->cs_allocated) {
			aprint_error_dev(parent, "CS#%d already allocated\n",
				ia.ia_cs);
			return 0;
		}
		if (ia.ia_iobus_addr != RMIXL_IOBUSCF_ADDR_DEFAULT) {
			if (ia.ia_iobus_addr != cs->cs_addr) {
				/*
				 * both CS and addr are configured,
				 * ensure they match
				 */
				aprint_error_dev(parent,
					"CS#%d addr 0x%08x mismatch cf_loc "
					"addr 0x%08" PRIxBUSADDR "\n",
					ia.ia_cs, cs->cs_addr, ia.ia_iobus_addr);
				return 0;
			}
		} else {
			/* no addr configured, pull from CS */
			ia.ia_iobus_addr = cs->cs_addr;
		}
	} else {
		/* addr is configured, CS is not; search for matching CS */
		bool found = false;
		cs = &sc->sc_csconfig[0];
		for (int i=0; i < RMIXL_FLASH_NCS; i++) {
			if (cs->cs_allocated)
				continue;
			if (cs->cs_addr == ia.ia_iobus_addr) {
				ia.ia_cs = i;
				found = true;
				break;
			}
			cs++;
		}
		if (! found) {
			aprint_error_dev(parent, "no CS for addr 0x%08"
				PRIxBUSADDR "\n", ia.ia_iobus_addr);
			return 0;
		}
	}

	if (ia.ia_iobus_size != RMIXL_IOBUSCF_SIZE_DEFAULT) {
		/* ensure size fits w/ CS mask */
		if ((ia.ia_iobus_size - 1) > (bus_size_t)cs->cs_mask) {
			aprint_error_dev(parent, "size %#" PRIxBUSSIZE
				" exceeds CS#%d mask 0x%08x\n",
				ia.ia_iobus_size, ia.ia_cs, cs->cs_mask);
		}
	} else {
		/* size not configured, pull from CS */
		ia.ia_iobus_size = (bus_size_t)cs->cs_mask + 1;
	}

	ia.ia_dev_parm = cs->cs_dev_parm;

	if (config_match(parent, cf, &ia) > 0) {
		cs->cs_allocated = true;
		config_attach(parent, cf, &ia, rmixl_iobus_print);
	}

	return 0;
}


#ifdef NOTYET 

void
rmixl_iobus_intr_disestablish(void *uh, void *ih)
{
	rmixl_iobus_softc_t *sc = uh;
	u_int intr;

	for (intr=0; intr <= RMIXL_UB_INTERRUPT_MAX; intr++) {
		if (ih == &sc->sc_dispatch[intr]) {
			uint32_t r;

			/* disable this interrupt in the usb interface */
			r = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
				RMIXL_USB_INTERRUPT_ENABLE);
			r &= 1 << intr;
			bus_space_write_4(sc->sc_obio_bst, sc->sc_obio_bsh,
				RMIXL_USB_INTERRUPT_ENABLE, r);

			/* free the dispatch slot */
			sc->sc_dispatch[intr].func = NULL;
			sc->sc_dispatch[intr].arg = NULL;

			break;
		}
	}
}

void *
rmixl_iobus_intr_establish(void *uh, u_int intr, int (func)(void *), void *arg)
{
	rmixl_iobus_softc_t *sc = uh;
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
	r = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		RMIXL_USB_INTERRUPT_ENABLE);
	r |= 1 << intr;
	bus_space_write_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		RMIXL_USB_INTERRUPT_ENABLE, r);

 out:
	splx(s);
	return ih;
}

static int
rmixl_iobus_intr(void *arg)
{
	rmixl_iobus_softc_t *sc = arg;
	uint32_t r;
	int intr;
	int rv = 0;

	r = bus_space_read_4(sc->sc_obio_bst, sc->sc_obio_bsh,
		RMIXL_USB_INTERRUPT_STATUS);
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

#endif	/* NOTYET */
