/*	$NetBSD: gayle_pcmcia.c,v 1.3 2000/06/26 14:20:30 mrg Exp $	*/

/* PCMCIA front-end driver for A1200's and A600's. */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/systm.h>

#include <vm/vm.h>

#include <uvm/uvm.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#include <machine/cpu.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/gayle.h>
#include <amiga/amiga/isr.h>


/*
 * There is one of these for each slot. This is useless since we have only one,
 * but it makes it clearer if someone wants to understand better the NetBSD
 * device drivers scheme.
 */
struct pccard_slot {
	struct	pccard_softc *sc;	/* refer to `parent' */
	int	(*intr_func)(void *);
	void *	intr_arg;
	struct	device *card;
	int	flags;
#define SLOT_OCCUPIED		0x01
#define SLOT_NEW_CARD_EVENT	0x02
};

struct pccard_softc {
	struct device sc_dev;
	struct bus_space_tag io_space;
	struct bus_space_tag attr_space;
	struct bus_space_tag mem_space;
	struct pccard_slot devs[1];
	struct isr intr6;
	struct isr intr2;
};

static int	pccard_probe __P((struct device *, struct cfdata *, void *));
static void	pccard_attach __P((struct device *, struct device *, void *));
static void	pccard_attach_slot __P((struct pccard_slot *));
static int	pccard_intr6 __P((void *));
static int	pccard_intr2 __P((void *));
static void	pccard_create_kthread __P((void *));
static void	pccard_kthread __P((void *));

static int pcf_mem_alloc __P((pcmcia_chipset_handle_t, bus_size_t,
		struct pcmcia_mem_handle *));
static void pcf_mem_free __P((pcmcia_chipset_handle_t,
		struct pcmcia_mem_handle *));
static int pcf_mem_map __P((pcmcia_chipset_handle_t, int, bus_addr_t,
		bus_size_t, struct pcmcia_mem_handle *, bus_addr_t *, int *));
static void pcf_mem_unmap __P((pcmcia_chipset_handle_t, int));
static int pcf_io_alloc __P((pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
		bus_size_t, struct pcmcia_io_handle *));
static void pcf_io_free __P((pcmcia_chipset_handle_t,
		struct pcmcia_io_handle *));
static int pcf_io_map __P((pcmcia_chipset_handle_t, int, bus_addr_t,
		bus_size_t, struct pcmcia_io_handle *, int *));
static void pcf_io_unmap __P((pcmcia_chipset_handle_t, int));
static void *pcf_intr_establish __P((pcmcia_chipset_handle_t,
		struct pcmcia_function *, int, int (*)(void *), void *));
static void pcf_intr_disestablish __P((pcmcia_chipset_handle_t, void *));
static void pcf_socket_enable __P((pcmcia_chipset_handle_t));
static void pcf_socket_disable __P((pcmcia_chipset_handle_t));

static bsr(pcmio_bsr1, u_int8_t);
static bsw(pcmio_bsw1, u_int8_t);
static bsrm(pcmio_bsrm1, u_int8_t);
static bswm(pcmio_bswm1, u_int8_t);
static bsrm(pcmio_bsrr1, u_int8_t);
static bswm(pcmio_bswr1, u_int8_t);
static bssr(pcmio_bssr1, u_int8_t);
static bscr(pcmio_bscr1, u_int8_t);

static u_int8_t *reset_card_reg;

struct cfattach pccard_ca = {
	sizeof(struct pccard_softc), pccard_probe, pccard_attach
};

struct pcmcia_chip_functions chip_functions = {
	pcf_mem_alloc,		pcf_mem_free,
	pcf_mem_map,		pcf_mem_unmap,
	pcf_io_alloc,		pcf_io_free,
	pcf_io_map,		pcf_io_unmap,
	pcf_intr_establish,	pcf_intr_disestablish,
	pcf_socket_enable,	pcf_socket_disable
};

struct amiga_bus_space_methods pcmio_bs_methods;

static int
pccard_probe(dev, cfd, aux)
	struct device *dev;
	struct cfdata *cfd;
	void *aux;
{
	return (/*is_a600() || */is_a1200()) && matchname(aux, "pccard");
}

static void
pccard_attach(parent, myself, aux)
	struct device *parent, *myself;
	void *aux;
{
	struct pccard_softc *self = (struct pccard_softc *) myself;
	struct pcmciabus_attach_args paa;
	vaddr_t pcmcia_base = GAYLE_PCMCIA_START;
	vaddr_t i;
	int ret;

	printf("\n");

	gayle_init();

	ret = uvm_map(kernel_map, &pcmcia_base,
		GAYLE_PCMCIA_END - GAYLE_PCMCIA_START, NULL,
		UVM_UNKNOWN_OFFSET, UVM_MAPFLAG(UVM_PROT_NONE, UVM_PROT_NONE,
			UVM_INH_NONE, UVM_ADV_RANDOM, 0));
	if (ret != KERN_SUCCESS) {
		printf("attach failed (no virtual memory)\n");
		return;
	}

	for (i = GAYLE_PCMCIA_START; i < GAYLE_PCMCIA_END; i += PAGE_SIZE)
		pmap_enter(kernel_map->pmap,
		    i - GAYLE_PCMCIA_START + pcmcia_base, i,
		    VM_PROT_READ | VM_PROT_WRITE, TRUE);

	/* override the one-byte access methods for I/O space */
	pcmio_bs_methods = amiga_bus_stride_1;
	pcmio_bs_methods.bsr1 = pcmio_bsr1;
	pcmio_bs_methods.bsw1 = pcmio_bsw1;
	pcmio_bs_methods.bsrm1 = pcmio_bsrm1;
	pcmio_bs_methods.bswm1 = pcmio_bswm1;
	pcmio_bs_methods.bsrr1 = pcmio_bsrr1;
	pcmio_bs_methods.bswr1 = pcmio_bswr1;
	pcmio_bs_methods.bssr1 = pcmio_bssr1;
	pcmio_bs_methods.bscr1 = pcmio_bscr1;

	reset_card_reg = (u_int8_t *) pcmcia_base - GAYLE_PCMCIA_START +
	    GAYLE_PCMCIA_RESET;

	self->io_space.base = (u_int) pcmcia_base - GAYLE_PCMCIA_START +
	    GAYLE_PCMCIA_IO_START;
	self->io_space.absm = &pcmio_bs_methods;

	self->attr_space.base = (u_int) pcmcia_base - GAYLE_PCMCIA_START +
	    GAYLE_PCMCIA_ATTR_START;
	self->attr_space.absm = &amiga_bus_stride_1;

	/* XXX should be invalid */
	self->mem_space.base = (u_int) pcmcia_base;
	self->mem_space.absm = &amiga_bus_stride_1;

	self->devs[0].sc = self;
	self->devs[0].intr_func = NULL;
	self->devs[0].intr_arg = NULL;
	self->devs[0].flags = 0;

	gayle.pcc_status = 0;
	gayle.intreq = 0;
	gayle.pcc_config = 0;
	gayle.intena &= GAYLE_INT_IDE;

	paa.paa_busname = "pcmcia";
	paa.pct = &chip_functions;
	paa.pch = &self->devs[0];
	paa.iobase = 0;
	paa.iosize = 0;
	self->devs[0].card =
		config_found_sm(myself, &paa, simple_devprint, NULL);
	if (self->devs[0].card == NULL) {
		printf("attach failed, config_found_sm() returned NULL\n");
		return;
	}

	self->intr6.isr_intr = pccard_intr6;
	self->intr6.isr_arg = self;
	self->intr6.isr_ipl = 6;
	add_isr(&self->intr6);

	self->intr2.isr_intr = pccard_intr2;
	self->intr2.isr_arg = self;
	self->intr2.isr_ipl = 2;
	add_isr(&self->intr2);

	kthread_create(pccard_create_kthread, self);

	gayle.intena |= GAYLE_INT_DETECT | GAYLE_INT_IREQ;

	/* reset the card if it's already there */
	if (gayle.pcc_status & GAYLE_CCMEM_DETECT) {
		volatile u_int8_t x;
		*reset_card_reg = 0x0;
		delay(1000);
		x = *reset_card_reg;
		gayle.pcc_status = GAYLE_CCMEM_WP | GAYLE_CCIO_SPKR;
	}

	pccard_attach_slot(&self->devs[0]);
}

/* This is called as soon as it is possible to create a kernel thread */
static void
pccard_create_kthread(arg)
	void *arg;
{
	struct pccard_softc *self = arg;

	if (kthread_create1(pccard_kthread, self, NULL, "pccard thread")) {
		printf("%s: can't create kernel thread\n",
			self->sc_dev.dv_xname);
		panic("pccard kthread_create() failed");
	}
}

static int
pccard_intr6(arg)
	void *arg;
{
	struct pccard_softc *self = arg;

	if (gayle.intreq & GAYLE_INT_DETECT) {
		gayle.intreq = GAYLE_INT_IDE | GAYLE_INT_STSCHG |
		    GAYLE_INT_SPKR | GAYLE_INT_WP | GAYLE_INT_IREQ;
		self->devs[0].flags |= SLOT_NEW_CARD_EVENT;
		return 1;
	}
	return 0;
}

static int
pccard_intr2(arg)
	void *arg;
{
	struct pccard_softc *self = arg;
	struct pccard_slot *slot = &self->devs[0];

	if (slot->flags & SLOT_NEW_CARD_EVENT) {
		slot->flags &= ~SLOT_NEW_CARD_EVENT;

		/* reset the registers */
		gayle.intreq = GAYLE_INT_IDE | GAYLE_INT_DETECT;
		gayle.pcc_status = GAYLE_CCMEM_WP | GAYLE_CCIO_SPKR;
		gayle.pcc_config = 0;
		pccard_attach_slot(&self->devs[0]);
	} else {
		int intreq = gayle.intreq &
		    (GAYLE_INT_STSCHG | GAYLE_INT_WP | GAYLE_INT_IREQ);
		if (intreq) {
			gayle.intreq = (intreq ^ 0x2c) | 0xc0;

			if (slot->flags & SLOT_OCCUPIED &&
		   	    slot->intr_func != NULL)
				slot->intr_func(slot->intr_arg);

			return 1;
		}
	}
	return 0;
}

static void
pccard_kthread(arg)
	void *arg;
{
	struct pccard_softc *self = arg;
	struct pccard_slot *slot = &self->devs[0];

	for (;;) {
		int s = spl2();

		if (slot->flags & SLOT_NEW_CARD_EVENT) {
			slot->flags &= ~SLOT_NEW_CARD_EVENT;
			gayle.intreq = 0xc0;

			/* reset the registers */
			gayle.intreq = GAYLE_INT_IDE | GAYLE_INT_DETECT;
			gayle.pcc_status = GAYLE_CCMEM_WP | GAYLE_CCIO_SPKR;
			gayle.pcc_config = 0;
			pccard_attach_slot(&self->devs[0]);
		}
		splx(s);

		tsleep(slot, PWAIT, "pccthread", hz);
	}
}

static void
pccard_attach_slot(slot)
	struct pccard_slot *slot;
{
	if (!(slot->flags & SLOT_OCCUPIED) &&
			gayle.pcc_status & GAYLE_CCMEM_DETECT) {
		if (pcmcia_card_attach(slot->card) == 0)
			slot->flags |= SLOT_OCCUPIED;
	}
}

static int
pcf_mem_alloc(pch, bsz, pcmh)
	pcmcia_chipset_handle_t pch;
	bus_size_t bsz;
	struct pcmcia_mem_handle *pcmh;
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;
	pcmh->memt = &slot->sc->attr_space;
	pcmh->memh = pcmh->memt->base;
	return 0;
}

static void
pcf_mem_free(pch, memh)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_mem_handle *memh;
{
}

static int
pcf_mem_map(pch, kind, addr, size, pcmh, offsetp, windowp)
	pcmcia_chipset_handle_t pch;
	int kind;
	bus_addr_t addr;
	bus_size_t size;
	struct pcmcia_mem_handle *pcmh;
	bus_addr_t *offsetp;
	int *windowp;
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	switch (kind) {
	case PCMCIA_MEM_ATTR:
		pcmh->memt = &slot->sc->attr_space;
		break;
	case PCMCIA_MEM_COMMON:
		pcmh->memt = &slot->sc->mem_space;
		break;
	default:
		/* This means that this code needs an update/a bugfix */
		printf("Unknown kind of PCMCIA memory (amiga/dev/pccard.c)\n");
		return 1;
	}

	bus_space_map(pcmh->memt, addr, size, 0, &pcmh->memh);
	*offsetp = 0;
	*windowp = 0;			/* unused */

	return 0;
}

static void
pcf_mem_unmap(pch, win)
	pcmcia_chipset_handle_t pch;
	int win;
{
}

static int
pcf_io_alloc(pch, start, size, align, pcihp)
	pcmcia_chipset_handle_t pch;
	bus_addr_t start;
	bus_size_t size;
	bus_size_t align;
	struct pcmcia_io_handle *pcihp;
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	pcihp->iot = &slot->sc->io_space;
	pcihp->ioh = pcihp->iot->base;
	return 0;
}

static void
pcf_io_free(pch, pcihp)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_io_handle *pcihp;
{
}

static int
pcf_io_map(pch, width, offset, size, pcihp, windowp)
	pcmcia_chipset_handle_t pch;
	int width;
	bus_addr_t offset;
	bus_size_t size;
	struct pcmcia_io_handle *pcihp;
	int *windowp;
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	pcihp->iot = &slot->sc->io_space;
	pcihp->ioh = offset;

	*windowp = 0;		/* unused */
	return 0;
}

static void
pcf_io_unmap(pch, win)
	pcmcia_chipset_handle_t pch;
	int win;
{
}

static void *
pcf_intr_establish(pch, pf, ipl, func, arg)
	pcmcia_chipset_handle_t pch;
	struct pcmcia_function *pf;
	int ipl;
	int (*func)(void *);
	void *arg;
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;
	int s;

	s = splhigh();
	if (slot->intr_func == NULL) {
		slot->intr_func = func;
		slot->intr_arg = arg;
	} else {
		/* if we are here, we need to put intrs into a list */
		printf("ARGH! see arch/amiga/dev/pccard.c\n");
		slot = NULL;
	}
	splx(s);

	return slot;
}

static void
pcf_intr_disestablish(pch, intr_handler)
	pcmcia_chipset_handle_t pch;
	void *intr_handler;
{
	struct pccard_slot *slot = (struct pccard_slot *) intr_handler;

	if (slot != NULL) {
		slot->intr_func = NULL;
		slot->intr_arg = NULL;
	}
}

static void
pcf_socket_enable(pch)
	pcmcia_chipset_handle_t pch;
{
}

static void
pcf_socket_disable(pch)
	pcmcia_chipset_handle_t pch;
{
}


static u_int8_t
pcmio_bsr1(h, o)
	bus_space_handle_t h;
	bus_size_t o;
{
	return *((volatile u_int8_t *) h + o + (o & 1 ? 0xffff : 0));
}

static void
pcmio_bsw1(h, o, v)
	bus_space_handle_t h;
	bus_size_t o;
	unsigned v;
{
	*((volatile u_int8_t *) h + o + (o & 1 ? 0xffff : 0)) = v;
}

static void
pcmio_bsrm1(h, o, p, c)
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t *p;
	bus_size_t c;
{
	volatile u_int8_t *src = (volatile u_int8_t *) (h + o +
							(o & 1 ? 0xffff : 0));


	/* XXX we can (should, must) optimize this if c >= 4 */
	for (; c > 0; c--)
		*p++ = *src;
}


static void
pcmio_bswm1(h, o, p, c)
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *p;
	bus_size_t c;
{
	volatile u_int8_t *dst = (volatile u_int8_t *) (h + o +
							(o & 1 ? 0xffff : 0));


	/* XXX we can (should, must) optimize this if c >= 4 */
	for (; c > 0; c--)
		*dst = *p++;
}

static void
pcmio_bsrr1(h, o, p, c)
	bus_space_handle_t h;
	bus_size_t o;
	u_int8_t *p;
	bus_size_t c;
{
	volatile u_int8_t *cp1;
	volatile u_int8_t *cp2;
	volatile u_int8_t *temp;

	if (o & 1) {
		cp1 = (volatile u_int8_t *) h + o + 0x10000;
		cp2 = (volatile u_int8_t *) h + o;
	} else {
		cp1 = (volatile u_int8_t *) h + o;
		cp2 = (volatile u_int8_t *) h + o + 0x10000 + 2;
	}

	/* XXX we can (should, must) optimize this if c >= 4 */
	for (; c > 0; c--) {
		*p++ = *cp1;
		cp1 += 2;

		/* swap pointers - hope gcc generates exg for this ;) */
		temp = cp1;
		cp1 = cp2;
		cp2 = temp;
	}
}


static void
pcmio_bswr1(h, o, p, c)
	bus_space_handle_t h;
	bus_size_t o;
	const u_int8_t *p;
	bus_size_t c;
{
	volatile u_int8_t *cp1;
	volatile u_int8_t *cp2;
	volatile u_int8_t *temp;

	if (o & 1) {
		cp1 = (volatile u_int8_t *) h + o + 0x10000;
		cp2 = (volatile u_int8_t *) h + o;
	} else {
		cp1 = (volatile u_int8_t *) h + o;
		cp2 = (volatile u_int8_t *) h + o + 0x10000 + 2;
	}

	/* XXX we can (should, must) optimize this if c >= 4 */
	for (; c > 0; c--) {
		*cp1 = *p++;
		cp1 += 2;

		/* swap pointers - hope gcc generates exg for this ;) */
		temp = cp1;
		cp1 = cp2;
		cp2 = temp;
	}
}

void
pcmio_bssr1(h, o, v, c)
	bus_space_handle_t h;
	bus_size_t o;
	unsigned v;
	bus_size_t c;
{
	panic("pcmio_bssr1 is not defined (" __FILE__ ")");
}

void
pcmio_bscr1(h, o, g, q, c)
	bus_space_handle_t h;
	bus_size_t o;
	bus_space_handle_t g;
	bus_size_t q;
	bus_size_t c;
{
	panic("pcmio_bscr1 is not defined (" __FILE__ ")");
}
