/*
 *   Copyright (c) 1998,1999 Martin Husemann. All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. Neither the name of the author nor the names of any co-contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *   4. Altered versions must be plainly marked as such, and must not be
 *      misrepresented as being the original software and/or documentation.
 *   
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *   SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	isic_pci.c - pci bus frontend for i4b_isic driver
 *	----------------------------------------------------
 *
 *	$Id: isic_pci.c,v 1.6 2002/02/10 12:26:23 wiz Exp $ 
 *
 *      last edit-date: [Fri Jan  5 11:38:58 2001]
 *
 *	-mh	original implementation
 *	-mh	added support for Fritz! PCI card
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isic_pci.c,v 1.6 2002/02/10 12:26:23 wiz Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>


#include <machine/bus.h>
#include <machine/intr.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#ifdef __FreeBSD__
#include <machine/i4b_ioctl.h>
#else
#include <netisdn/i4b_ioctl.h>
#endif

#include <dev/ic/isic_l1.h>
#include <dev/ic/ipac.h>
#include <dev/ic/isac.h>
#include <dev/ic/hscx.h>

#include <netisdn/i4b_global.h>
#include <netisdn/i4b_trace.h>
#include <netisdn/i4b_l1l2.h>
#include <dev/pci/isic_pci.h>

#include "opt_isicpci.h"

extern const struct isdn_layer1_bri_driver isic_std_driver;

static int isic_pci_match __P((struct device *, struct cfdata *, void *));
static void isic_pci_attach __P((struct device *, struct device *, void *));
static const struct isic_pci_product * find_matching_card __P((struct pci_attach_args *pa));

#ifdef ISICPCI_ELSA_QS1PCI
static void isic_pci_isdn_attach __P((struct pci_l1_softc *psc, struct pci_attach_args *pa));
#endif

struct cfattach isic_pci_ca = {
	sizeof(struct pci_l1_softc), isic_pci_match, isic_pci_attach
};


static const struct isic_pci_product {
	pci_vendor_id_t npp_vendor;
	pci_product_id_t npp_product;
	int cardtype;
	const char * name;
	void (*attach)(struct pci_l1_softc *psc, struct pci_attach_args *pa);
	void (*pciattach)(struct pci_l1_softc *psc, struct pci_attach_args *pa);
} isic_pci_products[] = {

#ifdef ISICPCI_ELSA_QS1PCI
#ifndef PCI_PRODUCT_ELSA_QS1PCI
#define PCI_PRODUCT_ELSA_QS1PCI 0x1000	/* added to pcidevs in 1.3K, earlier versions missing it */
#endif
	{ PCI_VENDOR_ELSA, PCI_PRODUCT_ELSA_QS1PCI,
	  CARD_TYPEP_ELSAQS1PCI,
	  "ELSA QuickStep 1000pro/PCI",
	  isic_attach_Eqs1pp,	/* card specific initialization */
	  isic_pci_isdn_attach	/* generic setup for ISAC/HSCX or IPAC boards */
	 },
#endif

#ifdef ISICPCI_AVM_A1
#ifndef PCI_VENDOR_AVM
#define PCI_VENDOR_AVM	0x1244	/* earlier versions missing this */
#define	PCI_PRODUCT_AVM_FRITZ_CARD 0x0a00
#endif
	{ PCI_VENDOR_AVM, PCI_PRODUCT_AVM_FRITZ_CARD,
	  CARD_TYPEP_AVMA1PCI,
	  "Fritz!Card",
	  isic_attach_fritzPci,
	  NULL				/* card rolls its own setup */
	 },
#endif

	{ 0, 0, 0, NULL, NULL },
};

static const struct isic_pci_product * find_matching_card(pa)
	struct pci_attach_args *pa;
{
	const struct isic_pci_product * pp = NULL;

	for (pp = isic_pci_products; pp->npp_vendor; pp++) 
		if (PCI_VENDOR(pa->pa_id) == pp->npp_vendor &&
		    PCI_PRODUCT(pa->pa_id) == pp->npp_product)
			return pp;

	return NULL;
}

/*
 * Match card
 */
static int
isic_pci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (!find_matching_card(pa))
		return 0;

	return 1;
}

/*
 * Attach the card
 */
static void
isic_pci_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pci_l1_softc *psc = (void*) self;
	struct l1_softc *sc = &psc->sc_isic;
	struct pci_attach_args *pa = aux;
	const struct isic_pci_product * prod;

	/* Redo probe */
	prod = find_matching_card(pa);
	if (prod == NULL) return; /* oops - not found?!? */

	sc->sc_unit = sc->sc_dev.dv_unit;
	printf(": %s\n", prod->name);

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
	callout_init(&sc->sc_T3_callout);
	callout_init(&sc->sc_T4_callout);
#endif

	/* card initilization and sc setup */
	prod->attach(psc, pa);

	/* generic setup, if needed for this card */
	if (prod->pciattach) prod->pciattach(psc, pa);
}

/*---------------------------------------------------------------------------*
 *	isic - pci device driver attach routine
 *---------------------------------------------------------------------------*/
#ifdef ISICPCI_ELSA_QS1PCI
static void
isic_pci_isdn_attach(psc, pa)
	struct pci_l1_softc *psc;
	struct pci_attach_args *pa;
{
	struct l1_softc *sc = &psc->sc_isic;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr;

  	static char *ISACversion[] = {
  		"2085 Version A1/A2 or 2086/2186 Version 1.1",
		"2085 Version B1",
		"2085 Version B2",
		"2085 Version V2.3 (B3)",
		"Unknown Version"
	};

	static char *HSCXversion[] = {
		"82525 Version A1",
		"Unknown (0x01)",
		"82525 Version A2",
		"Unknown (0x03)",
		"82525 Version A3",
		"82525 or 21525 Version 2.1",
		"Unknown Version"
	};

	sc->sc_isac_version = 0;
	sc->sc_hscx_version = 0;
	
	if(sc->sc_ipac)
	{
		u_int ret = IPAC_READ(IPAC_ID);

		switch(ret)
		{
			case 0x01:
				printf("%s: IPAC PSB2115 Version 1.1\n", sc->sc_dev.dv_xname);
				break;

			case 0x02:
				printf("%s: IPAC PSB2115 Version 2\n", sc->sc_dev.dv_xname);
				break;
	
			default:
				printf("%s: Error, IPAC version %d unknown!\n",
					sc->sc_dev.dv_xname, ret);
				return;
		}
	}
	else
	{
		sc->sc_isac_version = ((ISAC_READ(I_RBCH)) >> 5) & 0x03;
	
		switch(sc->sc_isac_version)
		{
			case ISAC_VA:
			case ISAC_VB1:
	                case ISAC_VB2:
			case ISAC_VB3:
				printf("%s: ISAC %s (IOM-%c)\n",
					sc->sc_dev.dv_xname,
					ISACversion[sc->sc_isac_version],
					sc->sc_bustyp == BUS_TYPE_IOM1 ? '1' : '2');
				break;
	
			default:
				printf("%s: Error, ISAC version %d unknown!\n",
					sc->sc_dev.dv_xname, sc->sc_isac_version);
				return;
		}
	
		sc->sc_hscx_version = HSCX_READ(0, H_VSTR) & 0xf;
	
		switch(sc->sc_hscx_version)
		{
			case HSCX_VA1:
			case HSCX_VA2:
			case HSCX_VA3:
			case HSCX_V21:
				printf("%s: HSCX %s\n",
					sc->sc_dev.dv_xname,
					HSCXversion[sc->sc_hscx_version]);
				break;
				
			default:
				printf("%s: Error, HSCX version %d unknown!\n",
					sc->sc_dev.dv_xname, sc->sc_hscx_version);
				return;
		}
	}
	
	/* Map and establish the interrupt. */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: couldn't map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	psc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, isicintr, sc);
	if (psc->sc_ih == NULL) {
		printf("%s: couldn't establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/* ISAC setup */
	
	isic_isac_init(sc);

	/* HSCX setup */

	isic_bchannel_setup(sc, HSCX_CH_A, BPROT_NONE, 0);
	
	isic_bchannel_setup(sc, HSCX_CH_B, BPROT_NONE, 0);

	/* setup linktab */

	isic_init_linktab(sc);

	/* set trace level */

	sc->sc_trace = TRACE_OFF;

	sc->sc_state = ISAC_IDLE;

	sc->sc_ibuf = NULL;
	sc->sc_ib = NULL;
	sc->sc_ilen = 0;

	sc->sc_obuf = NULL;
	sc->sc_op = NULL;
	sc->sc_ol = 0;
	sc->sc_freeflag = 0;

	sc->sc_obuf2 = NULL;
	sc->sc_freeflag2 = 0;

#if defined(__FreeBSD__) && __FreeBSD__ >=3
	callout_handle_init(&sc->sc_T3_callout);
	callout_handle_init(&sc->sc_T4_callout);	
#endif
	
	/* init higher protocol layers */
	
	sc->sc_l2 = isdn_attach_layer1_bri(sc, sc->sc_dev.dv_xname, "some isic card", &isic_std_driver);
}
#endif
