/*	$NetBSD: pci_machdep.c,v 1.32 2001/05/28 07:22:37 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles M. Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles M. Hannum.
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

#include "opt_mbtype.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#define _ATARI_BUS_DMA_PRIVATE
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>

#include <atari/atari/device.h>
#include <atari/pci/pci_vga.h>

/*
 * Sizes of pci memory and I/O area.
 */
#define PCI_MEM_END     0x10000000      /* 256 MByte */
#define PCI_IO_END      0x10000000      /* 256 MByte */

/*
 * We preserve some space at the begin of the pci area for 32BIT_1M
 * devices and standard vga.
 */
#define PCI_MEM_START   0x00100000      /*   1 MByte */
#define PCI_IO_START    0x00004000      /*  16 kByte (some PCI cards allow only
					    I/O addresses up to 0xffff) */

/*
 * PCI memory and IO should be aligned acording to this masks
 */
#define PCI_MACHDEP_IO_ALIGN_MASK	0xffffff00
#define PCI_MACHDEP_MEM_ALIGN_MASK	0xfffff000

/*
 * Convert a PCI 'device' number to a slot number.
 */
#define	DEV2SLOT(dev)	(3 - dev)

/*
 * Struct to hold the memory and I/O datas of the pci devices
 */
struct pci_memreg {
    LIST_ENTRY(pci_memreg) link;
    int dev;
    pcitag_t tag;
    pcireg_t reg, address, mask;
    u_int32_t size;
    u_int32_t csr;
};

typedef LIST_HEAD(pci_memreg_head, pci_memreg) PCI_MEMREG;

/*
 * Entry points for PCI DMA.  Use only the 'standard' functions.
 */
int	_bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
struct atari_bus_dma_tag pci_bus_dma_tag = {
	0,
#ifdef _ATARIHW_
	0x80000000, /* On the Hades, CPU memory starts here PCI-wise */
#else
	0,
#endif
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
};

int	pcibusprint __P((void *auxp, const char *));
int	pcibusmatch __P((struct device *, struct cfdata *, void *));
void	pcibusattach __P((struct device *, struct device *, void *));

static void enable_pci_devices __P((void));
static void insert_into_list __P((PCI_MEMREG *head, struct pci_memreg *elem));
static int overlap_pci_areas __P((struct pci_memreg *p,
	struct pci_memreg *self, u_int addr, u_int size, u_int what));

struct cfattach pcibus_ca = {
	sizeof(struct device), pcibusmatch, pcibusattach
};

/*
 * We need some static storage to probe pci-busses for VGA cards during
 * early console init.
 */
static struct atari_bus_space	bs_storage[2];	/* 1 iot, 1 memt */

int
pcibusmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	static int	nmatched = 0;

	if (strcmp((char *)auxp, "pcibus"))
		return (0);	/* Wrong number... */

	if(atari_realconfig == 0)
		return (1);

	if (machineid & (ATARI_HADES|ATARI_MILAN)) {
		/*
		 * Both Hades and Milan have only one pci bus
		 */
		if (nmatched)
			return (0);
		nmatched++;
		return (1);
	}
	return (0);
}

void
pcibusattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct pcibus_attach_args	pba;

	pba.pba_busname = "pci";
	pba.pba_pc      = NULL;
	pba.pba_bus     = 0;
	pba.pba_flags	= PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_dmat	= &pci_bus_dma_tag;
	pba.pba_iot     = leb_alloc_bus_space_tag(&bs_storage[0]);
	pba.pba_memt    = leb_alloc_bus_space_tag(&bs_storage[1]);
	if ((pba.pba_iot == NULL) || (pba.pba_memt == NULL)) {
		printf("leb_alloc_bus_space_tag failed!\n");
		return;
	}
	pba.pba_iot->base  = PCI_IO_PHYS;
	pba.pba_memt->base = PCI_MEM_PHYS;

	if (dp == NULL) {
		/*
		 * Scan the bus for a VGA-card that we support. If we
		 * find one, try to initialize it to a 'standard' text
		 * mode (80x25).
		 */
		check_for_vga(pba.pba_iot, pba.pba_memt);
		return;
	}

	enable_pci_devices();

#if defined(_ATARIHW_)
	MFP2->mf_aer &= ~(0x27); /* PCI interrupts: HIGH -> LOW */
#endif

	printf("\n");

	config_found(dp, &pba, pcibusprint);
}

int
pcibusprint(auxp, name)
void		*auxp;
const char	*name;
{
	if(name == NULL)
		return(UNCONF);
	return(QUIET);
}

void
pci_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
}

/*
 * Initialize the PCI-bus. The Atari-BIOS does not do this, so....
 * We only disable all devices here. Memory and I/O enabling is done
 * later at pcibusattach.
 */
void
init_pci_bus()
{
	pci_chipset_tag_t	pc = NULL; /* XXX */
	pcitag_t		tag;
	pcireg_t		csr;
	int			device, id, maxndevs;

	tag   = 0;
	id    = 0;
	
	maxndevs = pci_bus_maxdevs(pc, 0);

	for (device = 0; device < maxndevs; device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;

		csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		csr &= ~(PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_IO_ENABLE);
		csr &= ~PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);
	}
}

/*
 * insert a new element in an existing list that the ID's (size in struct
 * pci_memreg) are sorted.
 */
static void
insert_into_list(head, elem)
    PCI_MEMREG *head;
    struct pci_memreg *elem;
{
    struct pci_memreg *p, *q;

    p = LIST_FIRST(head);
    q = NULL;

    for (; p != NULL && p->size < elem->size; q = p, p = LIST_NEXT(p, link));

    if (q == NULL) {
	LIST_INSERT_HEAD(head, elem, link);
    } else {
	LIST_INSERT_AFTER(q, elem, link);
    }
}

/*
 * Test if a new selected area overlaps with an already (probably preselected)
 * pci area.
 */
static int
overlap_pci_areas(p, self, addr, size, what)
    struct pci_memreg *p, *self;
    u_int addr, size, what;
{
    struct pci_memreg *q;

    if (p == NULL)
	return 0;
    
    q = p;
    while (q != NULL) {
      if ((q != self) && (q->csr & what)) {
	if ((addr >= q->address) && (addr < (q->address + q->size))) {
#ifdef DEBUG_PCI_MACHDEP
	  printf("\noverlap area dev %d reg 0x%02x with dev %d reg 0x%02x",
			self->dev, self->reg, q->dev, q->reg);
#endif
	  return 1;
	}
	if ((q->address >= addr) && (q->address < (addr + size))) {
#ifdef DEBUG_PCI_MACHDEP
	  printf("\noverlap area dev %d reg 0x%02x with dev %d reg 0x%02x",
			self->dev, self->reg, q->dev, q->reg);
#endif
	  return 1;
	}
      }
      q = LIST_NEXT(q, link);
    }
    return 0;
}

/*
 * Enable memory and I/O on pci devices. Care about already enabled devices
 * (probabaly by the console driver).
 *
 * The idea behind the following code is:
 * We build a by sizes sorted list of the requirements of the different
 * pci devices. After that we choose the start addresses of that areas
 * in such a way that they are placed as closed as possible together.
 */
static void
enable_pci_devices()
{
    PCI_MEMREG memlist;
    PCI_MEMREG iolist;
    struct pci_memreg *p, *q;
    int dev, reg, id, class;
    pcitag_t tag;
    pcireg_t csr, address, mask;
    pci_chipset_tag_t pc;
    int sizecnt, membase_1m;

    pc = 0;
    csr = 0;
    tag = 0;

    LIST_INIT(&memlist);
    LIST_INIT(&iolist);

    /*
     * first step: go through all devices and gather memory and I/O
     * sizes
     */
    for (dev = 0; dev < pci_bus_maxdevs(pc,0); dev++) {

	tag = pci_make_tag(pc, 0, dev, 0);
	id  = pci_conf_read(pc, tag, PCI_ID_REG);
	if (id == 0 || id == 0xffffffff)
	    continue;

	csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);

	/*
	 * special case: if a display card is found and memory is enabled
	 * preserve 128k at 0xa0000 as vga memory.
	 * XXX: if a display card is found without being enabled, leave
	 *      it alone! You will usually only create conflicts by enabeling
	 *      it.
	 */
	class = pci_conf_read(pc, tag, PCI_CLASS_REG);
	switch (PCI_CLASS(class)) {
	    case PCI_CLASS_PREHISTORIC:
	    case PCI_CLASS_DISPLAY:
	      if (csr & (PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE)) {
		    p = (struct pci_memreg *)malloc(sizeof(struct pci_memreg),
				M_TEMP, M_WAITOK);
		    memset(p, '\0', sizeof(struct pci_memreg));
		    p->dev = dev;
		    p->csr = csr;
		    p->tag = tag;
		    p->reg = 0;     /* there is no register about this */
		    p->size = 0x20000;  /* 128kByte */
		    p->mask = 0xfffe0000;
		    p->address = 0xa0000;

		    insert_into_list(&memlist, p);
	      }
	      else continue;
	}

	for (reg = PCI_MAPREG_START; reg < PCI_MAPREG_END; reg += 4) {

	    address = pci_conf_read(pc, tag, reg);
	    pci_conf_write(pc, tag, reg, 0xffffffff);
	    mask    = pci_conf_read(pc, tag, reg);
	    pci_conf_write(pc, tag, reg, address);
	    if (mask == 0)
		continue; /* Register unused */

	    p = (struct pci_memreg *)malloc(sizeof(struct pci_memreg),
			M_TEMP, M_WAITOK);
	    memset(p, '\0', sizeof(struct pci_memreg));
	    p->dev = dev;
	    p->csr = csr;
	    p->tag = tag;
	    p->reg = reg;
	    p->mask = mask;
	    p->address = 0;

	    if (mask & PCI_MAPREG_TYPE_IO) {
		p->size = PCI_MAPREG_IO_SIZE(mask);

		/*
		 * Align IO if necessary
		 */
		if (p->size < PCI_MAPREG_IO_SIZE(PCI_MACHDEP_IO_ALIGN_MASK)) {
		    p->mask = PCI_MACHDEP_IO_ALIGN_MASK;
		    p->size = PCI_MAPREG_IO_SIZE(p->mask);
		}

		/*
		 * if I/O is already enabled (probably by the console driver)
		 * save the address in order to take care about it later.
		 */
		if (csr & PCI_COMMAND_IO_ENABLE)
		    p->address = address;

		insert_into_list(&iolist, p);
	    } else {
		p->size = PCI_MAPREG_MEM_SIZE(mask);

		/*
		 * Align memory if necessary
		 */
		if (p->size < PCI_MAPREG_IO_SIZE(PCI_MACHDEP_MEM_ALIGN_MASK)) {
		    p->mask = PCI_MACHDEP_MEM_ALIGN_MASK;
		    p->size = PCI_MAPREG_MEM_SIZE(p->mask);
		}

		/*
		 * if memory is already enabled (probably by the console driver)
		 * save the address in order to take care about it later.
		 */
		if (csr & PCI_COMMAND_MEM_ENABLE)
		    p->address = address;

		insert_into_list(&memlist, p);

		if (PCI_MAPREG_MEM_TYPE(mask) == PCI_MAPREG_MEM_TYPE_64BIT)
		    reg++;
	    }
	}

	/*
	 * Both interrupt pin & line are set to the device (== slot)
	 * number. This makes sense on the atari because the
	 * individual slots are hard-wired to a specific MFP-pin.
	 * XXX: This is _not_ true on the Milan.
	 */
	csr  = (DEV2SLOT(dev) << PCI_INTERRUPT_PIN_SHIFT);
	csr |= (DEV2SLOT(dev) << PCI_INTERRUPT_LINE_SHIFT);
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, csr);
    }

    /*
     * second step: calculate the memory and I/O adresses beginning from
     * PCI_MEM_START and PCI_IO_START. Care about already mapped areas.
     *
     * begin with memory list
     */

    address = PCI_MEM_START;
    sizecnt = 0;
    membase_1m = 0;
    p = LIST_FIRST(&memlist);
    while (p != NULL) {
	if (!(p->csr & PCI_COMMAND_MEM_ENABLE)) {
	    if (PCI_MAPREG_MEM_TYPE(p->mask) == PCI_MAPREG_MEM_TYPE_32BIT_1M) {
		if (p->size > membase_1m)
		    membase_1m = p->size;
		do {
		    p->address = membase_1m;
		    membase_1m += p->size;
		} while (overlap_pci_areas(LIST_FIRST(&memlist), p, p->address,
					   p->size, PCI_COMMAND_MEM_ENABLE));
		if (membase_1m > 0x00100000) {
		    /*
		     * Should we panic here?
		     */
		    printf("\npcibus0: dev %d reg %d: memory not configured",
			    p->dev, p->reg);
		    p->reg = 0;
		}
	    } else {

		if (sizecnt && (p->size > sizecnt))
		    sizecnt = ((p->size + sizecnt) & p->mask) &
			      PCI_MAPREG_MEM_ADDR_MASK;
		if (sizecnt > address) {
		    address = sizecnt;
		    sizecnt = 0;
		}

		do {
		    p->address = address + sizecnt;
		    sizecnt += p->size;
		} while (overlap_pci_areas(LIST_FIRST(&memlist), p, p->address,
					   p->size, PCI_COMMAND_MEM_ENABLE));

		if ((address + sizecnt) > PCI_MEM_END) {
		    /*
		     * Should we panic here?
		     */
		    printf("\npcibus0: dev %d reg %d: memory not configured",
			    p->dev, p->reg);
		    p->reg = 0;
		}
	    }
	    if (p->reg > 0) {
		pci_conf_write(pc, p->tag, p->reg, p->address);
		csr = pci_conf_read(pc, p->tag, PCI_COMMAND_STATUS_REG);
		csr |= PCI_COMMAND_MEM_ENABLE | PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, p->tag, PCI_COMMAND_STATUS_REG, csr);
		p->csr = csr;
	    }
	}
	p = LIST_NEXT(p, link);
    }

    /*
     * now the I/O list
     */

    address = PCI_IO_START;
    sizecnt = 0;
    p = LIST_FIRST(&iolist);
    while (p != NULL) {
	if (!(p->csr & PCI_COMMAND_IO_ENABLE)) {

	    if (sizecnt && (p->size > sizecnt))
		sizecnt = ((p->size + sizecnt) & p->mask) &
			  PCI_MAPREG_IO_ADDR_MASK;
	    if (sizecnt > address) {
		address = sizecnt;
		sizecnt = 0;
	    }

	    do {
		p->address = address + sizecnt;
		sizecnt += p->size;
	    } while (overlap_pci_areas(LIST_FIRST(&iolist), p, p->address,
				       p->size, PCI_COMMAND_IO_ENABLE));

	    if ((address + sizecnt) > PCI_IO_END) {
		/*
		 * Should we panic here?
		 */
		printf("\npcibus0: dev %d reg %d: io not configured",
			p->dev, p->reg);
	    } else {
		pci_conf_write(pc, p->tag, p->reg, p->address);
		csr = pci_conf_read(pc, p->tag, PCI_COMMAND_STATUS_REG);
		csr |= PCI_COMMAND_IO_ENABLE | PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, p->tag, PCI_COMMAND_STATUS_REG, csr);
		p->csr = csr;
	    }
	}
	p = LIST_NEXT(p, link);
    }

#ifdef DEBUG_PCI_MACHDEP
    printf("\nI/O List:\n");
    p = LIST_FIRST(&iolist);

    while (p != NULL) {
	printf("\ndev: %d, reg: 0x%02x, size: 0x%08x, addr: 0x%08x", p->dev,
			p->reg, p->size, p->address);
	p = LIST_NEXT(p, link);
    }
    printf("\nMemlist:");
    p = LIST_FIRST(&memlist);

    while (p != NULL) {
	printf("\ndev: %d, reg: 0x%02x, size: 0x%08x, addr: 0x%08x", p->dev,
			p->reg, p->size, p->address);
	p = LIST_NEXT(p, link);
    }
#endif

    /*
     * Free the lists
     */
    p = LIST_FIRST(&iolist);
    while (p != NULL) {
	q = p;
	LIST_REMOVE(q, link);
	free(p, M_WAITOK);
	p = LIST_FIRST(&iolist);
    }
    p = LIST_FIRST(&memlist);
    while (p != NULL) {
	q = p;
	LIST_REMOVE(q, link);
	free(p, M_WAITOK);
	p = LIST_FIRST(&memlist);
    }
}

pcitag_t
pci_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	return ((bus << 16) | (device << 11) | (function << 8));
}

int
pci_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
	int line = pa->pa_intrline;

	/*
	 * According to the PCI-spec, 255 means `unknown' or `no connection'.
	 * Interpret this as 'no interrupt assigned'.
	 */
	if (line == 255) {
		*ihp = -1;
		return 1;
	}

	/*
	 * Values are pretty useless on the Hades since all interrupt
	 * lines for a card are tied together and hardwired to a
	 * specific TT-MFP I/O port.
	 */
	*ihp = line;
	return 0;
}

const char *
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == -1)
		panic("pci_intr_string: bogus handle 0x%x\n", ih);

	sprintf(irqstr, "irq %d", ih);
	return (irqstr);
	
}

const struct evcnt *
pci_intr_evcnt(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}
