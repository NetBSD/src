#define PCMCIADEBUG

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
#define DPRINTF(arg) if (pcmcia_debug) printf arg
#else
#define DPRINTF(arg)
#endif

#ifdef	__BROKEN_INDIRECT_CONFIG
int pcmcia_match __P((struct device *, void *, void *));
int pcmcia_submatch __P((struct device *, void *, void *));
#else
int pcmcia_match __P((struct device *, struct cfdata *, void *));
int pcmcia_submatch __P((struct device *, struct cfdata *, void *));
#endif
void pcmcia_attach __P((struct device *, struct device *, void *));
int pcmcia_print __P((void *, const char *));

int pcmcia_card_intr __P((void *));

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
    return(bus_space_read_1((pf)->ccrt, (pf)->ccrh, (pf)->ccr_offset+(ccr)));
}

void
pcmcia_ccr_write(pf, ccr, val)
     struct pcmcia_function *pf;
     int ccr;
     int val;
{
    if (((pf)->ccr_mask) & (1<<(ccr/2))) { 
	bus_space_write_1((pf)->ccrt, (pf)->ccrh,
			  (pf)->ccr_offset+(ccr), (val));
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

    return(1);
}

void
pcmcia_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
    struct pcmciabus_attach_args *paa = (struct pcmciabus_attach_args *) aux;
    struct pcmcia_softc *sc = (struct pcmcia_softc *) self;

    printf("\n");

    sc->pct = paa->pct;
    sc->pch = paa->pch;

    sc->ih = NULL;
}

int
pcmcia_attach_card(dev, iftype)
     struct device *dev;
     int *iftype;
{
    struct pcmcia_softc *sc = (struct pcmcia_softc *) dev;
    struct pcmcia_function *pf;
    struct pcmcia_attach_args paa;
    int attached;

    pcmcia_read_cis(sc);

    /* bail now if the card has no functions, or if there was an error
       in the cis. */

    if (sc->card.error)
	return(1);
    if (!sc->card.pf_head.sqh_first)
	return(1);

    pcmcia_print_cis(sc);

    /* set the iftype to memory if this card has only one function,
       and that is memory. */
    if (sc->card.pf_head.sqh_first &&
	!sc->card.pf_head.sqh_first->pf_list.sqe_next &&
	(sc->card.pf_head.sqh_first->cfe_head.sqh_first->iftype ==
	 PCMCIA_IFTYPE_MEMORY))
	*iftype = PCMCIA_IFTYPE_MEMORY;
    else
	*iftype = PCMCIA_IFTYPE_IO;

    attached = 0;

    for (pf = sc->card.pf_head.sqh_first; pf; pf = pf->pf_list.sqe_next) {
	if (pf->cfe_head.sqh_first == NULL)
	    continue;

	pf->sc = sc;
	pf->cfe = NULL;
	pf->ih_fct = NULL;
	pf->ih_arg = NULL;
    }

    for (pf = sc->card.pf_head.sqh_first; pf; pf = pf->pf_list.sqe_next) {
	if (pf->cfe_head.sqh_first == NULL)
	    continue;

	paa.manufacturer = sc->card.manufacturer;
	paa.product = sc->card.product;
	paa.card = &sc->card;
	paa.pf = pf;

	if (config_found_sm(&sc->dev, &paa, pcmcia_print, pcmcia_submatch)) {
	    attached++;

	    DPRINTF(("%s: function %d CCR at %d offset %lx: %x %x %x %x, %x %x %x %x, %x\n",
		     sc->dev.dv_xname, pf->number,
		     pf->ccr_window, pf->ccr_offset,
		     pcmcia_ccr_read(pf, 0x00),
		     pcmcia_ccr_read(pf, 0x02), pcmcia_ccr_read(pf, 0x04),
		     pcmcia_ccr_read(pf, 0x06), pcmcia_ccr_read(pf, 0x0A),
		     pcmcia_ccr_read(pf, 0x0C), pcmcia_ccr_read(pf, 0x0E),
		     pcmcia_ccr_read(pf, 0x10), pcmcia_ccr_read(pf, 0x12)));
	}
    }

    return(attached?0:1);
}

void
pcmcia_detach_card(dev)
     struct device *dev;
{
/*    struct pcmcia_softc *sc = (struct pcmcia_softc *) dev; */
    /* don't do anything yet */
}

int
#ifdef __BROKEN_INDIRECT_CONFIG
pcmcia_submatch(parent, match, aux)
#else
pcmcia_submatch(parent, cf, aux)
#endif
     struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
     void *match;
#else
     struct cfdata *cf;
#endif
     void *aux;
{
#ifdef __BROKEN_INDIRECT_CONFIG
    struct cfdata *cf = match;
#endif

    struct pcmcia_attach_args *paa = (struct pcmcia_attach_args *) aux;

    if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != paa->pf->number)
	return(0);

    return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
pcmcia_print(arg, pnp)
     void *arg;
     const char *pnp;
{
    struct pcmcia_attach_args *paa = (struct pcmcia_attach_args *) arg;

    if (pnp)
	printf("manufacturer code %x, product %x at %s",
	       paa->manufacturer, paa->product, pnp);

    printf(" function %d", paa->pf->number);

    return(UNCONF);
}

int
pcmcia_enable_function(pf, pcmcia, cfe)
     struct pcmcia_function *pf;
     struct device *pcmcia;
     struct pcmcia_config_entry *cfe;
{
    struct pcmcia_function *tmp;
    int reg;

    pf->cfe = cfe;

    /* it's possible for different functions' CCRs to be in the same
       underlying page.  Check for that. */

    for (tmp = pf->sc->card.pf_head.sqh_first;
	 tmp;
	 tmp = tmp->pf_list.sqe_next) {
	if (tmp->cfe &&
	    (pf->ccr_base >= (tmp->ccr_base - tmp->ccr_offset)) &&
	    ((pf->ccr_base+PCMCIA_CCR_SIZE) <=
	     (tmp->ccr_base - tmp->ccr_offset + tmp->ccr_realsize))) {
	    pf->ccrt = tmp->ccrt;
	    pf->ccrh = tmp->ccrh;
	    pf->ccr_realsize = tmp->ccr_realsize;

	    /* pf->ccr_offset =
	       (tmp->ccr_offset - tmp->ccr_base) + pf->ccr_base; */
	    pf->ccr_offset = (tmp->ccr_offset + pf->ccr_base) - tmp->ccr_base;
	    pf->ccr_window = tmp->ccr_window;
	    /* XXX when shutting down one function, it will be necessary
	       to make sure this window is no longer in use before
	       actually unmapping and freeing it */
	    break;
	}
    }

    if (tmp == NULL) {
	if (pcmcia_mem_alloc(pf, PCMCIA_CCR_SIZE, &pf->ccrt, &pf->ccrh, 
			     &pf->ccr_mhandle, &pf->ccr_realsize))
	    return(1);

	if (pcmcia_mem_map(pf, PCMCIA_MEM_ATTR, PCMCIA_CCR_SIZE,
			   pf->ccrt, pf->ccrh, pf->ccr_base,
			   &pf->ccr_offset, &pf->ccr_window))
	    return(1);
    }

    DPRINTF(("%s: function %d CCR at %d offset %lx: %x %x %x %x, %x %x %x %x, %x\n",
	     pf->sc->dev.dv_xname, pf->number,
	     pf->ccr_window, pf->ccr_offset,
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

#if 0
    reg = PCMCIA_CCR_STATUS_SIGCHG;
#else
    reg = 0;
#endif
    if ((pf->cfe->flags & PCMCIA_CFE_IO16) == 0)
	reg |= PCMCIA_CCR_STATUS_IOIS8;
    if (pf->cfe->flags & PCMCIA_CFE_AUDIO)
	reg |= PCMCIA_CCR_STATUS_AUDIO;
    pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS, reg);

    pcmcia_ccr_write(pf, PCMCIA_CCR_SOCKETCOPY, 0);

    return(0);
}

int
pcmcia_io_map(pf, width, size, iot, ioh, windowp)
     struct pcmcia_function *pf;
     int width;
     bus_size_t size;
     bus_space_tag_t iot;
     bus_space_handle_t ioh;
     int *windowp;
{
    int reg;

    if (pcmcia_chip_io_map(pf->sc->pct, pf->sc->pch,
			   width, size, iot, ioh, windowp))
	return(1);

    /* XXX in the multifunction multi-iospace-per-function case, this
       needs to cooperate with io_alloc to make sure that the spaces
       don't overlap, and that the ccr's are set correctly */

    if (pcmcia_mfc(pf->sc)) {
	pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE0, ioh & 0xff);
	pcmcia_ccr_write(pf, PCMCIA_CCR_IOBASE1, (ioh >> 8) & 0xff);
	pcmcia_ccr_write(pf, PCMCIA_CCR_IOSIZE, size - 1);

	reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
	reg |= PCMCIA_CCR_OPTION_ADDR_DECODE;
	pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);
    }

    return(0);
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

	/* mask all the ipl's which are already used by this card,
	   and find the highest ipl number (lowest priority) */

	ihcnt = 0;
	s = 0; /* this is only here to keep the compipler happy */
	hiipl = 0; /* this is only here to keep the compipler happy */

	for (pf2 = pf->sc->card.pf_head.sqh_first; pf2;
	     pf2 = pf2->pf_list.sqe_next) {
	    if (pf2->ih_fct) {
		if (!ihcnt) {
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

	/* establish the real interrupt, changing the ipl if necessary */

	if (ihcnt == 0) {
#ifdef DIAGNOSTIC
	    if (pf->sc->ih)
		panic("card has intr handler, but no function does");
#endif

	    pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct, pf->sc->pch,
						    ipl, pcmcia_card_intr,
						    pf->sc);
	} else if (ipl > hiipl) {
#ifdef DIAGNOSTIC
	    if (! pf->sc->ih)
		panic("functions have ih, but the card does not");
#endif

	    pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
					  pf->sc->ih);
	    pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct, pf->sc->pch,
						    ipl, pcmcia_card_intr,
						    pf->sc);
	}

	if (ihcnt)
	    splx(s);

	ret = pf->sc->ih;

	if (ret) {
	    reg = pcmcia_ccr_read(pf, PCMCIA_CCR_OPTION);
	    reg |= PCMCIA_CCR_OPTION_IREQ_ENABLE;
	    pcmcia_ccr_write(pf, PCMCIA_CCR_OPTION, reg);

	    reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
	    reg |= PCMCIA_CCR_STATUS_INTRACK;
	    pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS, reg);
	}
    } else {
	ret = pcmcia_chip_intr_establish(pf->sc->pct, pf->sc->pch,
					 ipl, ih_fct, ih_arg);
    }

    return(ret);
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

	/* mask all the ipl's which are already used by this card,
	   and find the highest ipl number (lowest priority).  Skip
	   the current function. */

	ihcnt = 0;
	s = 0; /* this is only here to keep the compipler happy */
	hiipl = 0; /* this is only here to keep the compipler happy */

	for (pf2 = pf->sc->card.pf_head.sqh_first; pf2;
	     pf2 = pf2->pf_list.sqe_next) {
	    if (pf2 == pf)
		continue;

	    if (pf2->ih_fct) {
		if (!ihcnt) {
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

	/* if the ih being removed is lower priority than the lowest
	   priority remaining interrupt, up the priority. */

#ifdef DIAGNOSTIC
	if (ihcnt == 0) {
	    panic("can't remove a handler from a card which has none");
	} else
#endif
	if (ihcnt == 1) {
#ifdef DIAGNOSTIC
	    if (!pf->sc->ih)
		panic("disestablishing last function, but card has no ih");
#endif
	    pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
					  pf->sc->ih);
	    pf->sc->ih = NULL;
	} else if (pf->ih_ipl > hiipl) {
#ifdef DIAGNOSTIC
	    if (!pf->sc->ih)
		panic("changing ih ipl, but card has no ih");
#endif
	    pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch,
					  pf->sc->ih);
	    pf->sc->ih = pcmcia_chip_intr_establish(pf->sc->pct, pf->sc->pch,
						    hiipl, pcmcia_card_intr,
						    pf->sc);
	}
	    
	if (ihcnt)
	    splx(s);
    } else {
	pcmcia_chip_intr_disestablish(pf->sc->pct, pf->sc->pch, ih);
    }
}

int pcmcia_card_intr(arg)
    void *arg;
{
    struct pcmcia_softc *sc = (struct pcmcia_softc *) arg;
    struct pcmcia_function *pf;
    int reg, ret, ret2;

    ret = 0;

    for (pf = sc->card.pf_head.sqh_first; pf; pf = pf->pf_list.sqe_next) {
#if 0
	printf("%s: intr fct=%d physaddr=%lx cor=%02x csr=%02x pin=%02x",
	       sc->dev.dv_xname, pf->number,
	       pmap_extract(pmap_kernel(), (vm_offset_t) pf->ccrh) + pf->ccr_offset,
	       bus_space_read_1(pf->ccrt, pf->ccrh, 
				pf->ccr_offset+PCMCIA_CCR_OPTION),
	       bus_space_read_1(pf->ccrt, pf->ccrh, 
				pf->ccr_offset+PCMCIA_CCR_STATUS),
	       bus_space_read_1(pf->ccrt, pf->ccrh, 
				pf->ccr_offset+PCMCIA_CCR_PIN));
#endif
	if (pf->ih_fct &&
	    (pf->ccr_mask & (1<<(PCMCIA_CCR_STATUS/2)))) {
	    reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
	    if (reg & PCMCIA_CCR_STATUS_INTR) {
		ret2 = (*pf->ih_fct)(pf->ih_arg);
		if (ret2 && !ret)
		    ret = ret2;
		reg = pcmcia_ccr_read(pf, PCMCIA_CCR_STATUS);
#if 0
		printf("; csr %02x->%02x", reg, reg & ~PCMCIA_CCR_STATUS_INTR);
#endif
		pcmcia_ccr_write(pf, PCMCIA_CCR_STATUS,
				 reg & ~PCMCIA_CCR_STATUS_INTR);
	    }
	}
#if 0
	printf("\n");
#endif
    }

    return(ret);
}

