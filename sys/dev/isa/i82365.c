#define PCICDEBUG

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
#include <dev/pcmcia/pcmciachip.h>

#include <dev/ic/i82365reg.h>

#ifdef PCICDEBUG
int	pcic_debug = 0;
#define DPRINTF(arg) if (pcic_debug) printf arg;
#else
#define DPRINTF(arg)
#endif

/* This is sort of arbitrary.  It merely needs to be "enough".
   It can be overridden in the conf file, anyway. */

#define PCIC_MEM_PAGES	4
#define PCIC_MEMSIZE	PCIC_MEM_PAGES*PCIC_MEM_PAGESIZE

#define PCIC_NSLOTS	4

#define PCIC_FLAG_SOCKETP	0x0001
#define PCIC_FLAG_CARDP		0x0002

#define PCIC_VENDOR_UNKNOWN		0
#define PCIC_VENDOR_I82365SLR0		1
#define PCIC_VENDOR_I82365SLR1		2
#define PCIC_VENDOR_CIRRUS_PD6710	3
#define PCIC_VENDOR_CIRRUS_PD672X	4

struct pcic_handle {
    struct pcic_softc *sc;
    int vendor;
    int sock;
    int flags;
    int memalloc;
    int ioalloc;
    struct device *pcmcia;
};

struct pcic_softc {
    struct device dev;

    isa_chipset_tag_t ic;

    bus_space_tag_t memt;
    bus_space_tag_t memh;
    bus_space_tag_t iot;
    bus_space_tag_t ioh;

    /* this needs to be large enough to hold PCIC_MEM_PAGES bits */
    int subregionmask;

    int irq;
    void *ih;

    struct pcic_handle handle[PCIC_NSLOTS];
};

#define C0SA PCIC_CHIP0_BASE+PCIC_SOCKETA_INDEX
#define C0SB PCIC_CHIP0_BASE+PCIC_SOCKETB_INDEX
#define C1SA PCIC_CHIP1_BASE+PCIC_SOCKETA_INDEX
#define C1SB PCIC_CHIP1_BASE+PCIC_SOCKETB_INDEX

/* Individual drivers will allocate their own memory and io regions.
   Memory regions must be a multiple of 4k, aligned on a 4k boundary. */

#define PCIC_MEM_ALIGN	PCIC_MEM_PAGESIZE

int pcic_probe __P((struct device *, void *, void *));
void pcic_attach __P((struct device *, struct device *, void *));

int pcic_ident_ok __P((int));
int pcic_vendor __P((struct pcic_handle *));
char *pcic_vendor_to_string __P((int));
static inline int pcic_read __P((struct pcic_handle *, int));
static inline void pcic_write __P((struct pcic_handle *, int, int));
static inline void pcic_wait_ready __P((struct pcic_handle *));
void pcic_attach_socket __P((struct pcic_handle *));
void pcic_init_socket __P((struct pcic_handle *));

#ifdef __BROKEN_INDIRECT_CONFIG
int pcic_submatch __P((struct device *, void *, void *));
#else
int pcic_submatch __P((struct device *, struct cfdata *, void *));
#endif
int pcic_print __P((void *arg, const char *pnp));
int pcic_intr __P((void *arg));
int pcic_intr_socket __P((struct pcic_handle *));

int pcic_chip_mem_alloc __P((pcmcia_chipset_handle_t, bus_size_t, 
			     bus_space_tag_t *, bus_space_handle_t *,
			     pcmcia_mem_handle_t *, bus_size_t *));
void pcic_chip_mem_free __P((pcmcia_chipset_handle_t, bus_size_t,
			     bus_space_tag_t, bus_space_handle_t,
			     pcmcia_mem_handle_t));
int pcic_chip_mem_map __P((pcmcia_chipset_handle_t, int,
			   bus_size_t, bus_space_tag_t, bus_space_handle_t,
			   u_long, u_long *, int *));
void pcic_chip_mem_unmap __P((pcmcia_chipset_handle_t, int));

int pcic_chip_io_alloc __P((pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
			    bus_space_tag_t *, bus_space_handle_t *));
void pcic_chip_io_free __P((pcmcia_chipset_handle_t, bus_size_t,
			    bus_space_tag_t, bus_space_handle_t));
int pcic_chip_io_map __P((pcmcia_chipset_handle_t, int, bus_size_t,
			  bus_space_tag_t, bus_space_handle_t, int *));
void pcic_chip_io_unmap __P((pcmcia_chipset_handle_t, int));

void *pcic_chip_intr_establish __P((pcmcia_chipset_handle_t, u_int16_t, int,
				    int (*)(void *), void *));
void pcic_chip_intr_disestablish __P((pcmcia_chipset_handle_t, void *));


void pcic_attach_card(struct pcic_handle *);
void pcic_detach_card(struct pcic_handle *);

static struct pcmcia_chip_functions pcic_functions = {
    pcic_chip_mem_alloc,
    pcic_chip_mem_free,
    pcic_chip_mem_map,
    pcic_chip_mem_unmap,

    pcic_chip_io_alloc,
    pcic_chip_io_free,
    pcic_chip_io_map,
    pcic_chip_io_unmap,

    pcic_chip_intr_establish,
    pcic_chip_intr_disestablish,
};

struct cfdriver pcic_cd = {
	NULL, "pcic", DV_DULL
};

struct cfattach pcic_ca = {
	sizeof(struct pcic_softc), pcic_probe, pcic_attach
};

static inline int
pcic_read(h, idx)
     struct pcic_handle *h;
     int idx;
{
    if (idx != -1)
	bus_space_write_1(h->sc->iot, h->sc->ioh, PCIC_REG_INDEX, h->sock+idx);
    return(bus_space_read_1(h->sc->iot, h->sc->ioh, PCIC_REG_DATA));
}

static inline void
pcic_write(h, idx, data)
     struct pcic_handle *h;
     int idx;
     int data;
{
    if (idx != -1)
	bus_space_write_1(h->sc->iot, h->sc->ioh, PCIC_REG_INDEX, h->sock+idx);
    bus_space_write_1(h->sc->iot, h->sc->ioh, PCIC_REG_DATA, (data));
}

static inline void
pcic_wait_ready(h)
     struct pcic_handle *h;
{
    int i;

    for (i=0; i<10000; i++) {
	if (pcic_read(h, PCIC_IF_STATUS) & PCIC_IF_STATUS_READY)
	    return;
	delay(500);
    }

    DPRINTF(("pcic_wait_ready ready never happened\n"));
}

int
pcic_ident_ok(ident)
     int ident;
{
    /* this is very empirical and heuristic */

    if ((ident == 0) || (ident == 0xff) || (ident & PCIC_IDENT_ZERO))
	return(0);

    if ((ident & PCIC_IDENT_IFTYPE_MASK) != PCIC_IDENT_IFTYPE_MEM_AND_IO) {
#ifdef DIAGNOSTIC
	printf("pcic: does not support memory and I/O cards, ignored (ident=%0x)\n",
	       ident);
#endif
	return(0);
    }

    return(1);
}

int
pcic_vendor(h)
     struct pcic_handle *h;
{
    int reg;

    /* I can't claim to understand this; I'm just doing what the
       linux driver does */

    pcic_write(h, PCIC_CIRRUS_CHIP_INFO, 0);
    reg = pcic_read(h, -1);

    if ((reg & PCIC_CIRRUS_CHIP_INFO_CHIP_ID) ==
	PCIC_CIRRUS_CHIP_INFO_CHIP_ID) {
	reg = pcic_read(h, -1);
	if ((reg & PCIC_CIRRUS_CHIP_INFO_CHIP_ID) == 0) {
	    if (reg & PCIC_CIRRUS_CHIP_INFO_SLOTS)
		return(PCIC_VENDOR_CIRRUS_PD672X);
	    else
		return(PCIC_VENDOR_CIRRUS_PD6710);
	}
    }

    reg = pcic_read(h, PCIC_IDENT);

    if ((reg & PCIC_IDENT_REV_MASK) == PCIC_IDENT_REV_I82365SLR0)
	return(PCIC_VENDOR_I82365SLR0);
    else
	return(PCIC_VENDOR_I82365SLR1);

    return(PCIC_VENDOR_UNKNOWN);
}

char *
pcic_vendor_to_string(vendor)
     int vendor;
{
    switch (vendor) {
    case PCIC_VENDOR_I82365SLR0:
	return("Intel 82365SL Revision 0");
    case PCIC_VENDOR_I82365SLR1:
	return("Intel 82365SL Revision 1");
    case PCIC_VENDOR_CIRRUS_PD6710:
	return("Cirrus PD6710");
    case PCIC_VENDOR_CIRRUS_PD672X:
	return("Cirrus PD672X");
    }

    return("Unknown controller");
}

int
pcic_probe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
    struct isa_attach_args *ia = aux;
    bus_space_tag_t iot = ia->ia_iot;
    bus_space_handle_t ioh, memh;
    int val, found;

    DPRINTF(("pcic_probe %x\n", ia->ia_iobase));

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

    DPRINTF(("c0sa ident = %02x, ", val));

    if (pcic_ident_ok(val))
	found++;


    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C0SB+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    DPRINTF(("c0sb ident = %02x, ", val));

    if (pcic_ident_ok(val))
	found++;


    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C1SA+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    DPRINTF(("c1sa ident = %02x, ", val));

    if (pcic_ident_ok(val))
	found++;


    bus_space_write_1(iot, ioh, PCIC_REG_INDEX, C1SB+PCIC_IDENT);

    val = bus_space_read_1(iot, ioh, PCIC_REG_DATA);

    DPRINTF(("c1sb ident = %02x\n", val));

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
pcic_attach(parent, self, aux)
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
    int vendor, count, irq, i;

    /* Map i/o space. */
    if (bus_space_map(iot, ia->ia_iobase, ia->ia_iosize, 0, &ioh))
	panic("pcic_attach: can't map i/o space");

    /* Map mem space. */
    if (bus_space_map(memt, ia->ia_maddr, ia->ia_msize, 0, &memh))
	panic("pcic_attach: can't map i/o space");

    sc->subregionmask = (1<<(ia->ia_msize/PCIC_MEM_PAGESIZE))-1;

    sc->ic = ic;

    sc->iot = iot;
    sc->ioh = ioh;
    sc->memt = memt;
    sc->memh = memh;

    /* now check for each controller/socket */

    /* this could be done with a loop, but it would violate the
       abstraction */

    count = 0;

    sc->handle[0].sc = sc;
    sc->handle[0].sock = C0SA;
    if (pcic_ident_ok(pcic_read(&sc->handle[0], PCIC_IDENT))) {
	sc->handle[0].flags = PCIC_FLAG_SOCKETP;
	count++;
    } else {
	sc->handle[0].flags = 0;
    }

    sc->handle[1].sc = sc;
    sc->handle[1].sock = C0SB;
    if (pcic_ident_ok(pcic_read(&sc->handle[1], PCIC_IDENT))) {
	sc->handle[1].flags = PCIC_FLAG_SOCKETP;
	count++;
    } else {
	sc->handle[1].flags = 0;
    }

    sc->handle[2].sc = sc;
    sc->handle[2].sock = C1SA;
    if (pcic_ident_ok(pcic_read(&sc->handle[2], PCIC_IDENT))) {
	sc->handle[2].flags = PCIC_FLAG_SOCKETP;
	count++;
    } else {
	sc->handle[2].flags = 0;
    }

    sc->handle[3].sc = sc;
    sc->handle[3].sock = C1SB;
    if (pcic_ident_ok(pcic_read(&sc->handle[3], PCIC_IDENT))) {
	sc->handle[3].flags = PCIC_FLAG_SOCKETP;
	count++;
    } else {
	sc->handle[3].flags = 0;
    }

    if (count == 0)
	panic("pcic_attach: attach found no sockets");

    /* allocate an irq.  it will be used by both controllers.  I could
       use two different interrupts, but interrupts are relatively
       scarce, shareable, and for PCIC controllers, very infrequent. */

    if (ia->ia_irq == IRQUNK) {
	isa_intr_alloc(ic, PCIC_CSC_INTR_IRQ_VALIDMASK, IST_EDGE, &irq);
	sc->irq = irq;

	printf(": using irq %d", irq);
    }

    printf("\n");

    /* establish the interrupt */

    /* XXX block interrupts? */

    for (i=0; i<PCIC_NSLOTS; i++) {
	pcic_write(&sc->handle[i], PCIC_CSC_INTR, 0);
	pcic_read(&sc->handle[i], PCIC_CSC);
    }

    sc->ih = isa_intr_establish(ic, irq, IST_EDGE, IPL_TTY, pcic_intr, sc);

    if ((sc->handle[0].flags & PCIC_FLAG_SOCKETP) ||
	(sc->handle[1].flags & PCIC_FLAG_SOCKETP)) {
	vendor = pcic_vendor(&sc->handle[0]);

	printf("%s: controller 0 (%s) has ", sc->dev.dv_xname,
	       pcic_vendor_to_string(vendor));
	
	if ((sc->handle[0].flags & PCIC_FLAG_SOCKETP) &&
	    (sc->handle[1].flags & PCIC_FLAG_SOCKETP))
	    printf("sockets A and B\n");
	else if (sc->handle[0].flags & PCIC_FLAG_SOCKETP)
	    printf("socket A only\n");
	else
	    printf("socket B only\n");

#if 0
	pcic_write(&sc->handle[0], PCIC_GLOBAL_CTL,
		   PCIC_GLOBAL_CTL_EXPLICIT_CSC_ACK);
#endif

	if (sc->handle[0].flags & PCIC_FLAG_SOCKETP) {
	    sc->handle[0].vendor = vendor;
	    pcic_attach_socket(&sc->handle[0]);
	}
	if (sc->handle[1].flags & PCIC_FLAG_SOCKETP) {
	    sc->handle[1].vendor = vendor;
	    pcic_attach_socket(&sc->handle[1]);
	}
    }

    if ((sc->handle[2].flags & PCIC_FLAG_SOCKETP) ||
	(sc->handle[3].flags & PCIC_FLAG_SOCKETP)) {
	vendor = pcic_vendor(&sc->handle[2]);

	printf("%s: controller 1 (%s) has ", sc->dev.dv_xname,
	       pcic_vendor_to_string(vendor));
	
	if ((sc->handle[2].flags & PCIC_FLAG_SOCKETP) &&
	    (sc->handle[3].flags & PCIC_FLAG_SOCKETP))
	    printf("sockets A and B\n");
	else if (sc->handle[2].flags & PCIC_FLAG_SOCKETP)
	    printf("socket A only\n");
	else
	    printf("socket B only\n");

#if 0
	pcic_write(&sc->handle[2], PCIC_GLOBAL_CTL,
		   PCIC_GLOBAL_CTL_EXPLICIT_CSC_ACK);
#endif

	if (sc->handle[2].flags & PCIC_FLAG_SOCKETP) {
	    pcic_attach_socket(&sc->handle[2]);
	    sc->handle[2].vendor = vendor;
	}
	if (sc->handle[3].flags & PCIC_FLAG_SOCKETP) {
	    pcic_attach_socket(&sc->handle[3]);
	    sc->handle[3].vendor = vendor;
	}
    }
}

void
pcic_attach_socket(h)
     struct pcic_handle *h;
{
   struct pcmciabus_attach_args paa;

   /* initialize the rest of the handle */

   h->memalloc = 0;
   h->ioalloc = 0;

   /* now, config one pcmcia device per socket */

   paa.pct = (pcmcia_chipset_tag_t) &pcic_functions;
   paa.pch = (pcmcia_chipset_handle_t) h;

   h->pcmcia = config_found_sm(&h->sc->dev, &paa, pcic_print, pcic_submatch);

   /* if there's actually a pcmcia device attached, initialize the slot */

   if (h->pcmcia)
       pcic_init_socket(h);
}

void
pcic_init_socket(h)
     struct pcic_handle *h;
{
    int reg;

    /* set up the card to interrupt on card detect */

    pcic_write(h, PCIC_CSC_INTR,
	       (h->sc->irq<<PCIC_CSC_INTR_IRQ_SHIFT)|
	       PCIC_CSC_INTR_CD_ENABLE);
    pcic_write(h, PCIC_INTR, 0);
    pcic_read(h, PCIC_CSC);

    /* unsleep the cirrus controller */

    if ((h->vendor == PCIC_VENDOR_CIRRUS_PD6710) ||
	(h->vendor == PCIC_VENDOR_CIRRUS_PD672X)) {
	reg = pcic_read(h, PCIC_CIRRUS_MISC_CTL_2);
	if (reg & PCIC_CIRRUS_MISC_CTL_2_SUSPEND) {
	    DPRINTF(("%s: socket %02x was suspended\n", h->sc->dev.dv_xname,
		     h->sock));
	    reg &= ~PCIC_CIRRUS_MISC_CTL_2_SUSPEND;
	    pcic_write(h, PCIC_CIRRUS_MISC_CTL_2, reg);
	}
    }

    /* if there's a card there, then attach it. */

    reg = pcic_read(h, PCIC_IF_STATUS);

    if ((reg & PCIC_IF_STATUS_CARDDETECT_MASK) ==
	PCIC_IF_STATUS_CARDDETECT_PRESENT)
	pcic_attach_card(h);
}

int
#ifdef __BROKEN_INDIRECT_CONFIG
pcic_submatch(parent, match, aux)
#else
pcic_submatch(parent, cf, aux)
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

    struct pcmciabus_attach_args *paa = (struct pcmciabus_attach_args *) aux;
    struct pcic_handle *h = (struct pcic_handle *) paa->pch;

    switch (h->sock) {
    case C0SA:
	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != 0)
	    return 0;
	if (cf->cf_loc[1] != -1 && cf->cf_loc[1] != 0)
	    return 0;

	break;
    case C0SB:
	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != 0)
	    return 0;
	if (cf->cf_loc[1] != -1 && cf->cf_loc[1] != 1)
	    return 0;

	break;
    case C1SA:
	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != 1)
	    return 0;
	if (cf->cf_loc[1] != -1 && cf->cf_loc[1] != 0)
	    return 0;

	break;
    case C1SB:
	if (cf->cf_loc[0] != -1 && cf->cf_loc[0] != 1)
	    return 0;
	if (cf->cf_loc[1] != -1 && cf->cf_loc[1] != 1)
	    return 0;

	break;
    default:
	panic("unknown pcic socket");
    }

    return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
pcic_print(arg, pnp)
     void *arg;
     const char *pnp;
{
    struct pcmciabus_attach_args *paa = (struct pcmciabus_attach_args *) arg;
    struct pcic_handle *h = (struct pcic_handle *) paa->pch;

    if (pnp)
	printf("pcmcia at %s", pnp);

    switch (h->sock) {
    case C0SA:
	printf(" controller 0 socket 0");
	break;
    case C0SB:
	printf(" controller 0 socket 1");
	break;
    case C1SA:
	printf(" controller 1 socket 0");
	break;
    case C1SB:
	printf(" controller 1 socket 1");
	break;
    default:
	panic("unknown pcic socket");
    }

    return(UNCONF);
}

int
pcic_intr(arg)
     void *arg;
{
    struct pcic_softc *sc = (struct pcic_softc *) arg;
    int i, ret = 0;

    DPRINTF(("%s: intr\n", sc->dev.dv_xname));

    for (i=0; i<PCIC_NSLOTS; i++)
	if (sc->handle[i].flags & PCIC_FLAG_SOCKETP)
	    ret += pcic_intr_socket(&sc->handle[i]);

    return(ret?1:0);
}

int
pcic_intr_socket(h)
     struct pcic_handle *h;
{
    int cscreg;

    cscreg = pcic_read(h, PCIC_CSC);

    cscreg &= (PCIC_CSC_GPI |
	       PCIC_CSC_CD |
	       PCIC_CSC_READY |
	       PCIC_CSC_BATTWARN |
	       PCIC_CSC_BATTDEAD);

    if (cscreg & PCIC_CSC_GPI) {
	DPRINTF(("%s: %02x GPI\n", h->sc->dev.dv_xname, h->sock));
    }
    if (cscreg & PCIC_CSC_CD) {
	int statreg;

	statreg = pcic_read(h, PCIC_IF_STATUS);

	DPRINTF(("%s: %02x CD %x\n", h->sc->dev.dv_xname, h->sock,
		 statreg));

	if ((statreg & PCIC_IF_STATUS_CARDDETECT_MASK) ==
	    PCIC_IF_STATUS_CARDDETECT_PRESENT) {
	    if (!(h->flags & PCIC_FLAG_CARDP))
		pcic_attach_card(h);
	} else {
	    if (h->flags & PCIC_FLAG_CARDP)
		pcic_detach_card(h);
	}
    }
    if (cscreg & PCIC_CSC_READY) {
	DPRINTF(("%s: %02x READY\n", h->sc->dev.dv_xname, h->sock));
	/* shouldn't happen */
    }
    if (cscreg & PCIC_CSC_BATTWARN) {
	DPRINTF(("%s: %02x BATTWARN\n", h->sc->dev.dv_xname, h->sock));
    }
    if (cscreg & PCIC_CSC_BATTDEAD) {
	DPRINTF(("%s: %02x BATTDEAD\n", h->sc->dev.dv_xname, h->sock));
    }

#if 0
    /* ack the interrupt */

    pcic_write(h, PCIC_CSC, cscreg);
#endif

    return(cscreg?1:0);
}

void
pcic_attach_card(h)
     struct pcic_handle *h;
{
    int iftype;
    int reg;

    if (h->flags & PCIC_FLAG_CARDP)
	panic("pcic_attach_card: already attached");

    /* power down the socket to reset it, clear the card reset pin */

    pcic_write(h, PCIC_PWRCTL, 0);

    /* power up the socket */

    pcic_write(h, PCIC_PWRCTL, PCIC_PWRCTL_PWR_ENABLE);
    delay(10000);
    pcic_write(h, PCIC_PWRCTL, PCIC_PWRCTL_PWR_ENABLE | PCIC_PWRCTL_OE);

    /* clear the reset flag */

    pcic_write(h, PCIC_INTR, PCIC_INTR_RESET);

    /* wait 20ms as per pc card standard (r2.01) section 4.3.6 */

    delay(20000);

    /* wait for the chip to finish initializing */

    pcic_wait_ready(h);

    /* zero out the address windows */

    pcic_write(h, PCIC_ADDRWIN_ENABLE, 0);

#if 1
    pcic_write(h, PCIC_INTR, PCIC_INTR_RESET | PCIC_INTR_CARDTYPE_IO);
#endif

    reg = pcic_read(h, PCIC_INTR);

    DPRINTF(("%s: %02x PCIC_INTR = %02x\n", h->sc->dev.dv_xname,
	     h->sock, reg));

    /* call the MI attach function */

    pcmcia_attach_card(h->pcmcia, &iftype);

    /* set the card type */

    DPRINTF(("%s: %02x cardtype %s\n", h->sc->dev.dv_xname, h->sock,
	     ((iftype == PCMCIA_IFTYPE_IO)?"io":"mem")));

#if 0
    reg = pcic_read(h, PCIC_INTR);
    reg &= PCIC_INTR_CARDTYPE_MASK;
    reg |= ((iftype == PCMCIA_IFTYPE_IO)?
	    PCIC_INTR_CARDTYPE_IO:
	    PCIC_INTR_CARDTYPE_MEM);
    pcic_write(h, PCIC_INTR, reg);
#endif

    h->flags |= PCIC_FLAG_CARDP;
}

void
pcic_detach_card(h)
     struct pcic_handle *h;
{
    if (!(h->flags & PCIC_FLAG_CARDP))
	panic("pcic_attach_card: already attached");

    h->flags &= ~PCIC_FLAG_CARDP;

    /* call the MI attach function */

    pcmcia_detach_card(h->pcmcia);

    /* disable card detect resume and configuration reset */

#if 0
    pcic_write(h, PCIC_CARD_DETECT, 0);
#endif

    /* power down the socket */

    pcic_write(h, PCIC_PWRCTL, 0);

    /* reset the card */

    pcic_write(h, PCIC_INTR, 0);
}

int pcic_chip_mem_alloc(pch, size, memt, memh, mhandle, realsize)
     pcmcia_chipset_handle_t pch;
     bus_size_t size;
     bus_space_tag_t *memt;
     bus_space_handle_t *memh;
     pcmcia_mem_handle_t *mhandle;
     bus_size_t *realsize;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int i, mask;

    /* out of sc->memh, allocate as many pages as necessary */

    size += (PCIC_MEM_ALIGN-1);
    size /= PCIC_MEM_ALIGN;

    /* size is now in pages */

    mask = (1<<size)-1;

    for (i=0; i<(PCIC_MEM_PAGES+1-size); i++) {
	if ((h->sc->subregionmask & (mask<<i)) == (mask<<i)) {
	    if (bus_space_subregion(h->sc->memt, h->sc->memh,
				    i*PCIC_MEM_PAGESIZE,
				    size*PCIC_MEM_PAGESIZE, memh))
		return(1);
	    *mhandle = mask<<i;
	    h->sc->subregionmask &= ~(*mhandle);
	    break;
	}
    }

    if (i == (PCIC_MEM_PAGES+1-size))
	return(1);

    DPRINTF(("pcic_chip_mem_alloc paddr %lx+%lx at vaddr %lx\n",
	     (u_long) pmap_extract(pmap_kernel(), (vm_offset_t) *memh),
	     (u_long) (size-1), (u_long) *memh));

    *memt = h->sc->memt;
    if (realsize)
	*realsize = size*PCIC_MEM_PAGESIZE;

    return(0);
}

void pcic_chip_mem_free(pch, size, memt, memh, mhandle)
     pcmcia_chipset_handle_t pch;
     bus_size_t size;
     bus_space_tag_t memt;
     bus_space_handle_t memh;
     pcmcia_mem_handle_t mhandle;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;

    h->sc->subregionmask |= mhandle;
}

static struct mem_map_index_st {
    int sysmem_start_lsb;
    int sysmem_start_msb;
    int sysmem_stop_lsb;
    int sysmem_stop_msb;
    int cardmem_lsb;
    int cardmem_msb;
    int memenable;
} mem_map_index[] = {
    {
	PCIC_SYSMEM_ADDR0_START_LSB,
	PCIC_SYSMEM_ADDR0_START_MSB,
	PCIC_SYSMEM_ADDR0_STOP_LSB,
	PCIC_SYSMEM_ADDR0_STOP_MSB,
	PCIC_CARDMEM_ADDR0_LSB,
	PCIC_CARDMEM_ADDR0_MSB,
	PCIC_ADDRWIN_ENABLE_MEM0,
    },
    {
	PCIC_SYSMEM_ADDR1_START_LSB,
	PCIC_SYSMEM_ADDR1_START_MSB,
	PCIC_SYSMEM_ADDR1_STOP_LSB,
	PCIC_SYSMEM_ADDR1_STOP_MSB,
	PCIC_CARDMEM_ADDR1_LSB,
	PCIC_CARDMEM_ADDR1_MSB,
	PCIC_ADDRWIN_ENABLE_MEM1,
    },
    {
	PCIC_SYSMEM_ADDR2_START_LSB,
	PCIC_SYSMEM_ADDR2_START_MSB,
	PCIC_SYSMEM_ADDR2_STOP_LSB,
	PCIC_SYSMEM_ADDR2_STOP_MSB,
	PCIC_CARDMEM_ADDR2_LSB,
	PCIC_CARDMEM_ADDR2_MSB,
	PCIC_ADDRWIN_ENABLE_MEM2,
    },
    {
	PCIC_SYSMEM_ADDR3_START_LSB,
	PCIC_SYSMEM_ADDR3_START_MSB,
	PCIC_SYSMEM_ADDR3_STOP_LSB,
	PCIC_SYSMEM_ADDR3_STOP_MSB,
	PCIC_CARDMEM_ADDR3_LSB,
	PCIC_CARDMEM_ADDR3_MSB,
	PCIC_ADDRWIN_ENABLE_MEM3,
    },
    {
	PCIC_SYSMEM_ADDR4_START_LSB,
	PCIC_SYSMEM_ADDR4_START_MSB,
	PCIC_SYSMEM_ADDR4_STOP_LSB,
	PCIC_SYSMEM_ADDR4_STOP_MSB,
	PCIC_CARDMEM_ADDR4_LSB,
	PCIC_CARDMEM_ADDR4_MSB,
	PCIC_ADDRWIN_ENABLE_MEM4,
    },
};

int pcic_chip_mem_map(pch, kind, size, memt, memh, card_addr, offset, window)
     pcmcia_chipset_handle_t pch;
     int kind;
     bus_size_t size;
     bus_space_tag_t memt;
     bus_space_handle_t memh;
     u_long card_addr;
     u_long *offset;
     int *window;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;
    vm_offset_t physaddr;
    long card_offset;
    int i, win;

    win = -1;
    for (i=0; i<(sizeof(mem_map_index)/sizeof(mem_map_index[0])); i++) {
	if ((h->memalloc & (1<<i)) == 0) {
	    win = i;
	    h->memalloc |= (1<<i);
	    break;
	}
    }

    if (win == -1)
	return(1);

    *window = win;

    /* XXX this is pretty gross */

    if (h->sc->memt != memt)
	panic("pcic_chip_mem_map memt is bogus");

    /* convert the memh to a physical address */
    physaddr = pmap_extract(pmap_kernel(), (vm_offset_t) memh);

    /* compute the address offset to the pcmcia address space for the
       pcic.  this is intentionally signed.  The masks and shifts
       below will cause TRT to happen in the pcic registers.  Deal with
       making sure the address is aligned, and return the alignment
       offset.  */

    *offset = card_addr % PCIC_MEM_ALIGN;
    card_addr -= *offset;

    DPRINTF(("pcic_chip_mem_map window %d sys %lx+%lx+%lx at card addr %lx\n",
	     win, physaddr, *offset, size, card_addr));

    /* include the offset in the size, and decrement size by one,
       since the hw wants start/stop */
    size += *offset - 1;

    card_offset = (((long) card_addr) - ((long) physaddr));

    pcic_write(h, mem_map_index[win].sysmem_start_lsb,
	       (physaddr >> PCIC_SYSMEM_ADDRX_SHIFT) & 0xff);
    pcic_write(h, mem_map_index[win].sysmem_start_msb,
	       ((physaddr >> (PCIC_SYSMEM_ADDRX_SHIFT + 8)) &
		PCIC_SYSMEM_ADDRX_START_MSB_ADDR_MASK));

#if 0
    /* XXX do I want 16 bit all the time? */
    PCIC_SYSMEM_ADDRX_START_MSB_DATASIZE_16BIT;
#endif

    pcic_write(h, mem_map_index[win].sysmem_stop_lsb,
	       ((physaddr + size) >> PCIC_SYSMEM_ADDRX_SHIFT) & 0xff);
    pcic_write(h, mem_map_index[win].sysmem_stop_msb,
	       (((physaddr + size) >> (PCIC_SYSMEM_ADDRX_SHIFT + 8)) &
		PCIC_SYSMEM_ADDRX_STOP_MSB_ADDR_MASK) |
	       PCIC_SYSMEM_ADDRX_STOP_MSB_WAIT2);


    pcic_write(h, mem_map_index[win].cardmem_lsb,
	       (card_offset >> PCIC_CARDMEM_ADDRX_SHIFT) & 0xff);
    pcic_write(h, mem_map_index[win].cardmem_msb,
	       ((card_offset >> (PCIC_CARDMEM_ADDRX_SHIFT + 8)) &
		PCIC_CARDMEM_ADDRX_MSB_ADDR_MASK) |
	       ((kind == PCMCIA_MEM_ATTR)?
		PCIC_CARDMEM_ADDRX_MSB_REGACTIVE_ATTR:0));

    reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
    reg |= (mem_map_index[win].memenable | PCIC_ADDRWIN_ENABLE_MEMCS16 );
    pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

#ifdef PCICDEBUG
    {
	int r1,r2,r3,r4,r5,r6;

	r1 = pcic_read(h, mem_map_index[win].sysmem_start_msb);
	r2 = pcic_read(h, mem_map_index[win].sysmem_start_lsb);
	r3 = pcic_read(h, mem_map_index[win].sysmem_stop_msb);
	r4 = pcic_read(h, mem_map_index[win].sysmem_stop_lsb);
	r5 = pcic_read(h, mem_map_index[win].cardmem_msb);
	r6 = pcic_read(h, mem_map_index[win].cardmem_lsb);

	DPRINTF(("pcic_chip_mem_map window %d: %02x%02x %02x%02x %02x%02x\n",
		 win, r1, r2, r3, r4, r5, r6));
    }
#endif

    return(0);
}

void pcic_chip_mem_unmap(pch, window)
     pcmcia_chipset_handle_t pch;
     int window;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;
    
    if (window >= (sizeof(mem_map_index)/sizeof(mem_map_index[0])))
	panic("pcic_chip_mem_unmap: window out of range");

    reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
    reg &= ~mem_map_index[window].memenable;
    pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

    h->memalloc &= ~(1<<window);
}


int pcic_chip_io_alloc(pch, start, size, iot, ioh)
     pcmcia_chipset_handle_t pch;
     bus_addr_t start;
     bus_size_t size;
     bus_space_tag_t *iot;
     bus_space_handle_t *ioh;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    bus_addr_t ioaddr;

    /* 
     * Allocate some arbitrary I/O space.  XXX There really should be a
     * generic isa interface to this, but there isn't currently one
     */

    /* XXX mycroft recommends this I/O space range.  I should put this
       in a header somewhere */

    *iot = h->sc->iot;

    if (start) {
	if (bus_space_map(h->sc->iot, start, size, 0, ioh))
	    return(1);
	DPRINTF(("pcic_chip_io_alloc map port %lx+%lx\n",
		 (u_long) start, (u_long) size));
    } else {
	if (bus_space_alloc(h->sc->iot, 0x400, 0xfff, size, size, 
			    EX_NOBOUNDARY, 0, &ioaddr, ioh))
	    return(1);
	DPRINTF(("pcic_chip_io_alloc alloc port %lx+%lx\n",
		 (u_long) ioaddr, (u_long) size));
    }

    return(0);
}

void pcic_chip_io_free(pch, size, iot, ioh)
     pcmcia_chipset_handle_t pch;
     bus_size_t size;
     bus_space_tag_t iot;
     bus_space_handle_t ioh;
{
    bus_space_free(iot, ioh, size);
}


static struct io_map_index_st {
    int start_lsb;
    int start_msb;
    int stop_lsb;
    int stop_msb;
    int ioenable;
    int ioctlmask;
    int ioctl8;
    int ioctl16;
} io_map_index[] = {
    {
	PCIC_IOADDR0_START_LSB,
	PCIC_IOADDR0_START_MSB,
	PCIC_IOADDR0_STOP_LSB,
	PCIC_IOADDR0_STOP_MSB,
	PCIC_ADDRWIN_ENABLE_IO0,
	PCIC_IOCTL_IO0_WAITSTATE | PCIC_IOCTL_IO0_ZEROWAIT | 
	PCIC_IOCTL_IO0_IOCS16SRC_MASK | PCIC_IOCTL_IO0_DATASIZE_MASK,
	PCIC_IOCTL_IO0_IOCS16SRC_DATASIZE | PCIC_IOCTL_IO0_DATASIZE_8BIT,
	PCIC_IOCTL_IO0_IOCS16SRC_DATASIZE | PCIC_IOCTL_IO0_DATASIZE_16BIT,
    },
    {
	PCIC_IOADDR1_START_LSB,
	PCIC_IOADDR1_START_MSB,
	PCIC_IOADDR1_STOP_LSB,
	PCIC_IOADDR1_STOP_MSB,
	PCIC_ADDRWIN_ENABLE_IO1,
	PCIC_IOCTL_IO1_WAITSTATE | PCIC_IOCTL_IO1_ZEROWAIT | 
	PCIC_IOCTL_IO1_IOCS16SRC_MASK | PCIC_IOCTL_IO1_DATASIZE_MASK,
	PCIC_IOCTL_IO1_IOCS16SRC_DATASIZE | PCIC_IOCTL_IO1_DATASIZE_8BIT,
	PCIC_IOCTL_IO1_IOCS16SRC_DATASIZE | PCIC_IOCTL_IO1_DATASIZE_16BIT,
    },
};

int pcic_chip_io_map(pch, width, size, iot, ioh, window)
     pcmcia_chipset_handle_t pch;
     int width;
     bus_size_t size;
     bus_space_tag_t iot;
     bus_space_handle_t ioh;
     int *window;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;
    int i, win;

    win = -1;
    for (i=0; i<(sizeof(io_map_index)/sizeof(io_map_index[0])); i++) {
	if ((h->ioalloc & (1<<i)) == 0) {
	    win = i;
	    h->ioalloc |= (1<<i);
	    break;
	}
    }

    if (win == -1)
	return(1);

    *window = win;

    /* XXX this is pretty gross */

    if (h->sc->iot != iot)
	panic("pcic_chip_io_map iot is bogus");

    DPRINTF(("pcic_chip_io_map window %d %s port %lx+%lx\n",
	     win, (width == PCMCIA_WIDTH_IO8)?"io8":"io16",
	     (u_long) ioh, (u_long) size));

    pcic_write(h, io_map_index[win].start_lsb, ioh & 0xff);
    pcic_write(h, io_map_index[win].start_msb, (ioh >> 8) & 0xff);

    pcic_write(h, io_map_index[win].stop_lsb, (ioh + size - 1) & 0xff);
    pcic_write(h, io_map_index[win].stop_msb, ((ioh + size - 1) >> 8) & 0xff);

    reg = pcic_read(h, PCIC_IOCTL);
    reg &= ~io_map_index[win].ioctlmask;
    if (width == PCMCIA_WIDTH_IO8)
	reg |= io_map_index[win].ioctl8;
    else
	reg |= io_map_index[win].ioctl16;
    pcic_write(h, PCIC_IOCTL, reg);

    reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
    reg |= io_map_index[win].ioenable;
    pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

    return(0);
}

void pcic_chip_io_unmap(pch, window)
     pcmcia_chipset_handle_t pch;
     int window;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;
    
    if (window >= (sizeof(io_map_index)/sizeof(io_map_index[0])))
	panic("pcic_chip_io_unmap: window out of range");

    reg = pcic_read(h, PCIC_ADDRWIN_ENABLE);
    reg &= ~io_map_index[window].ioenable;
    pcic_write(h, PCIC_ADDRWIN_ENABLE, reg);

    h->ioalloc &= ~(1<<window);
}

void *
pcic_chip_intr_establish(pch, irqmask, ipl, fct, arg)
     pcmcia_chipset_handle_t pch;
     u_int16_t irqmask;
     int ipl;
     int (*fct)(void *);
     void *arg;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int irq;
    void *ih;
    int reg;

    /* Mask out IRQs which we shouldn't allocate. */
    irqmask &= PCIC_INTR_IRQ_VALIDMASK;
    if (irqmask == 0)
	return(NULL);

    isa_intr_alloc(h->sc->ic, irqmask, IST_PULSE, &irq);
    if (!(ih = isa_intr_establish(h->sc->ic, irq, IST_PULSE, ipl, fct, arg)))
	return(NULL);

    reg = pcic_read(h, PCIC_INTR);
    reg &= ~PCIC_INTR_IRQ_MASK;
    reg |= PCIC_INTR_ENABLE;
    reg |= irq;
    pcic_write(h, PCIC_INTR, reg);

    printf("%s: card irq %d\n", h->pcmcia->dv_xname, irq);

    return(ih);
}

void pcic_chip_intr_disestablish(pch, ih)
     pcmcia_chipset_handle_t pch;
     void *ih;
{
    struct pcic_handle *h = (struct pcic_handle *) pch;
    int reg;

    reg = pcic_read(h, PCIC_INTR);
    reg &= ~(PCIC_INTR_IRQ_MASK|PCIC_INTR_ENABLE);
    pcic_write(h, PCIC_INTR, reg);

    isa_intr_disestablish(h->sc->ic, ih);
}
