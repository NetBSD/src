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
    reg |= PCIC_INTR_ENABLE;
    reg |= irq;
    pcic_write(h, PCIC_INTR, reg);

    printf("%s: card irq %d\n", h->pcmcia->dv_xname, irq);

    return(ih);
}

void pcic_isa_chip_intr_disestablish(pch, ih)
     pcmcia_chipset_handle_t pch;
     void *ih;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;

    reg = pcic_read(h, PCIC_INTR);
    reg &= ~(PCIC_INTR_IRQ_MASK|PCIC_INTR_ENABLE);
    pcic_write(h, PCIC_INTR, reg);

    isa_intr_disestablish(h->sc->intr_est, ih);
}
