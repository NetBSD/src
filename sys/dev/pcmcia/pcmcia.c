/*	$NetBSD: pcmcia.c,v 1.2 1997/10/16 23:27:30 thorpej Exp $	*/

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

/* XXX only needed for intr debugging */
#include <vm/vm.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciachip.h>
#include <dev/pcmcia/pcmciavar.h>

#ifdef PCMCIADEBUG
int	pcmcia_debug = 0;
#define	DPRINTF(arg) if (pcmcia_debug) printf arg
#else
#define	DPRINTF(arg)
#endif

#ifdef PCMCIAVERBOSE
int	pcmcia_verbose = 1;
#else
int	pcmcia_verbose = 0;
#endif

#ifdef	__BROKEN_INDIRECT_CONFIG
int	pcmcia_match __P((struct device *, void *, void *));
int	pcmcia_submatch __P((struct device *, void *, void *));
#else
int	pcmcia_match __P((struct device *, struct cfdata *, void *));
int	pcmcia_submatch __P((struct device *, struct cfdata *, void *));
#endif
void	pcmcia_attach __P((struct device *, struct device *, void *));
int	pcmcia_print __P((void *, const char *));

int	pcmcia_card_intr __P((void *));

struct cfdriver pcmcia_cd = {
	NULL, "pcmcia", DV_DULL
};

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
#ifdef	__BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{

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

		pf->sc = sc;
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

		if (config_found_sm(&sc->dev, &paa, pcmcia_print,
		    pcmcia_submatch)) {
			attached++;

			DPRINTF(("%s: function %d CCR at %d "
			     "offset %lx: %x %x %x %x, %x %x %x %x, %x\n",
			     sc->dev.dv_xname, pf->number,
			     pf->pf_ccr_window, pf->pf_ccr_offset,
			     pcmcia_ccr_read(pf, 0x00),
			pcmcia_ccr_read(pf, 0x02), pcmcia_ccr_read(pf, 0x04),
			pcmcia_ccr_read(pf, 0x06), pcmcia_ccr_read(pf, 0x0A),
			pcmcia_ccr_read(pf, 0x0C), pcmcia_ccr_read(pf, 0x0E),
			pcmcia_ccr_read(pf, 0x10), pcmcia_ccr_read(pf, 0x12)));
		}
	}

	return (attached ? 0 : 1);
}

void
pcmcia_card_detach(dev)
	struct device *dev;
{
	/* struct pcmcia_softc *sc = (struct pcmcia_softc *) dev; */
	/* don't do anything yet */
}

int
#ifdef __BROKEN_INDIRECT_CONFIG
pcmcia_submatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;
#else
pcmcia_submatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
#endif
	struct pcmcia_attach_args *paa = aux;

	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != paa->pf->number)
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
	int i;

	if (pnp) {
		for (i = 0; i < 4; i++) {
			if (card->cis1_info[i] == NULL)
				break;
			if (i)
				printf(", ");
			printf("%s", card->cis1_info[i]);
		}
		if (i)
			printf(" ");
		printf("(manufacturer 0x%x, product 0x%x)", card->manufacturer,
		       card->product);
	}
	printf(" function %d", pa->pf->number);

	return (UNCONF);
}

int 
pcmcia_card_gettype(dev)
	struct device  *dev;
{
	struct pcmcia_softc *sc = (struct pcmcia_softc *) dev;

	/*
	 * set the iftype to memory if this card has no functions (not yet
	 * probed), or only one function, and that is memory.
	 */
	if (sc->card.pf_head.sqh_first == NULL ||
	    (sc->card.pf_head.sqh_first != NULL &&
	     sc->card.pf_head.sqh_first->pf_list.sqe_next == NULL &&
	     (sc->card.pf_head.sqh_first->cfe_head.sqh_first->iftype ==
	      PCMCIA_IFTYPE_MEMORY)))
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

/* Enable a PCMCIA function */
int
pcmcia_function_enable(pf)
	struct pcmcia_function *pf;
{
	struct pcmcia_function *tmp;
	int reg;

	if (pf->cfe == NULL)
		panic("pcmcia_function_enable: function not initialized");

	if (pf->pf_flags & PFF_ENABLED) {
		/*
		 * Don't do anything if we're already enabled.
		 */
		return (0);
	}

	/*
	 * Increase the reference count on the socket, enabling power, if
	 * necessary.
	 */
	if (pf->sc->sc_enabled_count++ == 0)
		pcmcia_chip_socket_enable(pf->sc->pct, pf->sc->pch);

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
	DPRINTF(("%s: function %d CCR at %d offset %lx: "
	    "%x %x %x %x, %x %x %x %x, %x\n",
	    pf->sc->dev.dv_xname, pf->number,
	    pf->pf_ccr_window, pf->pf_ccr_offset,
	    pcmcia_ccr_read(pf, 0x00),
	    pcmcia_ccr_read(pf, 0x02), pcmcia_ccr_read(pf, 0x04),
	    pcmcia_ccr_read(pf, 0x06), pcmcia_ccr_read(pf, 0x0A),
	    pcmcia_ccr_read(pf, 0x0C), pcmcia_ccr_read(pf, 0x0E),
	    pcmcia_ccr_read(pf, 0x10), pcmcia_ccr_read(pf, 0x12)));

	reg = (pf->cfe->number & PCMCIA_CCR_OPTION_CFINDEX);
	reg |= PCMCIA_CCR_OPTION_LEVIREQ;
	if (pcmcia_mfc(pf->sc))
		reg |= PCMCIA_CCR_OPTION_FUNC_ENABLE;
	pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);

	reg = 0;

	if ((pf->cfe->flags & PCMCIA_CFE_IO16) == 0)
		reg |= PCMCIA_CCR_STATUS_IOIS8;
	if (pf->cfe->flags & PCMCIA_CFE_AUDIO)
		reg |= PCMCIA_CCR_STATUS_AUDIO;
	/* Not really needed, since we start with 0. */
	if (pf->cfe->flags & PCMCIA_CFE_POWERDOWN)
		reg &= ~PCMCIA_CCR_STATUS_PWRDWN;
	pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS, reg);

	pcmcia_ccr_write(pf, PCMCIA_CCR_SOCKETCOPY, 0);

	pf->pf_flags |= PFF_ENABLED;
	return (0);

 bad:
	/*
	 * Decrement the reference count, and power down the socket, if
	 * necessary.
	 */
	if (pf->sc->sc_enabled_count-- == 1)
		pcmcia_chip_socket_disable(pf->sc->pct, pf->sc->pch);
	return (1);
}

/* Disable PCMCIA function. */
void
pcmcia_function_disable(pf)
	struct pcmcia_function *pf;
{
	struct pcmcia_function *tmp;
	int reg;

	if (pf->cfe == NULL)
		panic("pcmcia_function_enable: function not initialized");

	if ((pf->pf_flags & PFF_ENABLED) == 0) {
		/*
		 * Don't do anything if we're already disabled.
		 */
		return;
	}

	/* Power down the function if the card supports it. */
	if (pf->cfe->flags & PCMCIA_CFE_POWERDOWN) {
		reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
		reg |= PCMCIA_CCR_STATUS_PWRDWN;
		pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS, reg);
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
	bus_addr_t ioaddr;
	int reg;

	if (pcmcia_chip_io_map(pf->sc->pct, pf->sc->pch,
	    width, offset, size, pcihp, windowp))
		return (1);

	ioaddr = pcihp->addr + offset;

	/*
	 * XXX in the multifunction multi-iospace-per-function case, this
	 * needs to cooperate with io_alloc to make sure that the spaces
	 * don't overlap, and that the ccr's are set correctly
	 */

	if (pcmcia_mfc(pf->sc)) {
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE0, ioaddr & 0xff);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE1, (ioaddr >> 8) & 0xff);
		pcmcia_ccr_write(pf, PCMCIA_CCR_IOSIZE, size - 1);

		reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
		reg |= PCMCIA_CCR_OPTION_ADDR_DECODE;
		pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);
	}
	return (0);
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

		/* XXX splraise() use needs to go away! */

		/*
		 * mask all the ipl's which are already used by this card,
		 * and find the highest ipl number (lowest priority)
		 */

		ihcnt = 0;
		s = 0;		/* this is only here to keep the compipler
				   happy */
		hiipl = 0;	/* this is only here to keep the compipler
				   happy */

		for (pf2 = pf->sc->card.pf_head.sqh_first; pf2 != NULL;
		     pf2 = pf2->pf_list.sqe_next) {
			if (pf2->ih_fct) {
				if (ihcnt == 0) {
					s = splraise(pf2->ih_ipl);
					hiipl = pf2->ih_ipl;
					ihcnt++;
				} else {
					splraise(pf2->ih_ipl);
					if (pf2->ih_ipl > hiipl)
						hiipl = pf2->ih_ipl;
				}
			}
		}

		/* set up the handler for the new function */

		pf->ih_fct = ih_fct;
		pf->ih_arg = ih_arg;
		pf->ih_ipl = ipl;

		/*
		 * establish the real interrupt, changing the ipl if
		 * necessary
		 */

		if (ihcnt == 0) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih != NULL)
				panic("card has intr handler, but no function does");
#endif

			pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct,
			    pf->sc->pch, pf, ipl, pcmcia_card_intr, pf->sc);
		} else if (ipl > hiipl) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih == NULL)
				panic("functions have ih, but the card does not");
#endif

			pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
			    pf->sc->ih);
			pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct,
			    pf->sc->pch, pf, ipl, pcmcia_card_intr, pf->sc);
		}
		if (ihcnt)
			splx(s);

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
		s = 0;		/* this is only here to keep the compipler
				   happy */
		hiipl = 0;	/* this is only here to keep the compipler
				   happy */

		for (pf2 = pf->sc->card.pf_head.sqh_first; pf2 != NULL;
		     pf2 = pf2->pf_list.sqe_next) {
			if (pf2 == pf)
				continue;

			if (pf2->ih_fct) {
				if (ihcnt == 0) {
					s = splraise(pf2->ih_ipl);
					hiipl = pf2->ih_ipl;
					ihcnt++;
				} else {
					splraise(pf2->ih_ipl);
					if (pf2->ih_ipl > hiipl)
						hiipl = pf2->ih_ipl;
				}
			}
		}

		/* null out the handler for this function */

		pf->ih_fct = NULL;
		pf->ih_arg = NULL;

		/*
		 * if the ih being removed is lower priority than the lowest
		 * priority remaining interrupt, up the priority.
		 */

#ifdef DIAGNOSTIC
		if (ihcnt == 0) {
			panic("can't remove a handler from a card which has none");
		} else
#endif
		if (ihcnt == 1) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih == NULL)
				panic("disestablishing last function, but card has no ih");
#endif
			pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
			    pf->sc->ih);
			pf->sc->ih = NULL;
		} else if (pf->ih_ipl > hiipl) {
#ifdef DIAGNOSTIC
			if (pf->sc->ih == NULL)
				panic("changing ih ipl, but card has no ih");
#endif
			pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
			    pf->sc->ih);
			pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct,
			    pf->sc->pch, pf, hiipl, pcmcia_card_intr, pf->sc);
		}
		if (ihcnt)
			splx(s);
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
#if 0
		printf("%s: intr fct=%d physaddr=%lx cor=%02x csr=%02x pin=%02x",
		       sc->dev.dv_xname, pf->number,
		       pmap_extract(pmap_kernel(),
		           (vm_offset_t) pf->ccrh) + pf->ccr_offset,
		       bus_space_read_1(pf->pf_ccrt, pf->pf_ccrh,
		           pf->pf_ccr_offset + PCMCIA_CCR_OPTION),
		       bus_space_read_1(pf->pf_ccrt, pf->pf_ccrh,
		           pf->pf_ccr_offset + PCMCIA_CCR_STATUS),
		       bus_space_read_1(pf->pf_ccrt, pf->pf_ccrh,
		           pf->pf_ccr_offset + PCMCIA_CCR_PIN));
#endif
		if (pf->ih_fct != NULL &&
		    (pf->ccr_mask & (1 << (PCMCIA_CCR_STATUS / 2)))) {
			reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
			if (reg & PCMCIA_CCR_STATUS_INTR) {
				ret2 = (*pf->ih_fct)(pf->ih_arg);
				if (ret2 != 0 && ret == 0)
					ret = ret2;
				reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
#if 0
				printf("; csr %02x->%02x",
				    reg, reg & ~PCMCIA_CCR_STATUS_INTR);
#endif
				pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS,
				    reg & ~PCMCIA_CCR_STATUS_INTR);
			}
		}
#if 0
		printf("\n");
#endif
	}

	return (ret);
}
