#define PCICISADEBUG

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <dev/ic/i82365reg.h>
#include <dev/ic/i82365var.h>

#ifdef PCICISADEBUG
int	pcicisa_debug = 0 /* XXX */;
#define DPRINTF(arg) if (pcicisa_debug) printf arg;
#else
#define DPRINTF(arg)
#endif

int pcic_isa_probe __P((struct device *, void *, void *));
void pcic_isa_attach __P((struct device *, struct device *, void *));

void *pcic_isa_chip_intr_establish __P((pcmcia_chipset_handle_t, 
					struct pcmcia_function *, int,
					int (*)(void *), void *));
void pcic_isa_chip_intr_disestablish __P((pcmcia_chipset_handle_t, void *));

struct cfattach pcic_isa_ca = {
	sizeof(struct pcic_softc), pcic_isa_probe, pcic_isa_attach
};

static struct pcmcia_chip_functions pcic_isa_functions = {
    pcic_chip_mem_alloc,
    pcic_chip_mem_free,
    pcic_chip_mem_map,
    pcic_chip_mem_unmap,

    pcic_chip_io_alloc,
    pcic_chip_io_free,
    pcic_chip_io_map,
    pcic_chip_io_unmap,

    pcic_isa_chip_intr_establish,
    pcic_isa_chip_intr_disestablish,

    pcic_chip_socket_enable,
    pcic_chip_socket_disable,
};

int
pcic_isa_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
    struct isa_attach_args *ia = aux;
    bus_space_tag_t iot = ia->ia_iot;
    bus_space_handle_t ioh, memh;
    int val, found;

    if (bus_space_map(iot, ia->ia_iobase, PCIC_IOSIZE, 0, &ioh))
	return (0);

    if (ia->ia_msize == -1)
	ia->ia_msize = PCIC_MEMSIZE;

    if (bus_space_map(ia->ia_memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
	return (0);

    found = 0;

    /* this could be done with a loop, but it would violate the
       abstraction */

    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C0SA+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    if (pcic_ident_ok(val))
	found++;


    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C0SB+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    if (pcic_ident_ok(val))
	found++;


    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C1SA+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    if (pcic_ident_ok(val))
	found++;


    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C1SB+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    if (pcic_ident_ok(val))
	found++;


    bus_space_unmap(iot, ioh, PCIC_IOSIZE);
    bus_space_unmap(ia->ia_memt, memh, ia->ia_msize);

    if (!found)
	return(0);

    ia->ia_iosize = PCIC_IOSIZE;

    return(1);
}

#ifndef PCIC_ISA_ALLOC_IOBASE
#define PCIC_ISA_ALLOC_IOBASE 0
#endif

#ifndef PCIC_ISA_ALLOC_IOSIZE
#define PCIC_ISA_ALLOC_IOSIZE 0
#endif

int pcic_isa_alloc_iobase = PCIC_ISA_ALLOC_IOBASE;
int pcic_isa_alloc_iosize = PCIC_ISA_ALLOC_IOSIZE;

void
pcic_isa_attach(parent, self, aux)
     struct device *parent, *self;
     void *aux;
{
    struct pcic_softc *sc = (void *)self;
    struct isa_attach_args *ia = aux;
    isa_chipset_tag_t ic = ia->ia_ic;
    bus_space_tag_t iot = ia->ia_iot;
    bus_space_tag_t memt = ia->ia_memt;
    bus_space_handle_t ioh;
    bus_space_handle_t memh;
    bus_space_handle_t ioh_high;
    int i, iobuswidth, tmp1, tmp2;

    /* Map i/o space. */
    if (bus_space_map(iot, ia->ia_iobase, ia->ia_iosize, 0, &ioh))
	panic("pcic_isa_attach: can't map i/o space");

    /* Map mem space. */
    if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
	panic("pcic_isa_attach: can't map i/o space");

    sc->membase = ia->ia_maddr;
    sc->subregionmask = (1<<(ia->ia_msize/PCIC_MEM_PAGESIZE))-1;

    sc->intr_est = ic;
    sc->pct = (pcmcia_chipset_tag_t) &pcic_isa_functions;

    sc->iot = iot;
    sc->ioh = ioh;
    sc->memt = memt;
    sc->memh = memh;

    /* allocate an irq.  it will be used by both controllers.  I could
       use two different interrupts, but interrupts are relatively
       scarce, shareable, and for PCIC controllers, very infrequent. */

    if ((sc->irq = ia->ia_irq) == IRQUNK) {
	/* XXX CHECK RETURN VALUE */
	(void) isa_intr_alloc(ic,
			      PCIC_CSC_INTR_IRQ_VALIDMASK,
			      IST_EDGE, &sc->irq);
	printf(": using irq %d", sc->irq);
    }

    printf("\n");

    pcic_attach(sc);

    /* figure out how wide the isa bus is.  Do this by checking if
       the pcic controller is mirrored 0x400 above where we expect it
       to be. */

    iobuswidth = 12;

    /* Map i/o space. */
    if (bus_space_map(iot, ia->ia_iobase+0x400, ia->ia_iosize, 0, &ioh_high))
	panic("pcic_isa_attach: can't map high i/o space");

    for (i=0; i<PCIC_NSLOTS; i++) {
	if (sc->handle[i].flags & PCIC_FLAG_SOCKETP) {
	    /* read the ident flags from the normal space
	       and from the mirror, and compare them */

	    bus_space_write_1(iot, ioh, PCIC_REG_INDEX,
			      sc->handle[i].sock+PCIC_IDENT);
	    tmp1 = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

	    bus_space_write_1(iot, ioh_high, PCIC_REG_INDEX,
			      sc->handle[i].sock+PCIC_IDENT);
	    tmp2 = bus_space_read_1(iot, ioh_high, PCIC_REG_DATA);

	    if (tmp1 == tmp2)
		iobuswidth = 10;
	}
    }

    bus_space_free(iot, ioh_high, ia->ia_iosize);

/* XXX mycroft recommends I/O space range 0x400-0xfff .  I should put
   this in a header somewhere */

/* XXX some hardware doesn't seem to grok addresses in 0x400 range--
   apparently missing a bit or more of address lines.
   (e.g. CIRRUS_PD672X with Linksys EthernetCard ne2000 clone in TI
   TravelMate 5000--not clear which is at fault)

   Add a kludge to detect 10 bit wide buses and deal with them, and
   also a config file option to override the probe. */

    if (iobuswidth == 10) {
	sc->iobase = 0x300;
	sc->iosize = 0x0ff;
    } else {
	sc->iobase = 0x400;
	sc->iosize = 0xbff;
    }

    DPRINTF(("%s: bus_space_alloc range 0x%04lx-0x%04lx (probed)\n",
	     sc->dev.dv_xname, (long) sc->iobase,
	     (long) sc->iobase+sc->iosize));

    if (pcic_isa_alloc_iobase && pcic_isa_alloc_iosize) {
	sc->iobase = pcic_isa_alloc_iobase;
	sc->iosize = pcic_isa_alloc_iosize;

	DPRINTF(("%s: bus_space_alloc range 0x%04lx-0x%04lx (config override)\n",
		 sc->dev.dv_xname, (long) sc->iobase,
		 (long) sc->iobase+sc->iosize));
    }

    sc->ih = isa_intr_establish(ic, sc->irq, IST_EDGE, IPL_TTY, pcic_intr, sc);

    pcic_attach_sockets(sc);
}

/* allow patching or kernel option file override of available IRQs.
   Useful if order of probing would screw up other devices, or if PCIC
   hardware/cards have trouble with certain interrupt lines. */

#ifndef PCIC_INTR_ALLOC_MASK
#define	PCIC_INTR_ALLOC_MASK	0xffff
#endif

int	pcic_intr_alloc_mask = PCIC_INTR_ALLOC_MASK;

void *
pcic_isa_chip_intr_establish(pch, pf, ipl, fct, arg)
     pcmcia_chipset_handle_t pch;
     struct pcmcia_function *pf;
     int ipl;
     int (*fct)(void *);
     void *arg;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int irq, ist;
    void *ih;
    int reg;

    if (pf->cfe->flags & PCMCIA_CFE_IRQLEVEL)
	ist = IST_LEVEL;
    else if (pf->cfe->flags & PCMCIA_CFE_IRQPULSE)
	ist = IST_PULSE;
    else
	ist = IST_LEVEL;

    if (isa_intr_alloc(h->sc->intr_est,
    		       PCIC_INTR_IRQ_VALIDMASK & pcic_intr_alloc_mask,
    		       ist, &irq))
	return(NULL);
    if (!(ih = isa_intr_establish(h->sc->intr_est, irq, ist, ipl, fct, arg)))
	return(NULL);

    reg = pcic_read(h, PCIC_INTR);
    reg &= ~PCIC_INTR_IRQ_MASK;
    reg |= irq;
    pcic_write(h, PCIC_INTR, reg);

    h->ih_irq = irq;

    printf("%s: card irq %d\n", h->pcmcia->dv_xname, irq);

    return(ih);
}

void pcic_isa_chip_intr_disestablish(pch, ih)
     pcmcia_chipset_handle_t pch;
     void *ih;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;

    h->ih_irq = 0;

    reg = pcic_read(h, PCIC_INTR);
    reg &= ~(PCIC_INTR_IRQ_MASK|PCIC_INTR_ENABLE);
    pcic_write(h, PCIC_INTR, reg);

    isa_intr_disestablish(h->sc->intr_est, ih);
}
