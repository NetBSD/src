/*	$NetBSD: pcmcia.c,v 1.23 2000/07/28 19:17:02 drochner Exp $	*/

#define	PCMCIADEBUG

/*
 * Copyright (c) 1997 Marc Horowitz.  All rights reserved.
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
 *	This product includes software developed by Marc Horowitz.
 * 4. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_pcmciaverbose.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>


#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>
#ifdef IT8368E_LEGACY_MODE /* XXX -uch */
#include <arch/hpcmips/dev/it8368var.h>
#endif

#include "locators.h"

#ifdef PCMCIADEBUG
int	pcmcia_debug = 0;
#define	DPRINTF(arg) if (pcmcia_debug) printf arg
int	pcmciaintr_debug = 0;
/* this is done this way to avoid doing lots of conditionals
   at interrupt level.  */
#define PCMCIA_CARD_INTR (pcmciaintr_debug?pcmcia_card_intrdebug:pcmcia_card_intr)
#else
#define	DPRINTF(arg)
#define PCMCIA_CARD_INTR (pcmcia_card_intr)
#endif

#ifdef PCMCIAVERBOSE
int	pcmcia_verbose = 1;
#else
int	pcmcia_verbose = 0;
#endif

int	pcmcia_match __P((struct device *, struct cfdata *, void *));
int	pcmcia_submatch __P((struct device *, struct cfdata *, void *));
void	pcmcia_attach __P((struct device *, struct device *, void *));
int	pcmcia_print __P((void *, const char *));

static inline void pcmcia_socket_enable __P((pcmcia_chipset_tag_t,
					     pcmcia_chipset_handle_t *));
static inline void pcmcia_socket_disable __P((pcmcia_chipset_tag_t,
					      pcmcia_chipset_handle_t *));

int pcmcia_card_intr __P((void *));
#ifdef PCMCIADEBUG
int pcmcia_card_intrdebug __P((void *));
#endif

struct cfattach pcmcia_ca = {
	sizeof(struct pcmcia_softc), pcmcia_match, pcmcia_attach
};

int
pcmcia_ccr_read(pf, ccr)
	struct pcmcia_function *pf;
	int ccr;
{

	return (bus_space_read_1(pf->pf_ccrt, pf->pf_ccrh,
	    pf->pf_ccr_offset + ccr));
}

void
pcmcia_ccr_write(pf, ccr, val)
	struct pcmcia_function *pf;
	int ccr;
	int val;
{

	if ((pf->ccr_mask) & (1 << (ccr / 2))) {
		bus_space_write_1(pf->pf_ccrt, pf->pf_ccrh,
		    pf->pf_ccr_offset + ccr, val);
	}
}

int
pcmcia_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcmciabus_attach_args *paa = aux;

	if (strcmp(paa->paa_busname, match->cf_driver->cd_name)) {
	    return 0;
	}
	/* if the autoconfiguration got this far, there's a socket here */
	return (1);
}

void
pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcmciabus_attach_args *paa = aux;
	struct pcmcia_softc *sc = (struct pcmcia_softc *) self;

	printf("\n");

	sc->pct = paa->pct;
	sc->pch = paa->pch;
	sc->iobase = paa->iobase;
	sc->iosize = paa->iosize;

	sc->ih = NULL;
}

int
pcmcia_card_attach(dev)
	struct device *dev;
{
	struct pcmcia_softc *sc = (struct pcmcia_softc *) dev;
	struct pcmcia_function *pf;
	struct pcmcia_attach_args paa;
	int attached;

	/*
	 * this is here so that when socket_enable calls gettype, trt happens
	 */
	sc->card.pf_head.sqh_first = NULL;

	pcmcia_chip_socket_enable(sc->pct, sc->pch);

	pcmcia_read_cis(sc);

	pcmcia_chip_socket_disable(sc->pct, sc->pch);

	pcmcia_check_cis_quirks(sc);

	/*
	 * bail now if the card has no functions, or if there was an error in
	 * the cis.
	 */

	if (sc->card.error)
		return (1);
	if (sc->card.pf_head.sqh_first == NULL)
		return (1);

	if (pcmcia_verbose)
		pcmcia_print_cis(sc);

	attached = 0;

	for (pf = sc->card.pf_head.sqh_first; pf != NULL;
	    pf = pf->pf_list.sqe_next) {
		if (pf->cfe_head.sqh_first == NULL)
			continue;

#ifdef DIAGNOSTIC
		if (pf->child != NULL) {
			printf("%s: %s still attached to function %d!\n",
			    sc->dev.dv_xname, pf->child->dv_xname,
			    pf->number);
			panic("pcmcia_card_attach");
		}
#endif
		pf->sc = sc;
		pf->child = NULL;
		pf->cfe = NULL;
		pf->ih_fct = NULL;
		pf->ih_arg = NULL;
	}

	for (pf = sc->card.pf_head.sqh_first; pf != NULL;
	    pf = pf->pf_list.sqe_next) {
		if (pf->cfe_head.sqh_first == NULL)
			continue;

		paa.manufacturer = sc->card.manufacturer;
		paa.product = sc->card.product;
		paa.card = &sc->card;
		paa.pf = pf;

		if ((pf->child = config_found_sm(&sc->dev, &paa, pcmcia_print,
		    pcmcia_submatch)) != NULL)
			attached++;
	}

	return (attached ? 0 : 1);
}

void
pcmcia_card_detach(dev, flags)
	struct device *dev;
	int flags;		/* DETACH_* flags */
{
	struct pcmcia_softc *sc = (struct pcmcia_softc *) dev;
	struct pcmcia_function *pf;
	int error;

	/*
	 * We are running on either the PCMCIA socket's event thread
	 * or in user context detaching a device by user request.
	 */
	for (pf = SIMPLEQ_FIRST(&sc->card.pf_head); pf != NULL;
	     pf = SIMPLEQ_NEXT(pf, pf_list)) {
		if (SIMPLEQ_FIRST(&pf->cfe_head) == NULL)
			continue;
		if (pf->child == NULL)
			continue;
		DPRINTF(("%s: detaching %s (function %d)\n",
		    sc->dev.dv_xname, pf->child->dv_xname, pf->number));
		if ((error = config_detach(pf->child, flags)) != 0) {
			printf("%s: error %d detaching %s (function %d)\n",
			    sc->dev.dv_xname, error, pf->child->dv_xname,
			    pf->number);
		} else
			pf->child = NULL;
	}
}

void
pcmcia_card_deactivate(dev)
	struct device *dev;
{
	struct pcmcia_softc *sc = (struct pcmcia_softc *) dev;
	struct pcmcia_function *pf;

	/*
	 * We're in the chip's card removal interrupt handler.
	 * Deactivate the child driver.  The PCMCIA socket's
	 * event thread will run later to finish the detach.
	 */
	for (pf = SIMPLEQ_FIRST(&sc->card.pf_head); pf != NULL;
	     pf = SIMPLEQ_NEXT(pf, pf_list)) {
		if (SIMPLEQ_FIRST(&pf->cfe_head) == NULL)
			continue;
		if (pf->child == NULL)
			continue;
		DPRINTF(("%s: deactivating %s (function %d)\n",
		    sc->dev.dv_xname, pf->child->dv_xname, pf->number));
		config_deactivate(pf->child);
	}
}

int
pcmcia_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcmcia_attach_args *paa = aux;

	if (cf->cf_loc[PCMCIACF_FUNCTION] != PCMCIACF_FUNCTION_DEFAULT &&
	    cf->cf_loc[PCMCIACF_FUNCTION] != paa->pf->number)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
pcmcia_print(arg, pnp)
	void *arg;
	const char *pnp;
{
	struct pcmcia_attach_args *pa = arg;
	struct pcmcia_softc *sc = pa->pf->sc;
	struct pcmcia_card *card = &sc->card;
	char devinfo[256];

	if (pnp) {
		pcmcia_devinfo(card, 1, devinfo, sizeof devinfo);
		printf("%s at %s", devinfo, pnp);
	}
	printf(" function %d", pa->pf->number);

	return (UNCONF);
}

void
pcmcia_devinfo(card, showhex, cp, cplen)
	struct pcmcia_card *card;
	int showhex;
	char *cp;
	int cplen;
{
	int i, n;

	for (i = 0; i < 4 && card->cis1_info[i] != NULL && cplen > 0; i++) {
		n = snprintf(cp, cplen, "%s%s", i ? ", " : "",
		        card->cis1_info[i]);
		cp += n;
		cplen -= n;
	}
	if (showhex && cplen > 0)
		snprintf(cp, cplen, "%s(manufacturer 0x%04x, product 0x%04x)",
		    i ? " " : "", card->manufacturer, card->product);
}

const struct pcmcia_product *
pcmcia_product_lookup(pa, tab, ent_size, matchfn)
	struct pcmcia_attach_args *pa;
	const struct pcmcia_product *tab;
	size_t ent_size;
	pcmcia_product_match_fn matchfn;
{
        const struct pcmcia_product *ent;
	int matches;

#ifdef DIAGNOSTIC
	if (sizeof *ent > ent_size)
		panic("pcmcia_product_lookup: bogus ent_size %ld", 
		      (long) ent_size);
#endif

        for (ent = tab;
	    ent->pp_name != NULL;
	    ent = (const struct pcmcia_product *)
	      ((const char *)ent + ent_size)) {

		/* see if it matches vendor/product/function */
		matches = (pa->manufacturer == ent->pp_vendor) &&
		    (pa->product == ent->pp_product) &&
		    (pa->pf->number == ent->pp_expfunc);

		/* if a separate match function is given, let it override */
		if (matchfn != NULL)
			matches = (*matchfn)(pa, ent, matches);

		if (matches)
                        return (ent);
        }
        return (NULL);
}

int 
pcmcia_card_gettype(dev)
	struct device  *dev;
{
	struct pcmcia_softc *sc = (struct pcmcia_softc *)dev;
	struct pcmcia_function *pf;

	/*
	 * set the iftype to memory if this card has no functions (not yet
	 * probed), or only one function, and that is not initialized yet or
	 * that is memory.
	 */
	pf = SIMPLEQ_FIRST(&sc->card.pf_head);
	if (pf == NULL ||
	    (SIMPLEQ_NEXT(pf, pf_list) == NULL &&
	    (pf->cfe == NULL || pf->cfe->iftype == PCMCIA_IFTYPE_MEMORY)))
		return (PCMCIA_IFTYPE_MEMORY);
	else
		return (PCMCIA_IFTYPE_IO);
}

/*
 * Initialize a PCMCIA function.  May be called as long as the function is
 * disabled.
 */
void
pcmcia_function_init(pf, cfe)
	struct pcmcia_function *pf;
	struct pcmcia_config_entry *cfe;
{
	if (pf->pf_flags & PFF_ENABLED)
		panic("pcmcia_function_init: function is enabled");

	/* Remember which configuration entry we are using. */
	pf->cfe = cfe;
}

static inline void pcmcia_socket_enable(pct, pch)
     pcmcia_chipset_tag_t pct;
     pcmcia_chipset_handle_t *pch;
{
	pcmcia_chip_socket_enable(pct, pch);
}

static inline void pcmcia_socket_disable(pct, pch)
     pcmcia_chipset_tag_t pct;
     pcmcia_chipset_handle_t *pch;
{
	pcmcia_chip_socket_disable(pct, pch);
}

/* Enable a PCMCIA function */
int
pcmcia_function_enable(pf)
	struct pcmcia_function *pf;
{
	struct pcmcia_function *tmp;
	int reg;

	if (pf->cfe == NULL)
		panic("pcmcia_function_enable: function not initialized");

	/*
	 * Increase the reference count on the socket, enabling power, if
	 * necessary.
	 */
	if (pf->sc->sc_enabled_count++ == 0)
		pcmcia_chip_socket_enable(pf->sc->pct, pf->sc->pch);
	DPRINTF(("%s: ++enabled_count = %d\n", pf->sc->dev.dv_xname,
		 pf->sc->sc_enabled_count));

	if (pf->pf_flags & PFF_ENABLED) {
		/*
		 * Don't do anything if we're already enabled.
		 */
		return (0);
	}

	/*
	 * it's possible for different functions' CCRs to be in the same
	 * underlying page.  Check for that.
	 */

	for (tmp = pf->sc->card.pf_head.sqh_first; tmp != NULL;
	    tmp = tmp->pf_list.sqe_next) {
		if ((tmp->pf_flags & PFF_ENABLED) &&
		    (pf->ccr_base >= (tmp->ccr_base - tmp->pf_ccr_offset)) &&
		    ((pf->ccr_base + PCMCIA_CCR_SIZE) <=
		     (tmp->ccr_base - tmp->pf_ccr_offset +
		      tmp->pf_ccr_realsize))) {
			pf->pf_ccrt = tmp->pf_ccrt;
			pf->pf_ccrh = tmp->pf_ccrh;
			pf->pf_ccr_realsize = tmp->pf_ccr_realsize;

			/*
			 * pf->pf_ccr_offset = (tmp->pf_ccr_offset -
			 * tmp->ccr_base) + pf->ccr_base;
			 */
			pf->pf_ccr_offset =
			    (tmp->pf_ccr_offset + pf->ccr_base) -
			    tmp->ccr_base;
			pf->pf_ccr_window = tmp->pf_ccr_window;
			break;
		}
	}

	if (tmp == NULL) {
		if (pcmcia_mem_alloc(pf, PCMCIA_CCR_SIZE, &pf->pf_pcmh))
			goto bad;

		if (pcmcia_mem_map(pf, PCMCIA_MEM_ATTR, pf->ccr_base,
		    PCMCIA_CCR_SIZE, &pf->pf_pcmh, &pf->pf_ccr_offset,
		    &pf->pf_ccr_window)) {
			pcmcia_mem_free(pf, &pf->pf_pcmh);
			goto bad;
		}
	}

	reg = (pf->cfe->number & PCMCIA_CCR_OPTION_CFINDEX);
	reg |= PCMCIA_CCR_OPTION_LEVIREQ;
	if (pcmcia_mfc(pf->sc)) {
		reg |= (PCMCIA_CCR_OPTION_FUNC_ENABLE |
			PCMCIA_CCR_OPTION_ADDR_DECODE);
		if (pf->ih_fct)
			reg |= PCMCIA_CCR_OPTION_IREQ_ENABLE;

	}
	pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);

	reg = 0;

	if ((pf->cfe->flags & PCMCIA_CFE_IO16) == 0)
		reg |= PCMCIA_CCR_STATUS_IOIS8;
	if (pf->cfe->flags & PCMCIA_CFE_AUDIO)
		reg |= PCMCIA_CCR_STATUS_AUDIO;
	pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS, reg);

	pcmcia_ccr_write(pf, PCMCIA_CCR_SOCKETCOPY, 0);

	if (pcmcia_mfc(pf->sc)) {
		long tmp, iosize;

		tmp = pf->pf_mfc_iomax - pf->pf_mfc_iobase;
		/* round up to nearest (2^n)-1 */
		for (iosize = 1; iosize < tmp; iosize <<= 1)
			;
		iosize--;

		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE0,
				 pf->pf_mfc_iobase & 0xff);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE1,
				 (pf->pf_mfc_iobase >> 8) & 0xff);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE2, 0);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE3, 0);

		pcmcia_ccr_write(pf, PCMCIA_CCR_IOSIZE, iosize);
	}

#ifdef PCMCIADEBUG
	if (pcmcia_debug) {
		for (tmp = pf->sc->card.pf_head.sqh_first; tmp != NULL;
		     tmp = tmp->pf_list.sqe_next) {
			printf("%s: function %d CCR at %d offset %lx: "
			       "%x %x %x %x, %x %x %x %x, %x\n",
			       tmp->sc->dev.dv_xname, tmp->number,
			       tmp->pf_ccr_window,
			       (unsigned long) tmp->pf_ccr_offset,
			       pcmcia_ccr_read(tmp, 0x00),
			       pcmcia_ccr_read(tmp, 0x02),
			       pcmcia_ccr_read(tmp, 0x04),
			       pcmcia_ccr_read(tmp, 0x06),

			       pcmcia_ccr_read(tmp, 0x0A),
			       pcmcia_ccr_read(tmp, 0x0C), 
			       pcmcia_ccr_read(tmp, 0x0E),
			       pcmcia_ccr_read(tmp, 0x10),

			       pcmcia_ccr_read(tmp, 0x12));
		}
	}
#endif

	pf->pf_flags |= PFF_ENABLED;

#ifdef IT8368E_LEGACY_MODE
	/* return to I/O mode */
	it8368_mode(pf, IT8368_IO_MODE, IT8368_WIDTH_16);
#endif
	return (0);

 bad:
	/*
	 * Decrement the reference count, and power down the socket, if
	 * necessary.
	 */
	if (--pf->sc->sc_enabled_count == 0)
		pcmcia_chip_socket_disable(pf->sc->pct, pf->sc->pch);
	DPRINTF(("%s: --enabled_count = %d\n", pf->sc->dev.dv_xname,
		 pf->sc->sc_enabled_count));

	return (1);
}

/* Disable PCMCIA function. */
void
pcmcia_function_disable(pf)
	struct pcmcia_function *pf;
{
	struct pcmcia_function *tmp;

	if (pf->cfe == NULL)
		panic("pcmcia_function_enable: function not initialized");

	if ((pf->pf_flags & PFF_ENABLED) == 0) {
		/*
		 * Don't do anything if we're already disabled.
		 */
		return;
	}

	/*
	 * it's possible for different functions' CCRs to be in the same
	 * underlying page.  Check for that.  Note we mark us as disabled
	 * first to avoid matching ourself.
	 */

	pf->pf_flags &= ~PFF_ENABLED;
	for (tmp = pf->sc->card.pf_head.sqh_first; tmp != NULL;
	    tmp = tmp->pf_list.sqe_next) {
		if ((tmp->pf_flags & PFF_ENABLED) &&
		    (pf->ccr_base >= (tmp->ccr_base - tmp->pf_ccr_offset)) &&
		    ((pf->ccr_base + PCMCIA_CCR_SIZE) <=
		(tmp->ccr_base - tmp->pf_ccr_offset + tmp->pf_ccr_realsize)))
			break;
	}

	/* Not used by anyone else; unmap the CCR. */
	if (tmp == NULL) {
		pcmcia_mem_unmap(pf, pf->pf_ccr_window);
		pcmcia_mem_free(pf, &pf->pf_pcmh);
	}

	/*
	 * Decrement the reference count, and power down the socket, if
	 * necessary.
	 */
	if (--pf->sc->sc_enabled_count == 0)
		pcmcia_chip_socket_disable(pf->sc->pct, pf->sc->pch);
	DPRINTF(("%s: --enabled_count = %d\n", pf->sc->dev.dv_xname,
		 pf->sc->sc_enabled_count));
}

int
pcmcia_io_map(pf, width, offset, size, pcihp, windowp)
	struct pcmcia_function *pf;
	int width;
	bus_addr_t offset;
	bus_size_t size;
	struct pcmcia_io_handle *pcihp;
	int *windowp;
{
	int reg;

	if (pcmcia_chip_io_map(pf->sc->pct, pf->sc->pch,
	    width, offset, size, pcihp, windowp))
		return (1);

	/*
	 * XXX in the multifunction multi-iospace-per-function case, this
	 * needs to cooperate with io_alloc to make sure that the spaces
	 * don't overlap, and that the ccr's are set correctly
	 */

	if (pcmcia_mfc(pf->sc)) {
		long tmp, iosize;

		if (pf->pf_mfc_iomax == 0) {
			pf->pf_mfc_iobase = pcihp->addr + offset;
			pf->pf_mfc_iomax = pf->pf_mfc_iobase + size;
		} else {
			/* this makes the assumption that nothing overlaps */
			if (pf->pf_mfc_iobase > pcihp->addr + offset)
				pf->pf_mfc_iobase = pcihp->addr + offset;
			if (pf->pf_mfc_iomax < pcihp->addr + offset + size)
				pf->pf_mfc_iomax = pcihp->addr + offset + size;
		}

		tmp = pf->pf_mfc_iomax - pf->pf_mfc_iobase;
		/* round up to nearest (2^n)-1 */
		for (iosize = 1; iosize >= tmp; iosize <<= 1)
			;
		iosize--;

		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE0,
				 pf->pf_mfc_iobase & 0xff);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE1,
				 (pf->pf_mfc_iobase >> 8) & 0xff);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE2, 0);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE3, 0);

		pcmcia_ccr_write(pf, PCMCIA_CCR_IOSIZE, iosize);

		reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
		reg |= PCMCIA_CCR_OPTION_ADDR_DECODE;
		pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);
	}
	return (0);
}

void
pcmcia_io_unmap(pf, window)
	struct pcmcia_function *pf;
	int window;
{

	pcmcia_chip_io_unmap(pf->sc->pct, pf->sc->pch, window);

	/* XXX Anything for multi-function cards? */
}

void *
pcmcia_intr_establish(pf, ipl, ih_fct, ih_arg)
	struct pcmcia_function *pf;
	int ipl;
	int (*ih_fct) __P((void *));
	void *ih_arg;
{
	void *ret;

	/* behave differently if this is a multifunction card */

	if (pcmcia_mfc(pf->sc)) {
		int s, ihcnt, hiipl, reg;
		struct pcmcia_function *pf2;

		/*
		 * mask all the ipl's which are already used by this card,
		 * and find the highest ipl number (lowest priority)
		 */

		ihcnt = 0;
		s = 0;		/* this is only here to keep the compiler
				   happy */
		hiipl = 0;	/* this is only here to keep the compiler
				   happy */

		for (pf2 = pf->sc->card.pf_head.sqh_first; pf2 != NULL;
		     pf2 = pf2->pf_list.sqe_next) {
			if (pf2->ih_fct) {
				DPRINTF(("%s: function %d has ih_fct %p\n",
					 pf->sc->dev.dv_xname, pf2->number,
					 pf2->ih_fct));

				if (ihcnt == 0) {
					hiipl = pf2->ih_ipl;
				} else {
					if (pf2->ih_ipl > hiipl)
						hiipl = pf2->ih_ipl;
				}

				ihcnt++;
			}
		}

		/*
		 * establish the real interrupt, changing the ipl if
		 * necessary
		 */

		if (ihcnt == 0) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih != NULL)
				panic("card has intr handler, but no function does");
#endif
			s = splhigh();

			/* set up the handler for the new function */

			pf->ih_fct = ih_fct;
			pf->ih_arg = ih_arg;
			pf->ih_ipl = ipl;

			pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct,
			    pf->sc->pch, pf, ipl, PCMCIA_CARD_INTR, pf->sc);
			splx(s);
		} else if (ipl > hiipl) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih == NULL)
				panic("functions have ih, but the card does not");
#endif

			/* XXX need #ifdef for splserial on x86 */
			s = splhigh();

			pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
						      pf->sc->ih);

			/* set up the handler for the new function */
			pf->ih_fct = ih_fct;
			pf->ih_arg = ih_arg;
			pf->ih_ipl = ipl;

			pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct,
			    pf->sc->pch, pf, ipl, PCMCIA_CARD_INTR, pf->sc);

			splx(s);
		} else {
			s = splhigh();

			/* set up the handler for the new function */

			pf->ih_fct = ih_fct;
			pf->ih_arg = ih_arg;
			pf->ih_ipl = ipl;

			splx(s);
		}

		ret = pf->sc->ih;

		if (ret != NULL) {
			reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
			reg |= PCMCIA_CCR_OPTION_IREQ_ENABLE;
			pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);

			reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
			reg |= PCMCIA_CCR_STATUS_INTRACK;
			pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS, reg);
		}
	} else {
		ret = pcmcia_chip_intr_establish(pf->sc->pct, pf->sc->pch,
		    pf, ipl, ih_fct, ih_arg);
	}

	return (ret);
}

void
pcmcia_intr_disestablish(pf, ih)
	struct pcmcia_function *pf;
	void *ih;
{
	/* behave differently if this is a multifunction card */

	if (pcmcia_mfc(pf->sc)) {
		int s, ihcnt, hiipl;
		struct pcmcia_function *pf2;

		/*
		 * mask all the ipl's which are already used by this card,
		 * and find the highest ipl number (lowest priority).  Skip
		 * the current function.
		 */

		ihcnt = 0;
		s = 0;		/* avoid compiler warning */
		hiipl = 0;	/* avoid compiler warning */

		for (pf2 = pf->sc->card.pf_head.sqh_first; pf2 != NULL;
		     pf2 = pf2->pf_list.sqe_next) {
			if (pf2 == pf)
				continue;

			if (pf2->ih_fct) {
				if (ihcnt == 0) {
					hiipl = pf2->ih_ipl;
				} else {
					if (pf2->ih_ipl > hiipl)
						hiipl = pf2->ih_ipl;
				}
				ihcnt++;
			}
		}

		/*
		 * If the ih being removed is lower priority than the lowest
		 * priority remaining interrupt, up the priority.
		 */

		/* 
		 * ihcnt is the number of interrupt handlers *not* including
		 * the one about to be removed. 
		 */

		if (ihcnt == 0) {
			int reg;

#ifdef DIAGNOSTIC
			if (pf->sc->ih == NULL)
				panic("disestablishing last function, but card has no ih");
#endif
			pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
			    pf->sc->ih);

			reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
			reg &= ~PCMCIA_CCR_OPTION_IREQ_ENABLE;
			pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);

			pf->ih_fct = NULL;
			pf->ih_arg = NULL;

			pf->sc->ih = NULL;
		} else if (pf->ih_ipl > hiipl) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih == NULL)
				panic("changing ih ipl, but card has no ih");
#endif
			/* XXX need #ifdef for splserial on x86 */
			s = splhigh();

			pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
			    pf->sc->ih);
			pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct,
			    pf->sc->pch, pf, hiipl, PCMCIA_CARD_INTR, pf->sc);

			/* null out the handler for this function */

			pf->ih_fct = NULL;
			pf->ih_arg = NULL;

			splx(s);
		} else {
			s = splhigh();

			pf->ih_fct = NULL;
			pf->ih_arg = NULL;

			splx(s);
		}
	} else {
		pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch, ih);
	}
}

int 
pcmcia_card_intr(arg)
	void *arg;
{
	struct pcmcia_softc *sc = arg;
	struct pcmcia_function *pf;
	int reg, ret, ret2;

	ret = 0;

	for (pf = sc->card.pf_head.sqh_first; pf != NULL;
	    pf = pf->pf_list.sqe_next) {
		if (pf->ih_fct != NULL &&
		    (pf->ccr_mask & (1 << (PCMCIA_CCR_STATUS / 2)))) {
			reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
			if (reg & PCMCIA_CCR_STATUS_INTR) {
				ret2 = (*pf->ih_fct)(pf->ih_arg);
				if (ret2 != 0 && ret == 0)
					ret = ret2;
				reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
				pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS,
				    reg & ~PCMCIA_CCR_STATUS_INTR);
			}
		}
	}

	return (ret);
}

#ifdef PCMCIADEBUG
int 
pcmcia_card_intrdebug(arg)
	void *arg;
{
	struct pcmcia_softc *sc = arg;
	struct pcmcia_function *pf;
	int reg, ret, ret2;

	ret = 0;

	for (pf = sc->card.pf_head.sqh_first; pf != NULL;
	    pf = pf->pf_list.sqe_next) {
		printf("%s: intr flags=%x fct=%d cor=%02x csr=%02x pin=%02x",
		       sc->dev.dv_xname, pf->pf_flags, pf->number,
		       pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION),
		       pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS),
		       pcmcia_ccr_read(pf, PCMCIA_CCR_PIN));
		if (pf->ih_fct != NULL &&
		    (pf->ccr_mask & (1 << (PCMCIA_CCR_STATUS / 2)))) {
			reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
			if (reg & PCMCIA_CCR_STATUS_INTR) {
				ret2 = (*pf->ih_fct)(pf->ih_arg);
				if (ret2 != 0 && ret == 0)
					ret = ret2;
				reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
				printf("; csr %02x->%02x",
				    reg, reg & ~PCMCIA_CCR_STATUS_INTR);
				pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS,
				    reg & ~PCMCIA_CCR_STATUS_INTR);
			}
		}
		printf("\n");
	}

	return (ret);
}
#endif
