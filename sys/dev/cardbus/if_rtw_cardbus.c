/* $NetBSD: if_rtw_cardbus.c,v 1.16.2.2 2007/12/27 00:44:59 mjf Exp $ */

/*-
 * Copyright (c) 2004, 2005 David Young.  All rights reserved.
 *
 * Adapted for the RTL8180 by David Young.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of David Young may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY David Young ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL David
 * Young BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Cardbus front-end for the Realtek RTL8180 802.11 MAC/BBP driver.
 *
 * TBD factor with atw, tlp Cardbus front-ends?
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_rtw_cardbus.c,v 1.16.2.2 2007/12/27 00:44:59 mjf Exp $");

#include "opt_inet.h"
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

#include <net80211/ieee80211_netbsd.h>
#include <net80211/ieee80211_radiotap.h>
#include <net80211/ieee80211_var.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif


#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/ic/rtwreg.h>
#include <dev/ic/rtwvar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/cardbus/cardbusvar.h>
#include <dev/pci/pcidevs.h>

/*
 * PCI configuration space registers used by the RTL8180.
 */
#define	RTW_PCI_IOBA		0x10	/* i/o mapped base */
#define	RTW_PCI_MMBA		0x14	/* memory mapped base */

struct rtw_cardbus_softc {
	struct rtw_softc sc_rtw;	/* real RTL8180 softc */

	/* CardBus-specific goo. */
	void			*sc_ih;		/* interrupt handle */
	cardbus_devfunc_t	sc_ct;		/* our CardBus devfuncs */
	cardbustag_t		sc_tag;		/* our CardBus tag */
	int			sc_csr;		/* CSR bits */
	bus_size_t		sc_mapsize;	/* size of the mapped bus space
						 * region
						 */

	int			sc_cben;	/* CardBus enables */
	int			sc_bar_reg;	/* which BAR to use */
	pcireg_t		sc_bar_val;	/* value of the BAR */

	int			sc_intrline;	/* interrupt line */
#if 0
	struct cardbus_conf_state	sc_conf;	/* configuration regs */
#endif
};

int	rtw_cardbus_match(device_t, struct cfdata *, void *);
void	rtw_cardbus_attach(device_t, device_t, void *);
int	rtw_cardbus_detach(device_t, int);

CFATTACH_DECL_NEW(rtw_cardbus, sizeof(struct rtw_cardbus_softc),
    rtw_cardbus_match, rtw_cardbus_attach, rtw_cardbus_detach, rtw_activate);

void	rtw_cardbus_setup(struct rtw_cardbus_softc *);

int rtw_cardbus_enable(struct rtw_softc *);
void rtw_cardbus_disable(struct rtw_softc *);
void rtw_cardbus_power(struct rtw_softc *, int);

const struct rtw_cardbus_product *rtw_cardbus_lookup(
     const struct cardbus_attach_args *);

const struct rtw_cardbus_product {
	u_int32_t	 rcp_vendor;	/* PCI vendor ID */
	u_int32_t	 rcp_product;	/* PCI product ID */
	const char	*rcp_product_name;
} rtw_cardbus_products[] = {
	{ PCI_VENDOR_REALTEK,		PCI_PRODUCT_REALTEK_RT8180,
	  "Realtek RTL8180 802.11 MAC/BBP" },

	{ PCI_VENDOR_BELKIN,		PCI_PRODUCT_BELKIN_F5D6020V3,
	  "Belkin F5D6020v3 802.11b (RTL8180 MAC/BBP)" },

	{ PCI_VENDOR_DLINK,		PCI_PRODUCT_DLINK_DWL610,
	  "DWL-610 D-Link Air 802.11b (RTL8180 MAC/BBP)" },

	{ 0,				0,	NULL },
};

const struct rtw_cardbus_product *
rtw_cardbus_lookup(const struct cardbus_attach_args *ca)
{
	const struct rtw_cardbus_product *rcp;

	for (rcp = rtw_cardbus_products; rcp->rcp_product_name != NULL; rcp++) {
		if (PCI_VENDOR(ca->ca_id) == rcp->rcp_vendor &&
		    PCI_PRODUCT(ca->ca_id) == rcp->rcp_product)
			return rcp;
	}
	return NULL;
}

int
rtw_cardbus_match(device_t parent, struct cfdata *match, void *aux)
{
	struct cardbus_attach_args *ca = aux;

	if (rtw_cardbus_lookup(ca) != NULL)
		return 1;

	return 0;
}

static void
rtw_cardbus_intr_ack(struct rtw_regs *regs)
{
	RTW_WRITE(regs, RTW_FER, RTW_FER_INTR);
}

static void
rtw_cardbus_funcregen(struct rtw_regs *regs, int enable)
{
	u_int32_t reg;
	rtw_config0123_enable(regs, 1);
	reg = RTW_READ(regs, RTW_CONFIG3);
	if (enable)
		RTW_WRITE(regs, RTW_CONFIG3, reg | RTW_CONFIG3_FUNCREGEN);
	else
		RTW_WRITE(regs, RTW_CONFIG3, reg & ~RTW_CONFIG3_FUNCREGEN);
	rtw_config0123_enable(regs, 0);
}

void
rtw_cardbus_attach(device_t parent, device_t self, void *aux)
{
	struct rtw_cardbus_softc *csc = device_private(self);
	struct rtw_softc *sc = &csc->sc_rtw;
	struct rtw_regs *regs = &sc->sc_regs;
	struct cardbus_attach_args *ca = aux;
	cardbus_devfunc_t ct = ca->ca_ct;
	const struct rtw_cardbus_product *rcp;
	bus_addr_t adr;
	int rev;

	sc->sc_dev = self;
	sc->sc_dmat = ca->ca_dmat;
	csc->sc_ct = ct;
	csc->sc_tag = ca->ca_tag;

	rcp = rtw_cardbus_lookup(ca);
	if (rcp == NULL) {
		printf("\n");
		panic("rtw_cardbus_attach: impossible");
	}

	/*
	 * Power management hooks.
	 */
	sc->sc_enable = rtw_cardbus_enable;
	sc->sc_disable = rtw_cardbus_disable;

	sc->sc_intr_ack = rtw_cardbus_intr_ack;

	/* Get revision info. */
	rev = PCI_REVISION(ca->ca_class);

	printf(": %s\n", rcp->rcp_product_name);

	RTW_DPRINTF(RTW_DEBUG_ATTACH,
	    ("%s: pass %d.%d signature %08x\n", device_xname(sc->sc_dev),
	     (rev >> 4) & 0xf, rev & 0xf,
	     cardbus_conf_read(ct->ct_cc, ct->ct_cf, csc->sc_tag, 0x80)));

	/*
	 * Map the device.
	 */
	csc->sc_csr = CARDBUS_COMMAND_MASTER_ENABLE;
	if (Cardbus_mapreg_map(ct, RTW_PCI_MMBA,
	    CARDBUS_MAPREG_TYPE_MEM, 0, &regs->r_bt, &regs->r_bh, &adr,
	    &csc->sc_mapsize) == 0) {
		RTW_DPRINTF(RTW_DEBUG_ATTACH,
		    ("%s: %s mapped %lu bytes mem space\n",
		     device_xname(sc->sc_dev), __func__,
		     (long)csc->sc_mapsize));
#if rbus
#else
		(*ct->ct_cf->cardbus_mem_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_MEM_ENABLE;
		csc->sc_csr |= CARDBUS_COMMAND_MEM_ENABLE;
		csc->sc_bar_reg = RTW_PCI_MMBA;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_MEM;
	} else if (Cardbus_mapreg_map(ct, RTW_PCI_IOBA,
	    CARDBUS_MAPREG_TYPE_IO, 0, &regs->r_bt, &regs->r_bh, &adr,
	    &csc->sc_mapsize) == 0) {
		RTW_DPRINTF(RTW_DEBUG_ATTACH,
		    ("%s: %s mapped %lu bytes I/O space\n",
		     device_xname(sc->sc_dev), __func__,
		     (long)csc->sc_mapsize));
#if rbus
#else
		(*ct->ct_cf->cardbus_io_open)(cc, 0, adr, adr+csc->sc_mapsize);
#endif
		csc->sc_cben = CARDBUS_IO_ENABLE;
		csc->sc_csr |= CARDBUS_COMMAND_IO_ENABLE;
		csc->sc_bar_reg = RTW_PCI_IOBA;
		csc->sc_bar_val = adr | CARDBUS_MAPREG_TYPE_IO;
	} else {
		aprint_error_dev(sc->sc_dev,
		    "unable to map device registers\n");
		return;
	}

	/*
	 * Bring the chip out of powersave mode and initialize the
	 * configuration registers.
	 */
	rtw_cardbus_setup(csc);

	/* Remember which interrupt line. */
	csc->sc_intrline = ca->ca_intrline;

	aprint_normal_dev(sc->sc_dev, "interrupting at %d\n", csc->sc_intrline);
	/*
	 * Finish off the attach.
	 */
	rtw_attach(sc);

	rtw_cardbus_funcregen(regs, 1);

	RTW_WRITE(regs, RTW_FEMR, RTW_FEMR_INTR);
	RTW_WRITE(regs, RTW_FER, RTW_FER_INTR);

#if 0
	cardbus_conf_capture(ct->ct_cc, ct->ct_cf, csc->sc_tag, &csc->sc_conf);
#endif

	/*
	 * Power down the socket.
	 */
	Cardbus_function_disable(csc->sc_ct);
}

int
rtw_cardbus_detach(device_t self, int flags)
{
	struct rtw_cardbus_softc *csc = device_private(self);
	struct rtw_softc *sc = &csc->sc_rtw;
	struct rtw_regs *regs = &sc->sc_regs;
	struct cardbus_devfunc *ct = csc->sc_ct;
	int rc;

#if defined(DIAGNOSTIC)
	if (ct == NULL)
		panic("%s: data structure lacks", device_xname(self));
#endif

	if ((rc = rtw_detach(sc)) != 0)
		return rc;

	rtw_cardbus_funcregen(regs, 0);

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
		    regs->r_bt, regs->r_bh, csc->sc_mapsize);

	return 0;
}

int
rtw_cardbus_enable(struct rtw_softc *sc)
{
	struct rtw_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	/*
	 * Power on the socket.
	 */
	Cardbus_function_enable(ct);

#if 0
	cardbus_conf_restore(cc, cf, csc->sc_tag, &csc->sc_conf);
#endif

	/*
	 * Set up the PCI configuration registers.
	 */
	rtw_cardbus_setup(csc);

	/*
	 * Map and establish the interrupt.
	 */
	csc->sc_ih = cardbus_intr_establish(cc, cf, csc->sc_intrline, IPL_NET,
	    rtw_intr, sc);
	if (csc->sc_ih == NULL) {
		aprint_error_dev(sc->sc_dev,
		    "unable to establish interrupt at %d\n", csc->sc_intrline);
		Cardbus_function_disable(csc->sc_ct);
		return 1;
	}

	rtw_cardbus_funcregen(&sc->sc_regs, 1);

	RTW_WRITE(&sc->sc_regs, RTW_FEMR, RTW_FEMR_INTR);
	RTW_WRITE(&sc->sc_regs, RTW_FER, RTW_FER_INTR);

	return 0;
}

void
rtw_cardbus_disable(struct rtw_softc *sc)
{
	struct rtw_cardbus_softc *csc = (void *) sc;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbus_function_tag_t cf = ct->ct_cf;

	RTW_WRITE(&sc->sc_regs, RTW_FEMR,
	    RTW_READ(&sc->sc_regs, RTW_FEMR) & ~RTW_FEMR_INTR);

	rtw_cardbus_funcregen(&sc->sc_regs, 0);

	/* Unhook the interrupt handler. */
	cardbus_intr_disestablish(cc, cf, csc->sc_ih);
	csc->sc_ih = NULL;

#if 0
	cardbus_conf_capture(cc, cf, csc->sc_tag, &csc->sc_conf);
#endif

	/* Power down the socket. */
	Cardbus_function_disable(ct);
}

void
rtw_cardbus_setup(struct rtw_cardbus_softc *csc)
{
	cardbustag_t tag = csc->sc_tag;
	cardbus_devfunc_t ct = csc->sc_ct;
	cardbus_chipset_tag_t cc = ct->ct_cc;
	cardbusreg_t bhlc, csr, lattimer;
	cardbus_function_tag_t cf = ct->ct_cf;

	(void)cardbus_set_powerstate(ct, tag, PCI_PWR_D0);

	/* I believe the datasheet tries to warn us that the RTL8180
	 * wants for 16 (0x10) to divide the latency timer.
	 */
	bhlc = cardbus_conf_read(cc, cf, tag, CARDBUS_BHLC_REG);
	lattimer = rounddown(PCI_LATTIMER(bhlc), 0x10);
	if (PCI_LATTIMER(bhlc) != lattimer) {
		bhlc &= ~(PCI_LATTIMER_MASK << PCI_LATTIMER_SHIFT);
		bhlc |= (lattimer << PCI_LATTIMER_SHIFT);
		cardbus_conf_write(cc, cf, tag, CARDBUS_BHLC_REG, bhlc);
	}

	/* Program the BAR. */
	cardbus_conf_write(cc, cf, tag, csc->sc_bar_reg, csc->sc_bar_val);

	/* Make sure the right access type is on the CardBus bridge. */
	(*ct->ct_cf->cardbus_ctrl)(cc, csc->sc_cben);
	(*ct->ct_cf->cardbus_ctrl)(cc, CARDBUS_BM_ENABLE);

	/* Enable the appropriate bits in the PCI CSR. */
	csr = cardbus_conf_read(cc, cf, tag, PCI_COMMAND_STATUS_REG);
	csr &= ~(PCI_COMMAND_IO_ENABLE|PCI_COMMAND_MEM_ENABLE);
	csr |= csc->sc_csr;
	csr |= CARDBUS_COMMAND_PARITY_ENABLE | CARDBUS_COMMAND_SERR_ENABLE;
	cardbus_conf_write(cc, cf, tag, PCI_COMMAND_STATUS_REG, csr);
}
