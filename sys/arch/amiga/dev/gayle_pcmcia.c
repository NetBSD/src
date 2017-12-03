/*	$NetBSD: gayle_pcmcia.c,v 1.25.12.3 2017/12/03 11:35:48 jdolecek Exp $ */

/* public domain */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gayle_pcmcia.c,v 1.25.12.3 2017/12/03 11:35:48 jdolecek Exp $");

/* PCMCIA front-end driver for A1200's and A600's. */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/systm.h>

#include <uvm/uvm.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>

#include <machine/cpu.h>
#include <amiga/amiga/custom.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/gayle.h>
#include <amiga/amiga/isr.h>


/* There is one of these for each slot. And yes, there is only one slot. */
struct pccard_slot {
	struct	pccard_softc *sc;	/* refer to `parent' */
	int	(*intr_func)(void *);
	void *	intr_arg;
	device_t card;
	int	flags;
#define SLOT_OCCUPIED		0x01
#define SLOT_NEW_CARD_EVENT	0x02
};

struct pccard_softc {
	struct bus_space_tag io_space;
	struct bus_space_tag attr_space;
	struct bus_space_tag mem_space;
	struct pccard_slot devs[1];
	struct isr intr6;
	struct isr intr2;
};

static int	pccard_probe(device_t, cfdata_t, void *);
static void	pccard_attach(device_t, device_t, void *);
static void	pccard_attach_slot(struct pccard_slot *);
static int	pccard_intr6(void *);
static int	pccard_intr2(void *);
static void	pccard_kthread(void *);

static int pcf_mem_alloc(pcmcia_chipset_handle_t, bus_size_t,
		struct pcmcia_mem_handle *);
static void pcf_mem_free(pcmcia_chipset_handle_t, struct pcmcia_mem_handle *);
static int pcf_mem_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
		struct pcmcia_mem_handle *, bus_addr_t *, int *);
static void pcf_mem_unmap(pcmcia_chipset_handle_t, int);
static int pcf_io_alloc(pcmcia_chipset_handle_t, bus_addr_t, bus_size_t,
		bus_size_t, struct pcmcia_io_handle *);
static void pcf_io_free(pcmcia_chipset_handle_t, struct pcmcia_io_handle *);
static int pcf_io_map(pcmcia_chipset_handle_t, int, bus_addr_t, bus_size_t,
		struct pcmcia_io_handle *, int *);
static void pcf_io_unmap(pcmcia_chipset_handle_t, int);
static void *pcf_intr_establish(pcmcia_chipset_handle_t,
		struct pcmcia_function *, int, int (*)(void *), void *);
static void pcf_intr_disestablish(pcmcia_chipset_handle_t, void *);
static void pcf_socket_enable(pcmcia_chipset_handle_t);
static void pcf_socket_disable(pcmcia_chipset_handle_t);
static void pcf_socket_settype(pcmcia_chipset_handle_t, int);

static bsr(pcmio_bsr1, u_int8_t);
static bsw(pcmio_bsw1, u_int8_t);
static bsrm(pcmio_bsrm1, u_int8_t);
static bswm(pcmio_bswm1, u_int8_t);
static bsrm(pcmio_bsrr1, u_int8_t);
static bswm(pcmio_bswr1, u_int8_t);
static bssr(pcmio_bssr1, u_int8_t);
static bscr(pcmio_bscr1, u_int8_t);

CFATTACH_DECL_NEW(pccard, sizeof(struct pccard_softc),
    pccard_probe, pccard_attach, NULL, NULL);

static struct pcmcia_chip_functions chip_functions = {
	pcf_mem_alloc,		pcf_mem_free,
	pcf_mem_map,		pcf_mem_unmap,
	pcf_io_alloc,		pcf_io_free,
	pcf_io_map,		pcf_io_unmap,
	pcf_intr_establish,	pcf_intr_disestablish,
	pcf_socket_enable,	pcf_socket_disable,
	pcf_socket_settype
};

static struct amiga_bus_space_methods pcmio_bs_methods;

static u_int8_t *reset_card_reg;

static int
pccard_probe(device_t parent, cfdata_t cf, void *aux)
{

	return (is_a600() || is_a1200()) && matchname(aux, "pccard");
}

static void
pccard_attach(device_t parent, device_t self, void *aux)
{
	struct pccard_softc *sc = device_private(self);
	struct pcmciabus_attach_args paa;
	vaddr_t pcmcia_base;
	vaddr_t i;

	printf("\n");

	gayle_init();

	pcmcia_base = uvm_km_alloc(kernel_map,
				   GAYLE_PCMCIA_END - GAYLE_PCMCIA_START,
				   0, UVM_KMF_VAONLY | UVM_KMF_NOWAIT);
	if (pcmcia_base == 0) {
		printf("attach failed (no virtual memory)\n");
		return;
	}

	for (i = GAYLE_PCMCIA_START; i < GAYLE_PCMCIA_END; i += PAGE_SIZE)
		pmap_enter(vm_map_pmap(kernel_map),
		    i - GAYLE_PCMCIA_START + pcmcia_base, i,
		    VM_PROT_READ | VM_PROT_WRITE, true);
	pmap_update(vm_map_pmap(kernel_map));

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

	reset_card_reg = (u_int8_t *) pcmcia_base +
	    (GAYLE_PCMCIA_RESET - GAYLE_PCMCIA_START);

	sc->io_space.base = (bus_addr_t) pcmcia_base +
	    (GAYLE_PCMCIA_IO_START - GAYLE_PCMCIA_START);
	sc->io_space.absm = &pcmio_bs_methods;

	sc->attr_space.base = (bus_addr_t) pcmcia_base +
	    (GAYLE_PCMCIA_ATTR_START - GAYLE_PCMCIA_START);
	sc->attr_space.absm = &amiga_bus_stride_1;

	/* XXX we should check if the 4M of common memory are actually
	 *	RAM or PCMCIA usable.
	 * For now, we just do as if the 4M were RAM and make common memory
	 * point to attribute memory, which is OK for some I/O cards.
	 */
	sc->mem_space.base = (bus_addr_t) pcmcia_base;
	sc->mem_space.absm = &amiga_bus_stride_1;

	sc->devs[0].sc = sc;
	sc->devs[0].intr_func = NULL;
	sc->devs[0].intr_arg = NULL;
	sc->devs[0].flags = 0;

	gayle_pcmcia_status_write(0);
	gayle_intr_ack(0);
	gayle_pcmcia_config_write(0);
	gayle_intr_enable_set(GAYLE_INT_IDE);

	paa.paa_busname = "pcmcia";
	paa.pct = &chip_functions;
	paa.pch = &sc->devs[0];
	sc->devs[0].card = config_found(self, &paa, simple_devprint);
	if (sc->devs[0].card == NULL) {
		printf("attach failed, config_found() returned NULL\n");
		pmap_remove(kernel_map->pmap, pcmcia_base,
		    pcmcia_base + (GAYLE_PCMCIA_END - GAYLE_PCMCIA_START));
		pmap_update(kernel_map->pmap);
		uvm_deallocate(kernel_map, pcmcia_base,
			GAYLE_PCMCIA_END - GAYLE_PCMCIA_START);
		return;
	}

	sc->intr6.isr_intr = pccard_intr6;
	sc->intr6.isr_arg = sc;
	sc->intr6.isr_ipl = 6;
	add_isr(&sc->intr6);

	sc->intr2.isr_intr = pccard_intr2;
	sc->intr2.isr_arg = sc;
	sc->intr2.isr_ipl = 2;
	add_isr(&sc->intr2);

	if (kthread_create(PRI_NONE, 0, NULL, pccard_kthread, sc,
	    NULL, "pccard")) {
		printf("%s: can't create kernel thread\n",
		       device_xname(self));
		panic("pccard kthread_create() failed");
	}

	gayle_intr_enable_set(GAYLE_INT_DETECT | GAYLE_INT_IREQ);

	/* reset the card if it's already there */
	if (gayle_pcmcia_status_read() & GAYLE_CCMEM_DETECT) {
		volatile u_int8_t x;

		gayle_intr_ack(0xff);
		delay(500);
		gayle_intr_ack(0xfc);

		delay(100*1000);

		*reset_card_reg = 0x0;
		delay(1000);
		x = *reset_card_reg;
		__USE(x);
		gayle_pcmcia_status_write(GAYLE_CCMEM_WP | GAYLE_CCIO_SPKR);
	}

	pccard_attach_slot(&sc->devs[0]);
}

static int
pccard_intr6(void *arg)
{
	struct pccard_softc *sc = arg;

	if (gayle_intr_status() & GAYLE_INT_DETECT) {
		gayle_intr_ack(GAYLE_INT_IDE | GAYLE_INT_STSCHG |
		    GAYLE_INT_SPKR | GAYLE_INT_WP | GAYLE_INT_IREQ);
		sc->devs[0].flags |= SLOT_NEW_CARD_EVENT;
		return 1;
	}
	return 0;
}

static int
pccard_intr2(void *arg)
{
	struct pccard_softc *sc = arg;
	struct pccard_slot *slot = &sc->devs[0];

	if (slot->flags & SLOT_NEW_CARD_EVENT) {
		slot->flags &= ~SLOT_NEW_CARD_EVENT;

		/* reset the registers */
		gayle_intr_ack(GAYLE_INT_IDE | GAYLE_INT_DETECT);
		gayle_pcmcia_status_write(GAYLE_CCMEM_WP | GAYLE_CCIO_SPKR);
		gayle_pcmcia_config_write(0);
		pccard_attach_slot(&sc->devs[0]);
	} else {
		int intreq = gayle_intr_status() &
		    (GAYLE_INT_STSCHG | GAYLE_INT_WP | GAYLE_INT_IREQ);
		if (intreq) {
			gayle_intr_ack((intreq ^ 0x2c) | 0xc0);

			return slot->flags & SLOT_OCCUPIED &&
		   		slot->intr_func != NULL &&
				slot->intr_func(slot->intr_arg);
		}
	}
	return 0;
}

static void
pccard_kthread(void *arg)
{
	struct pccard_softc *sc = arg;
	struct pccard_slot *slot = &sc->devs[0];

	for (;;) {
		int s = spl2();

		if (slot->flags & SLOT_NEW_CARD_EVENT) {
			slot->flags &= ~SLOT_NEW_CARD_EVENT;
			gayle_intr_ack(0xc0);

			/* reset the registers */
			gayle_intr_ack(GAYLE_INT_IDE | GAYLE_INT_DETECT);
			gayle_pcmcia_status_write(GAYLE_CCMEM_WP | GAYLE_CCIO_SPKR);
			gayle_pcmcia_config_write(0);
			pccard_attach_slot(&sc->devs[0]);
		}
		splx(s);

		tsleep(slot, PWAIT, "pccthread", hz);
	}
}

static void
pccard_attach_slot(struct pccard_slot *slot)
{

	if (!(slot->flags & SLOT_OCCUPIED) &&
			gayle_pcmcia_status_read() & GAYLE_CCMEM_DETECT) {
		if (pcmcia_card_attach(slot->card) == 0)
			slot->flags |= SLOT_OCCUPIED;
	}
}

static int
pcf_mem_alloc(pcmcia_chipset_handle_t pch, bus_size_t bsz,
	      struct pcmcia_mem_handle *pcmh)
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	pcmh->memt = &slot->sc->attr_space;
	pcmh->memh = pcmh->memt->base;
	return 0;
}

static void
pcf_mem_free(pcmcia_chipset_handle_t pch, struct pcmcia_mem_handle *memh)
{
}

static int
pcf_mem_map(pcmcia_chipset_handle_t pch, int kind, bus_addr_t addr,
	    bus_size_t size, struct pcmcia_mem_handle *pcmh,
	    bus_addr_t *offsetp, int *windowp)
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	/* Ignore width requirements */
	kind &= ~PCMCIA_WIDTH_MEM_MASK;

	switch (kind) {
	case PCMCIA_MEM_ATTR:
		pcmh->memt = &slot->sc->attr_space;
		break;
	case PCMCIA_MEM_COMMON:
		pcmh->memt = &slot->sc->mem_space;
		break;
	default:
		/* This means that this code needs an update/a bugfix */
		printf(__FILE__ ": unknown kind %d of PCMCIA memory\n", kind);
		return 1;
	}

	bus_space_map(pcmh->memt, addr, size, 0, &pcmh->memh);
	*offsetp = 0;
	*windowp = 0;			/* unused */

	return 0;
}

static void
pcf_mem_unmap(pcmcia_chipset_handle_t pch, int win)
{
}

static int
pcf_io_alloc(pcmcia_chipset_handle_t pch, bus_addr_t start, bus_size_t size,
	     bus_size_t align, struct pcmcia_io_handle *pcihp)
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	pcihp->iot = &slot->sc->io_space;
	pcihp->ioh = pcihp->iot->base;
	return 0;
}

static void
pcf_io_free(pcmcia_chipset_handle_t pch, struct pcmcia_io_handle *pcihp)
{
}

static int
pcf_io_map(pcmcia_chipset_handle_t pch, int width, bus_addr_t offset,
	   bus_size_t size, struct pcmcia_io_handle *pcihp, int *windowp)
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;

	pcihp->iot = &slot->sc->io_space;
	bus_space_map(pcihp->iot, offset, size, 0, &pcihp->ioh);

	*windowp = 0;		/* unused */
	return 0;
}

static void
pcf_io_unmap(pcmcia_chipset_handle_t pch, int win)
{
}

static void *
pcf_intr_establish(pcmcia_chipset_handle_t pch, struct pcmcia_function *pf,
		   int ipl, int (*func)(void *), void *arg)
{
	struct pccard_slot *slot = (struct pccard_slot *) pch;
	int s;

	s = splhigh();
	if (slot->intr_func == NULL) {
		slot->intr_func = func;
		slot->intr_arg = arg;
	} else {
		/* if we are here, we need to put intrs into a list */
		printf("ARGH! see " __FILE__ "\n");
		slot = NULL;
	}
	splx(s);

	return slot;
}

static void
pcf_intr_disestablish(pcmcia_chipset_handle_t pch, void *intr_handler)
{
	struct pccard_slot *slot = (struct pccard_slot *) intr_handler;

	if (slot != NULL) {
		slot->intr_func = NULL;
		slot->intr_arg = NULL;
	}
}

static void
pcf_socket_enable(pcmcia_chipset_handle_t pch)
{
}

static void
pcf_socket_disable(pcmcia_chipset_handle_t pch)
{
}

static void
pcf_socket_settype(pcmcia_chipset_handle_t pch, int type) {
}

static u_int8_t
pcmio_bsr1(bus_space_handle_t h, bus_size_t o)
{

	return *((volatile u_int8_t *) h + o + (o & 1 ? 0xffff : 0));
}

static void
pcmio_bsw1(bus_space_handle_t h, bus_size_t o, unsigned v)
{

	*((volatile u_int8_t *) h + o + (o & 1 ? 0xffff : 0)) = v;
}

static void
pcmio_bsrm1(bus_space_handle_t h, bus_size_t o, u_int8_t *p, bus_size_t c)
{
	volatile u_int8_t *src = (volatile u_int8_t *)
		(h + o + (o & 1 ? 0xffff : 0));


	/* XXX we can (should, must) optimize this if c >= 4 */
	for (; c > 0; c--)
		*p++ = *src;
}


static void
pcmio_bswm1(bus_space_handle_t h, bus_size_t o, const u_int8_t *p, bus_size_t c)
{
	volatile u_int8_t *dst = (volatile u_int8_t *)
		(h + o + (o & 1 ? 0xffff : 0));


	/* XXX we can (should, must) optimize this if c >= 4 */
	for (; c > 0; c--)
		*dst = *p++;
}

static void
pcmio_bsrr1(bus_space_handle_t h, bus_size_t o, u_int8_t *p, bus_size_t c)
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
pcmio_bswr1(bus_space_handle_t h, bus_size_t o, const u_int8_t *p, bus_size_t c)
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
pcmio_bssr1(bus_space_handle_t h, bus_size_t o, unsigned v, bus_size_t c)
{

	panic("pcmio_bssr1 is not defined (" __FILE__ ")");
}

void
pcmio_bscr1(bus_space_handle_t h, bus_size_t o, bus_space_handle_t g,
	    bus_size_t q, bus_size_t c)
{

	panic("pcmio_bscr1 is not defined (" __FILE__ ")");
}
