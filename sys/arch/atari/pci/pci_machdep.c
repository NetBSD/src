/*	$NetBSD: pci_machdep.c,v 1.18.6.1 1999/12/27 18:31:50 wrstuden Exp $	*/

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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/bswap.h>
#include <machine/bus.h>

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
					    I/O adresses up to 0xffff) */

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

int	pcibusprint __P((void *auxp, const char *));
int	pcibusmatch __P((struct device *, struct cfdata *, void *));
void	pcibusattach __P((struct device *, struct device *, void *));

static void enable_pci_devices __P((void));
static void insert_into_list __P((PCI_MEMREG *head, struct pci_memreg *elem));
static int overlap_pci_areas __P((struct pci_memreg *p,
	struct pci_memreg *self, u_int addr, u_int size, u_int what));
static int pci_config_offset __P((pcitag_t));

struct cfattach pcibus_ca = {
	sizeof(struct device), pcibusmatch, pcibusattach
};

int
pcibusmatch(pdp, cfp, auxp)
struct device	*pdp;
struct cfdata	*cfp;
void		*auxp;
{
	if(atari_realconfig == 0)
		return (0);
	if (strcmp((char *)auxp, "pcibus") || cfp->cf_unit != 0)
		return(0);
	return(machineid & ATARI_HADES ? 1 : 0);
}

void
pcibusattach(pdp, dp, auxp)
struct device	*pdp, *dp;
void		*auxp;
{
	struct pcibus_attach_args	pba;
	bus_space_tag_t			leb_alloc_bus_space_tag __P((void));


	enable_pci_devices();

	pba.pba_busname = "pci";
	pba.pba_pc      = NULL;
	pba.pba_bus     = 0;
	pba.pba_flags	= PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_dmat	= BUS_PCI_DMA_TAG;
	pba.pba_iot     = leb_alloc_bus_space_tag();
	pba.pba_memt    = leb_alloc_bus_space_tag();
	if ((pba.pba_iot == NULL) || (pba.pba_memt == NULL)) {
		printf("leb_alloc_bus_space_tag failed!\n");
		return;
	}
	pba.pba_iot->base  = PCI_IO_PHYS;
	pba.pba_memt->base = PCI_MEM_PHYS;

	MFP2->mf_aer &= ~(0x27); /* PCI interrupts: HIGH -> LOW */

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

	/*
	 * Scan the bus for a VGA-card that we support. If we find
	 * one, try to initialize it to a 'standard' text mode (80x25).
	 */
	check_for_vga();
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
	 */
	csr  = (DEV2SLOT(dev) << PCI_INTERRUPT_PIN_SHIFT);
	csr |= (DEV2SLOT(dev) << PCI_INTERRUPT_LINE_SHIFT);
	pci_conf_write(pc, tag, PCI_INTERRUPT_REG, csr);
    }

    /*
     * second step: calculate the memory and I/O adresses beginning from
     * PCI_MEM_START and PCI_IO_START. Care about already mapped areas.
     *
     * beginn with memory list
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

/*
 * Atari_init.c maps the config areas NBPG bytes apart....
 */
static int pci_config_offset(tag)
pcitag_t	tag;
{
	int	device;

	device = (tag >> 11) & 0x1f;
	return(device * NBPG);
}

int
pci_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{
	return (4);
}

pcitag_t
pci_make_tag(pc, bus, device, function)
	pci_chipset_tag_t pc;
	int bus, device, function;
{
	return ((bus << 16) | (device << 11) | (function << 8));
}

pcireg_t
pci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	u_long	data;

	data = *(u_long *)(pci_conf_addr + pci_config_offset(tag) + reg);
	return (bswap32(data));
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	*((u_long *)(pci_conf_addr + pci_config_offset(tag) + reg))
		= bswap32(data);
}

int
pci_intr_map(pc, intrtag, pin, line, ihp)
	pci_chipset_tag_t pc;
	pcitag_t intrtag;
	int pin, line;
	pci_intr_handle_t *ihp;
{
	/*
	 * According to the PCI-spec, 255 means `unknown' or `no connection'.
	 * Interpret this as 'no interrupt assigned'.
	 */
	if (line == 255) {
		*ihp = -1;
		return 1;
	}

	/*
	 * Values are pretty useless because the on the Hades all interrupt
	 * lines for a card are tied together and hardwired to the TT-MFP
	 * I/O port.
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

/*
 * The interrupt stuff is rather ugly. On the Hades, all interrupt lines
 * for a slot are wired together and connected to IO 0,1,2 or 5 (slots:
 * (0-3) on the TT-MFP. The Pci-config code initializes the irq. number
 * to the slot position.
 */
static pci_intr_info_t iinfo[4] = { { -1 }, { -1 }, { -1 }, { -1 } };

static int	iifun __P((int, int));

static int
iifun(slot, sr)
int	slot;
int	sr;
{
	pci_intr_info_t *iinfo_p;
	int		s;

	iinfo_p = &iinfo[slot];

	/*
	 * Disable the interrupts
	 */
	MFP2->mf_imrb  &= ~iinfo_p->imask;

	if ((sr & PSL_IPL) >= (iinfo_p->ipl & PSL_IPL)) {
		/*
		 * We're running at a too high priority now.
		 */
		add_sicallback((si_farg)iifun, (void*)slot, 0);
	}
	else {
		s = splx(iinfo_p->ipl);
		(void) (iinfo_p->ifunc)(iinfo_p->iarg);
		splx(s);

		/*
		 * Re-enable interrupts after handling
		 */
		MFP2->mf_imrb |= iinfo_p->imask;
	}
	return 1;
}

void *
pci_intr_establish(pc, ih, level, ih_fun, ih_arg)
	pci_chipset_tag_t	pc;
	pci_intr_handle_t	ih;
	int			level;
	int			(*ih_fun) __P((void *));
	void			*ih_arg;
{
	pci_intr_info_t *iinfo_p;
	struct intrhand	*ihand;
	int		slot;

	slot    = ih;
	iinfo_p = &iinfo[slot];

	if (iinfo_p->ipl > 0)
	    panic("pci_intr_establish: interrupt was already established\n");

	ihand = intr_establish((slot == 3) ? 23 : 16 + slot, USER_VEC, 0,
				(hw_ifun_t)iifun, (void *)slot);
	if (ihand != NULL) {
		iinfo_p->ipl   = level;
		iinfo_p->imask = (slot == 3) ? 0x80 : (0x01 << slot);
		iinfo_p->ifunc = ih_fun;
		iinfo_p->iarg  = ih_arg;
		iinfo_p->ihand = ihand;

		/*
		 * Enable (unmask) the interrupt
		 */
		MFP2->mf_imrb |= iinfo_p->imask;
		MFP2->mf_ierb |= iinfo_p->imask;
		return(iinfo_p);
	}
	return NULL;
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	pci_intr_info_t *iinfo_p = (pci_intr_info_t *)cookie;

	if (iinfo->ipl < 0)
	    panic("pci_intr_disestablish: interrupt was not established\n");

	MFP2->mf_imrb &= ~iinfo->imask;
	MFP2->mf_ierb &= ~iinfo->imask;
	(void) intr_disestablish(iinfo_p->ihand);
	iinfo_p->ipl = -1;
}
