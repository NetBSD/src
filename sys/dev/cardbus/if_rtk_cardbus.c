/*	$NetBSD: if_rtk_cardbus.c,v 1.14 2003/02/01 07:50:38 ichiro Exp $	*/

/*
 * Copyright (c) 2000 Masanori Kanaoka
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * if_rtk_cardbus.c:
 *	Cardbus specific routines for RealTek 8139 ethernet adapter.
 *	Tested for 
 *		- elecom-Laneed	LD-10/100CBA (Accton MPX5030)
 *		- MELCO		LPC3-TX-CB   (RealTek 8139)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_rtk_cardbus.c,v 1.14 2003/02/01 07:50:38 ichiro Exp $");

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif
#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/cardbus/cardbusdevs.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

/*
 * Default to using PIO access for this driver. On SMP systems,
 * there appear to be problems with memory mapped mode: it looks like
 * doing too many memory mapped access back to back in rapid succession
 * can hang the bus. I'm inclined to blame this on crummy design/construction
 * on the part of RealTek. Memory mapped mode does appear to work on
 * uniprocessor systems though.
 */
#define RTK_USEIOSPACE 

#include <dev/ic/rtl81x9reg.h>
#include <dev/ic/rtl81x9var.h>

/*
 * Various supported device vendors/types and their names.
 */
static const struct rtk_type rtk_cardbus_devs[] = {
	{ CARDBUS_VENDOR_ACCTON, CARDBUS_PRODUCT_ACCTON_MPX5030,
		"Accton MPX 5030/5038 10/100BaseTX",
		RTK_8139 },
	{ CARDBUS_VENDOR_DLINK, CARDBUS_PRODUCT_DLINK_DFE_690TXD,
		"D-Link DFE-690TXD 10/100BaseTX", RTK_8139 },
	{ CARDBUS_VENDOR_REALTEK, CARDBUS_PRODUCT_REALTEK_RT8138,
		"RealTek 8138 10/100BaseTX", RTK_8139 },
	{ CARDBUS_VENDOR_REALTEK, CARDBUS_PRODUCT_REALTEK_RT8139,
		"RealTek 8139 10/100BaseTX", RTK_8139 },
	{ CARDBUS_VENDOR_COREGA, CARDBUS_PRODUCT_COREGA_CB_TXD,
		"Corega FEther CB-TXD 10/100BaseTX", RTK_8139 },
	{ CARDBUS_VENDOR_PLANEX, CARDBUS_PRODUCT_PLANEX_FNW_3603_TX,
		"Planex FNW-3603 10/100BaseTX", RTK_8139 },
	{ CARDBUS_VENDOR_ABOCOM, CARDBUS_PRODUCT_ABOCOM_FE2000VX,
		"AboCom FE2000VX 10/100BaseTX", RTK_8139 },

	{ 0, 0, NULL, 0 }
};

static int rtk_cardbus_match	__P((struct device *, struct cfdata *, void *));
static void rtk_cardbus_attach	__P((struct device *, struct device *, void *));
static int rtk_cardbus_detach	__P((struct device *, int));

struct rtk_cardbus_softc {
	struct rtk_softc sc_rtk;	/* real rtk softc */ 

	/* CardBus-specific goo. */
	void *sc_ih;
	cardbus_devfunc_t sc_ct;
	cardbustag_t sc_tag;
	int sc_csr;
	int sc_cben;
	int sc_bar_reg;
	pcireg_t sc_bar_val;
	bus_size_t sc_mapsize;
	int sc_intrline;
};

CFATTACH_DECL(rtk_cardbus, sizeof(struct rtk_cardbus_softc),
    rtk_cardbus_match, rtk_cardbus_attach, rtk_cardbus_detach, rtk_activate);

const struct rtk_type *rtk_cardbus_lookup
	__P((const struct cardbus_attach_args *));

void rtk_cardbus_setup		__P((struct rtk_cardbus_softc *));

int rtk_cardbus_enable		__P((struct rtk_softc *));
void rtk_cardbus_disable	__P((struct rtk_softc *));
void rtk_cardbus_power		__P((struct rtk_softc *, int));
const struct rtk_type *
rtk_cardbus_lookup(ca)
	const struct cardbus_attach_args *ca;
{
	const struct rtk_type *t;

	for (t = rtk_cardbus_devs; t->rtk_name != NULL; t++){ 	
		if (CARDBUS_VENDOR(ca->ca_id) == t->rtk_vid &&
		    CARDBUS_PRODUCT(ca->ca_id) == t->rtk_did) {
			return (t);
		}
	}
	return (NULL);
}

int
rtk_cardbus_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct cardbus_attach_args *ca = aux;

	if (rtk_cardbus_lookup(ca) != NULL)
		return (1);

	return (0);
}


void
rtk_cardbus_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct rtk_cardbus_softc *csc = (struct rtk_cardbus_softc *)self;
	struct rtk_softc *sc = &csc->sc_rtk;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	const struct rtk_type *t;
	bus_addr_t adr;

	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;
	csc->sc_intrline = ca->ca_intrline;

	t = rtk_cardbus_lookup(ca); 
	if (t == NULL) { 
		printf("\n"); 
		panic("rtk_cardbus_attach: impossible");
	 } 
	printf(": %s\n", t->rtk_name); 
	
	/*
	 * Power management hooks.
	 */
	sc->sc_enable = rtk_cardbus_enable;
	sc->sc_disable = rtk_cardbus_disable;
	sc->sc_power = rtk_cardbus_power;

	/*
	 * Map control/status registers.
	 */
	csc->sc_csr = CARDBUS_COMMAND_MASTER_ENABLE;
#ifdef RTK_USEIOSPACE
	if (Cardbus_mapreg_map(ct, RTK_PCI_LOIO, CARDBUS_MAPREG_TYPE_IO, 0,
	    &sc->rtk_btag, &sc->rtk_bhandle, &adr, &csc->sc_mapsize) == 0) {
#if rbus
#else
		(*ct->ct_cf->cardbus_io_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_IO_ENABLE;
		csc->sc_csr |= CARDBUS_COMMAND_IO_ENABLE;
		csc->sc_bar_reg = RTK_PCI_LOIO;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_IO;
	}
#else
	if (Cardbus_mapreg_map(ct, RTK_PCI_LOMEM, CARDBUS_MAPREG_TYPE_MEM, 0,
	    &sc->rtk_btag, &sc->rtk_bhandle, &adr, &csc->sc_mapsize) == 0) {
#if rbus
#else
		(*ct->ct_cf->cardbus_mem_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_MEM_ENABLE;
		csc->sc_csr |= CARDBUS_COMMAND_MEM_ENABLE;
		csc->sc_bar_reg = RTK_PCI_LOMEM;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_MEM;
	}
#endif
	else {
		printf("%s: unable to map deviceregisters\n",
			 sc->sc_dev.dv_xname);
		return;
	}
	/*
	 * Handle power management nonsense and initialize the
	 * configuration registers.
	 */
	rtk_cardbus_setup(csc);
	sc->rtk_type = t->rtk_type;

	rtk_attach(sc);

	/*
	 * Power down the socket.
	 */
	Cardbus_function_disable(csc->sc_ct);
}

int 
rtk_cardbus_detach(self, flags)
	struct device *self;
	int flags;
{
	struct rtk_cardbus_softc *csc = (void *) self;
	struct rtk_softc *sc = &csc->sc_rtk;
	struct cardbus_devfunc *ct = csc->sc_ct;
	int	rv;

#ifdef DIAGNOSTIC
	if (ct == NULL)
		panic("%s: data structure lacks", sc->sc_dev.dv_xname);
#endif
	rv = rtk_detach(sc);
	if (rv)
		return (rv);
	/*
	 * Unhook the interrut handler.
	 */
	if (csc->sc_ih != NULL)
		cardbus_intr_disestablish(ct->ct_cc, ct->ct_cf, csc->sc_ih);
	
	/*
	 * Release bus space and close window.
	 */
	if (csc->sc_bar_reg != 0)
		Cardbus_mapreg_unmap(ct, csc->sc_bar_reg,
			sc->rtk_btag, sc->rtk_bhandle, csc->sc_mapsize);

	return (0);
}

void 
rtk_cardbus_setup(csc)
	struct rtk_cardbus_softc *csc;
{
	struct rtk_softc *sc = &csc->sc_rtk;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;
	pcireg_t	reg,command;
	int		pmreg;

	/*
	 * Handle power management nonsense.
	 */
	if (cardbus_get_capability(cc, cf, csc->sc_tag,
	    PCI_CAP_PWRMGMT, &pmreg, 0)) {
		command = cardbus_conf_read(cc, cf, csc->sc_tag, pmreg + 4);
		if (command & RTK_PSTATE_MASK) {
			pcireg_t		iobase, membase, irq;

			/* Save important PCI config data. */
			iobase = cardbus_conf_read(cc, cf, csc->sc_tag,
			    RTK_PCI_LOIO);
			membase = cardbus_conf_read(cc, cf,csc->sc_tag,
			    RTK_PCI_LOMEM);
			irq = cardbus_conf_read(cc, cf,csc->sc_tag,
			    CARDBUS_INTERRUPT_REG);

			/* Reset the power state. */
			printf("%s: chip is in D%d power mode "
			    "-- setting to D0\n", sc->sc_dev.dv_xname,
			    command & RTK_PSTATE_MASK);
			command &= 0xFFFFFFFC;
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    pmreg + 4, command);

			/* Restore PCI config data. */
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    RTK_PCI_LOIO, iobase);
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    RTK_PCI_LOMEM, membase);
			cardbus_conf_write(cc, cf, csc->sc_tag,
			    CARDBUS_INTERRUPT_REG, irq);
		}
	}

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, csc->sc_cben);
	(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Program the BAR */
	cardbus_conf_write(cc, cf, csc->sc_tag,
		csc->sc_bar_reg, csc->sc_bar_val);

	/* Enable the appropriate bits in the CARDBUS CSR. */
	reg = cardbus_conf_read(cc, cf, csc->sc_tag, 
	    CARDBUS_COMMAND_STATUS_REG);
	reg &= ~(CARDBUS_COMMAND_IO_ENABLE|CARDBUS_COMMAND_MEM_ENABLE);
	reg |= csc->sc_csr;
	cardbus_conf_write(cc, cf, csc->sc_tag, 
	    CARDBUS_COMMAND_STATUS_REG, reg);

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

int 
rtk_cardbus_enable(sc)
	struct rtk_softc *sc;
{
	struct rtk_cardbus_softc *csc = (void *) sc;
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
	rtk_cardbus_setup(csc);

	/*
	 * Map and establish the interrupt.
	 */
	csc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline,
		IPL_NET, rtk_intr, sc);
	if (csc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt at %d\n",
			sc->sc_dev.dv_xname, csc->sc_intrline);
		Cardbus_function_disable(csc->sc_ct);
		return (1);
	}
	printf("%s: interrupting at %d\n", sc->sc_dev.dv_xname,
		csc->sc_intrline);
	return (0);
}

void 
rtk_cardbus_disable(sc)
	struct rtk_softc *sc;
{
	struct rtk_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/* Unhook the interrupt handler. */
	cardbus_intr_disestablish(cc, cf, csc->sc_ih);

	/* Power down the socket. */
	Cardbus_function_disable(ct);
}

void 
rtk_cardbus_power(sc, why)
	struct rtk_softc *sc;
	int why;
{
	struct rtk_cardbus_softc *csc = (void *) sc;

	if (why == PWR_RESUME) {
		/*
		 * Give the PCI configuration registers a kick
		 * in the head.
		 */
#ifdef DIAGNOSTIC
		if (RTK_IS_ENABLED(sc) == 0)
			panic("rtk_cardbus_power");
#endif
		rtk_cardbus_setup(csc);
	}
}
