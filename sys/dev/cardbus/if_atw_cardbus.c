/*	$NetBSD: if_atw_cardbus.c,v 1.1 2003/07/06 22:57:23 dyoung Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.  This code was adapted for the ADMtek ADM8211
 * by David Young.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * CardBus bus front-end for the ADMtek ADM8211 802.11 MAC/BBP driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_atw_cardbus.c,v 1.1 2003/07/06 22:57:23 dyoung Exp $");

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <machine/endian.h>
 
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_ieee80211.h>

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#ifdef INET
#include <netinet/in.h> 
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/atwreg.h>
#include <dev/ic/atwvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/cardbusdevs.h>

/*
 * PCI configuration space registers used by the ADM8211.
 */
#define	ATW_PCI_IOBA		0x10	/* i/o mapped base */
#define	ATW_PCI_MMBA		0x14	/* memory mapped base */

struct atw_cardbus_softc {
	struct atw_softc sc_atw;	/* real ADM8211 softc */

	/* CardBus-specific goo. */
	void	*sc_ih;			/* interrupt handle */
	cardbus_devfunc_t sc_ct;	/* our CardBus devfuncs */
	cardbustag_t sc_tag;		/* our CardBus tag */
	int	sc_csr;			/* CSR bits */
	bus_size_t sc_mapsize;		/* the size of mapped bus space
					   region */

	int	sc_cben;		/* CardBus enables */
	int	sc_bar_reg;		/* which BAR to use */
	pcireg_t sc_bar_val;		/* value of the BAR */

	int	sc_intrline;		/* interrupt line */
};

int	atw_cardbus_match __P((struct device *, struct cfdata *, void *));
void	atw_cardbus_attach __P((struct device *, struct device *, void *));
int	atw_cardbus_detach __P((struct device *, int));

CFATTACH_DECL(atw_cardbus, sizeof(struct atw_cardbus_softc),
    atw_cardbus_match, atw_cardbus_attach, atw_cardbus_detach, atw_activate);

void	atw_cardbus_setup __P((struct atw_cardbus_softc *));

int	atw_cardbus_enable __P((struct atw_softc *));
void	atw_cardbus_disable __P((struct atw_softc *));
void	atw_cardbus_power __P((struct atw_softc *, int));

static void atw_cardbus_intr_ack(struct atw_softc *);

const struct atw_cardbus_product *atw_cardbus_lookup
    __P((const struct cardbus_attach_args *));

const struct atw_cardbus_product {
	u_int32_t	 acp_vendor;	/* PCI vendor ID */
	u_int32_t	 acp_product;	/* PCI product ID */
	const char	*acp_product_name;
} atw_cardbus_products[] = {
	{ PCI_VENDOR_ADMTEK,		PCI_PRODUCT_ADMTEK_ADM8211,
	  "ADMtek ADM8211 802.11 MAC/BBP" },

	{ 0,				0,	NULL },
};

const struct atw_cardbus_product *
atw_cardbus_lookup(ca)
	const struct cardbus_attach_args *ca;
{
	const struct atw_cardbus_product *acp;

	for (acp = atw_cardbus_products;
	     acp->acp_product_name != NULL;
	     acp++) {
		if (PCI_VENDOR(ca->ca_id) == acp->acp_vendor &&
		    PCI_PRODUCT(ca->ca_id) == acp->acp_product)
			return (acp);
	}
	return (NULL);
}

int
atw_cardbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct cardbus_attach_args *ca = aux;

	if (atw_cardbus_lookup(ca) != NULL)
		return (1);

	return (0);
}

void
atw_cardbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct atw_cardbus_softc *csc = (void *)self;
	struct atw_softc *sc = &csc->sc_atw;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	const struct atw_cardbus_product *acp;
	bus_addr_t adr;
	int rev;

	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;

	acp = atw_cardbus_lookup(ca);
	if (acp == NULL) {
		printf("\n");
		panic("atw_cardbus_attach: impossible");
	}

	/*
	 * Power management hooks.
	 */
	sc->sc_enable = atw_cardbus_enable;
	sc->sc_disable = atw_cardbus_disable;
	sc->sc_power = atw_cardbus_power;

	sc->sc_intr_ack = atw_cardbus_intr_ack;

	/* Get revision info. */
	rev = PCI_REVISION(ca->ca_class);

	printf(": %s\n", acp->acp_product_name);

#if 0
	printf("%s: pass %d.%d signature %08x\n", sc->sc_dev.dv_xname,
	    (rev >> 4) & 0xf, rev & 0xf,
	    cardbus_conf_read(ct->ct_cc, ct->ct_cf, csc->sc_tag, 0x80));
#endif

	/*
	 * Map the device.
	 */
	csc->sc_csr = CARDBUS_COMMAND_MASTER_ENABLE;
	if (Cardbus_mapreg_map(ct, ATW_PCI_MMBA,
	    CARDBUS_MAPREG_TYPE_MEM, 0, &sc->sc_st, &sc->sc_sh, &adr,
	    &csc->sc_mapsize) == 0) {
#if 0
		printf("%s: atw_cardbus_attach mapped %d bytes mem space\n",
		    sc->sc_dev.dv_xname, csc->sc_mapsize);
#endif
#if rbus
#else
		(*ct->ct_cf->cardbus_mem_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_MEM_ENABLE;
		csc->sc_csr |= CARDBUS_COMMAND_MEM_ENABLE;
		csc->sc_bar_reg = ATW_PCI_MMBA;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_MEM;
	} else if (Cardbus_mapreg_map(ct, ATW_PCI_IOBA,
	    CARDBUS_MAPREG_TYPE_IO, 0, &sc->sc_st, &sc->sc_sh, &adr,
	    &csc->sc_mapsize) == 0) {
#if 0
		printf("%s: atw_cardbus_attach mapped %d bytes I/O space\n",
		    sc->sc_dev.dv_xname, csc->sc_mapsize);
#endif
#if rbus
#else
		(*ct->ct_cf->cardbus_io_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_IO_ENABLE;
		csc->sc_csr |= CARDBUS_COMMAND_IO_ENABLE;
		csc->sc_bar_reg = ATW_PCI_IOBA;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_IO;
	} else {
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Bring the chip out of powersave mode and initialize the
	 * configuration registers.
	 */
	atw_cardbus_setup(csc);

	/* Remember which interrupt line. */
	csc->sc_intrline = ca->ca_intrline;

	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
	    csc->sc_intrline);
#if 0
	/*
	 * The CardBus cards will make it to store-and-forward mode as
	 * soon as you put them under any kind of load, so just start
	 * out there.
	 */
	sc->sc_txthresh = 3; /* TBD name constant */
#endif

	/*
	 * Finish off the attach.
	 */
	atw_attach(sc);

	ATW_WRITE(sc, ATW_FER, ATW_FER_INTR);

	/*
	 * Power down the socket.
	 */
	Cardbus_function_disable(csc->sc_ct);
}

static void
atw_cardbus_intr_ack(sc)
        struct atw_softc *sc;
{
	ATW_WRITE(sc, ATW_FER, ATW_FER_INTR);
}

int
atw_cardbus_detach(self, flags)
	struct device *self;
	int flags;
{
	struct atw_cardbus_softc *csc = (void *)self;
	struct atw_softc *sc = &csc->sc_atw;
	struct cardbus_devfunc *ct = csc->sc_ct;
	int rv;

#if defined(DIAGNOSTIC)
	if (ct == NULL)
		panic("%s: data structure lacks", sc->sc_dev.dv_xname);
#endif

	rv = atw_detach(sc);
	if (rv)
		return (rv);

	/*
	 * Unhook the interrupt handler.
	 */
	if (csc->sc_ih != NULL)
		cardbus_intr_disestablish(ct->ct_cc, ct->ct_cf, csc->sc_ih);

	/*
	 * Release bus space and close window.
	 */
	if (csc->sc_bar_reg != 0)
		Cardbus_mapreg_unmap(ct, csc->sc_bar_reg,
		    sc->sc_st, sc->sc_sh, csc->sc_mapsize);

	return (0);
}

int
atw_cardbus_enable(sc)
	struct atw_softc *sc;
{
	struct atw_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/*
	 * Power on the socket.
	 */
	Cardbus_function_enable(ct);

	/*
	 * Set up the PCI configuration registers.
	 */
	atw_cardbus_setup(csc);

	/*
	 * Map and establish the interrupt.
	 */
	csc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline, IPL_NET,
	    atw_intr, sc);
	if (csc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt at %d\n",
		    sc->sc_dev.dv_xname, csc->sc_intrline);
		Cardbus_function_disable(csc->sc_ct);
		return (1);
	}

	return (0);
}

void
atw_cardbus_disable(sc)
	struct atw_softc *sc;
{
	struct atw_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/* Unhook the interrupt handler. */
	cardbus_intr_disestablish(cc, cf, csc->sc_ih);
	csc->sc_ih = NULL;

	/* Power down the socket. */
	Cardbus_function_disable(ct);
}

void
atw_cardbus_power(sc, why)
	struct atw_softc *sc;
	int why;
{
	struct atw_cardbus_softc *csc = (void *) sc;

	printf("%s: atw_cardbus_power\n", sc->sc_dev.dv_xname);

	if (why == PWR_RESUME) {
		/*
		 * Give the PCI configuration registers a kick
		 * in the head.
		 */
#ifdef DIAGNOSTIC
		if (ATW_IS_ENABLED(sc) == 0)
			panic("atw_cardbus_power");
#endif
		atw_cardbus_setup(csc);
	}
}

void
atw_cardbus_setup(csc)
	struct atw_cardbus_softc *csc;
{
	struct atw_softc *sc = &csc->sc_atw;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t reg;
	int pmreg;

	if (cardbus_get_capability(cc, cf, csc->sc_tag,
	    PCI_CAP_PWRMGMT, &pmreg, 0)) {
		reg = cardbus_conf_read(cc, cf, csc->sc_tag, pmreg + 4) & 0x03;
#if 1 /* XXX Probably not right for CardBus. */
		if (reg == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake up from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
#endif
		if (reg != 0) {
			printf("%s: waking up from power state D%d\n",
			    sc->sc_dev.dv_xname, reg);
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    pmreg + 4, 0);
		}
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, csc->sc_cben);
	(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Program the BAR. */
	cardbus_conf_write(cc, cf, csc->sc_tag, csc->sc_bar_reg,
	    csc->sc_bar_val);

	/* Enable the appropriate bits in the PCI CSR. */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag,
	    CARDBUS_COMMAND_STATUS_REG);
	reg &= ~(CARDBUS_COMMAND_IO_ENABLE|CARDBUS_COMMAND_MEM_ENABLE);
	reg |= csc->sc_csr;
	cardbus_conf_write(cc, cf, csc->sc_tag, CARDBUS_COMMAND_STATUS_REG,
	    reg);

	/*
	 * Make sure the latency timer is set to some reasonable
	 * value.
	 */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag, CARDBUS_BHLC_REG);
	if (CARDBUS_LATTIMER(reg) < 0x20) {
		reg &= ~(CARDBUS_LATTIMER_MASK << CARDBUS_LATTIMER_SHIFT);
		reg |= (0x20 << CARDBUS_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, csc->sc_tag, CARDBUS_BHLC_REG, reg);
	}
}

