/*
 *   Copyright (c) 1998 Martin Husemann. All rights reserved.
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
 *	isic_pcmcia.c - pcmcia bus frontend for i4b_isic driver
 *	-------------------------------------------------------
 *
 *	$Id: isic_pcmcia.c,v 1.13 2002/04/10 23:51:09 martin Exp $ 
 *
 *      last edit-date: [Fri Jan  5 11:39:32 2001]
 *
 *	-mh	original implementation
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: isic_pcmcia.c,v 1.13 2002/04/10 23:51:09 martin Exp $");

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciadevs.h>

#ifdef __FreeBSD__
#include <machine/i4b_ioctl.h>
#include <machine/i4b_trace.h>
#else
#include <netisdn/i4b_ioctl.h>
#include <netisdn/i4b_trace.h>
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_l2.h>
#include <netisdn/i4b_l1l2.h>
#endif

#include <dev/ic/isic_l1.h>
#include <dev/ic/ipac.h>
#include <dev/ic/isac.h>
#include <dev/ic/hscx.h>

#include <netisdn/i4b_l1l2.h>
#include <netisdn/i4b_global.h>

#include <dev/pcmcia/isic_pcmcia.h>

#include "opt_isicpcmcia.h"

extern const struct isdn_layer1_bri_driver isic_std_driver;

static int isic_pcmcia_match __P((struct device *, struct cfdata *, void *));
static void isic_pcmcia_attach __P((struct device *, struct device *, void *));
static const struct isic_pcmcia_card_entry * find_matching_card __P((struct pcmcia_attach_args *pa));
static int isic_pcmcia_isdn_attach __P((struct isic_softc *sc, const char*));
static int isic_pcmcia_detach(struct device *self, int flags);
static int isic_pcmcia_activate(struct device *self, enum devact act);

struct cfattach isic_pcmcia_ca = {
	sizeof(struct pcmcia_isic_softc), isic_pcmcia_match, 
	isic_pcmcia_attach, isic_pcmcia_detach, isic_pcmcia_activate
};

struct isic_pcmcia_card_entry {
	int32_t vendor;		/* vendor ID */
	int32_t product;	/* product ID */
	char *cis1_info[4];	/* CIS info to match */
	char *name;		/* name of controller */
	int function;		/* expected PCMCIA function type */
	int card_type;		/* card type found */
	isic_pcmcia_attach_func attach;	/* card initialization */
};

static const struct isic_pcmcia_card_entry card_list[] = {

#ifdef ISICPCMCIA_AVM_A1
    {   PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
    	{ "AVM", "ISDN A", NULL, NULL },
        "AVM Fritz!Card", PCMCIA_FUNCTION_NETWORK,
	CARD_TYPEP_PCFRITZ, isic_attach_fritzpcmcia },
#endif

#ifdef ISICPCMCIA_ELSA_ISDNMC
    {	PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
    	{ "ELSA GmbH, Aachen", "MicroLink ISDN/MC ", NULL, NULL },
        "ELSA MicroLink ISDN/MC", PCMCIA_FUNCTION_NETWORK,
	CARD_TYPEP_ELSAMLIMC, isic_attach_elsaisdnmc },
    {	PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
    	{ "ELSA AG, Aachen", "MicroLink ISDN/MC ", NULL, NULL },
        "ELSA MicroLink ISDN/MC", PCMCIA_FUNCTION_NETWORK,
	CARD_TYPEP_ELSAMLIMC, isic_attach_elsaisdnmc },
    {	PCMCIA_VENDOR_INVALID, PCMCIA_PRODUCT_INVALID,
    	{ "ELSA AG (Aachen, Germany)", "MicroLink ISDN/MC ", NULL, NULL },
        "ELSA MicroLink ISDN/MC", PCMCIA_FUNCTION_NETWORK,
	CARD_TYPEP_ELSAMLIMC, isic_attach_elsaisdnmc },
#endif

#ifdef ISICPCMCIA_ELSA_MCALL
    {	0x105, 0x410a,
        { "ELSA", "MicroLink MC all", NULL, NULL },
        "ELSA MicroLink MCall", PCMCIA_FUNCTION_NETWORK,
	CARD_TYPEP_ELSAMLMCALL, isic_attach_elsamcall },
#endif

#ifdef ISICPCMCIA_SBSPEEDSTAR2
    {	0x020e, 0x0002,
        { "SEDLBAUER", "speed star II", NULL, NULL },
        "SEDLBAUER speed star II", PCMCIA_FUNCTION_NETWORK,
	CARD_TYPEP_SWS, isic_attach_sbspeedstar2 },
#endif

};
#define	NUM_MATCH_ENTRIES	(sizeof(card_list)/sizeof(card_list[0]))

static const struct isic_pcmcia_card_entry *
find_matching_card(pa)
	struct pcmcia_attach_args *pa;
{
	int i, j;

	for (i = 0; i < NUM_MATCH_ENTRIES; i++) {
		if (card_list[i].vendor != PCMCIA_VENDOR_INVALID && pa->card->manufacturer != card_list[i].vendor)
			continue;
		if (card_list[i].product != PCMCIA_PRODUCT_INVALID && pa->card->product != card_list[i].product)
				continue;
		if (pa->pf->function != card_list[i].function)
			continue;
		for (j = 0; j < 4; j++) {
			if (card_list[i].cis1_info[j] == NULL)
				continue;	/* wildcard */
			if (pa->card->cis1_info[j] == NULL)
				break;		/* not available */
			if (strcmp(pa->card->cis1_info[j], card_list[i].cis1_info[j]) != 0)
				break;		/* mismatch */
		}
		if (j >= 4)
			break;
	}
	if (i >= NUM_MATCH_ENTRIES)
		return NULL;

	return &card_list[i];
}

/*
 * Match card
 */
static int
isic_pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmcia_attach_args *pa = aux;

	if (!find_matching_card(pa))
		return 0;

	return 1;
}

/*
 * Attach the card
 */
static void
isic_pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcmcia_isic_softc *psc = (void*) self;
	struct isic_softc *sc = &psc->sc_isic;
	struct pcmcia_attach_args *pa = aux;
	struct pcmcia_config_entry *cfe;
	const struct isic_pcmcia_card_entry * cde;
	int s;

	psc->sc_pf = pa->pf;
	cfe = pa->pf->cfe_head.sqh_first;
	psc->sc_ih = NULL;

	/* Which card is it? */
	cde = find_matching_card(pa);
	if (cde == NULL)
		goto bad2;	/* oops - not found?!? */

	/* Enable the card */
	pcmcia_function_init(pa->pf, cfe);
	pcmcia_function_enable(pa->pf);

	if (!cde->attach(psc, cfe, pa))
		goto bad;	/* Ooops ? */

	/* Announce card name */
	printf(": %s\n", cde->name);

	/* XXX - we generate interrupts during card initialization.
	   Block them for now, until the handler is established. */
	s = splhigh();
	
	/* MI initilization */
	sc->sc_cardtyp = cde->card_type;
	if (isic_pcmcia_isdn_attach(sc, cde->name) == 0) {
		/* setup interrupt */
		psc->sc_ih = pcmcia_intr_establish(pa->pf, IPL_NET, isicintr, sc);
	} else
		goto bad;

	splx(s);
	return;
bad:
	pcmcia_function_disable(psc->sc_pf);
	splx(s);
bad2:
	printf("%s: attach failed\n", psc->sc_isic.sc_dev.dv_xname);
}

static int
isic_pcmcia_detach(self, flags)
	struct device *self;
	int flags;
{
	struct pcmcia_isic_softc *psc = (struct pcmcia_isic_softc *)self;

	pcmcia_function_disable(psc->sc_pf);
	pcmcia_io_unmap(psc->sc_pf, psc->sc_io_window);
	pcmcia_io_free(psc->sc_pf, &psc->sc_pcioh);
	if (psc->sc_ih != NULL)
		pcmcia_intr_disestablish(psc->sc_pf, psc->sc_ih);

	return (0);
}

int
isic_pcmcia_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct pcmcia_isic_softc *psc = (struct pcmcia_isic_softc *)self;
	int error = 0, s;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		psc->sc_isic.sc_intr_valid = ISIC_INTR_DYING;
		if (psc->sc_isic.sc_l3token != NULL)
			isic_detach_bri(&psc->sc_isic);
		break;
	}
	splx(s);
	return (error);
}
	
/*---------------------------------------------------------------------------*
 *	card independend attach for pcmicia cards
 *---------------------------------------------------------------------------*/

/* parameter and format for message producing e.g. "isic0: " */

#ifdef __FreeBSD__
#define	ISIC_FMT	"isic%d: "
#define	ISIC_PARM	dev->id_unit
#define	TERMFMT	" "
#else
#define	ISIC_FMT	"%s: "
#define	ISIC_PARM	sc->sc_dev.dv_xname
#define	TERMFMT	"\n"
#endif

int
isic_pcmcia_isdn_attach(struct isic_softc *sc, const char *cardname)
{
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

	sc->sc_l3token = NULL;
	sc->sc_isac_version = 0;
	sc->sc_isac_version = ((ISAC_READ(I_RBCH)) >> 5) & 0x03;

	switch(sc->sc_isac_version)
	{
		case ISAC_VA:
		case ISAC_VB1:
                case ISAC_VB2:
		case ISAC_VB3:
			break;

		default:
			printf(ISIC_FMT "Error, ISAC version %d unknown!\n",
				ISIC_PARM, sc->sc_isac_version);
			return(EIO);
	}

	sc->sc_hscx_version = HSCX_READ(0, H_VSTR) & 0xf;

	switch(sc->sc_hscx_version)
	{
		case HSCX_VA1:
		case HSCX_VA2:
		case HSCX_VA3:
		case HSCX_V21:
			break;

		default:
			printf(ISIC_FMT "Error, HSCX version %d unknown!\n",
				ISIC_PARM, sc->sc_hscx_version);
			return(EIO);
	};

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

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
	callout_init(&sc->sc_T3_callout);
	callout_init(&sc->sc_T4_callout);
#endif

	/* announce chip versions */
	
	if(sc->sc_isac_version >= ISAC_UNKN)
	{
		printf(ISIC_FMT "ISAC Version UNKNOWN (VN=0x%x)" TERMFMT,
				ISIC_PARM,
				sc->sc_isac_version);
		sc->sc_isac_version = ISAC_UNKN;
	}
	else
	{
		printf(ISIC_FMT "ISAC %s (IOM-%c)" TERMFMT,
				ISIC_PARM,
				ISACversion[sc->sc_isac_version],
				sc->sc_bustyp == BUS_TYPE_IOM1 ? '1' : '2');
	}

	if(sc->sc_hscx_version >= HSCX_UNKN)
	{
		printf(ISIC_FMT "HSCX Version UNKNOWN (VN=0x%x)" TERMFMT,
				ISIC_PARM,
				sc->sc_hscx_version);
		sc->sc_hscx_version = HSCX_UNKN;
	}
	else
	{
		printf(ISIC_FMT "HSCX %s" TERMFMT,
				ISIC_PARM,
				HSCXversion[sc->sc_hscx_version]);
	}

	/* init higher protocol layers */
	isic_attach_bri(sc, cardname, &isic_std_driver);

	return(0);
}

