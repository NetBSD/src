/*	$NetBSD: pci_machdep.c,v 1.10 1998/03/10 11:43:11 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1994 Charles Hannum.  All rights reserved.
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
 *	This product includes software developed by Charles Hannum.
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

#include <vm/vm.h>
#include <vm/vm_kern.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/mfp.h>
#include <machine/bus.h>

#include <atari/atari/device.h>

/*
 * I/O and memory we assume 'reserved' when an vga card is detected on
 * the PCI-bus.
 */
#define MAX_VGA_MEM	0x1000000	/* 16 MB mem	*/
#define MAX_VGA_IO	0x0010000	/* 64 KB io	*/

int	pcibusprint __P((void *auxp, const char *));
int	pcibusmatch __P((struct device *, struct cfdata *, void *));
void	pcibusattach __P((struct device *, struct device *, void *));

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

	pba.pba_busname = "pci";
	pba.pba_pc      = NULL;
	pba.pba_bus     = 0;
	pba.pba_iot     = PCI_IO_PHYS;
	pba.pba_memt    = PCI_MEM_PHYS;
	pba.pba_flags	= PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_dmat	= BUS_PCI_DMA_TAG;

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
 */
void
init_pci_bus()
{
	pci_chipset_tag_t	pc = NULL; /* XXX */
	pcitag_t		tag;
	pcireg_t		csr, address, mask;
	int			device, id, class, maxndevs;
	int			reg;
	u_int32_t		membase, iobase;

	tag        = 0;
	id = class = 0;
	
	membase = iobase = 0;

	maxndevs = pci_bus_maxdevs(pc, 0);

	/*
	 * Scan the bus for prehistory (usually VGA) devices.
	 */
	for (device = 0; device < maxndevs; device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;
		class = pci_conf_read(pc, tag, PCI_CLASS_REG);
		switch (PCI_CLASS(class)) {
			case PCI_CLASS_PREHISTORIC:
			case PCI_CLASS_DISPLAY:

				membase = MAX_VGA_MEM;
				iobase  = MAX_VGA_IO;
		}
	}

	for (device = 0; device < maxndevs; device++) {

		tag = pci_make_tag(pc, 0, device, 0);
		id  = pci_conf_read(pc, tag, PCI_ID_REG);
		if (id == 0 || id == 0xffffffff)
			continue;

		class = pci_conf_read(pc, tag, PCI_CLASS_REG);
		switch (PCI_CLASS(class)) {
			case PCI_CLASS_PREHISTORIC:
			case PCI_CLASS_DISPLAY:
				/*
				 * XXX: We rely on the BIOS to do the
				 * right thing here. Eventually, we should
				 * take the initiative...
				 */
				continue;
		}

		csr = pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);
		csr &= ~(PCI_COMMAND_MEM_ENABLE|PCI_COMMAND_IO_ENABLE);
		csr &= ~PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

		for (reg = PCI_MAPREG_START; reg < PCI_MAPREG_END; reg += 4) {
		    int	size, type;

		    address = pci_conf_read(pc, tag, reg);
		    pci_conf_write(pc, tag, reg, 0xffffffff);
		    mask    = pci_conf_read(pc, tag, reg);
		    pci_conf_write(pc, tag, reg, address);
		    if (mask == 0)
			continue; /* Register unused */

		    if (mask & PCI_MAPREG_TYPE_IO) {
			csr |= PCI_COMMAND_IO_ENABLE;
			address = PCI_MAPREG_IO_ADDR(mask);
			mask    = (~address << 1) | 1;
			size    = (mask & address) & 0xffffffff;
			address = iobase | PCI_MAPREG_TYPE_IO;
			iobase += roundup(size, 4096); /* XXX */
		    }
		    else {
			type = PCI_MAPREG_MEM_TYPE(address);
			switch (type) {
			    case PCI_MAPREG_MEM_TYPE_32BIT:
				break;
			    case PCI_MAPREG_MEM_TYPE_64BIT:
				reg++;
			    case PCI_MAPREG_MEM_TYPE_32BIT_1M:
				/*
				 * XXX: We can do better here!
				 */
				if (membase >= 0x100000)
					continue;
			}
			csr |= PCI_COMMAND_MEM_ENABLE;
			size = PCI_MAPREG_MEM_SIZE(mask);
			address = membase | PCI_MAPREG_TYPE_MEM;
			membase += roundup(size, 4096); /* XXX */
		    }
		    pci_conf_write(pc, tag, reg, address);
		}
		csr |= PCI_COMMAND_MASTER_ENABLE;
		pci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, csr);

		/*
		 * Both interrupt pin & line are set to the device (== slot)
		 * number. This makes sense on the atari because the
		 * individual slots are hard-wired to a specific MFP-pin.
		 */
		csr  = (device << PCI_INTERRUPT_PIN_SHIFT);
		csr |= (device << PCI_INTERRUPT_LINE_SHIFT);
		pci_conf_write(pc, tag, PCI_INTERRUPT_REG, csr);
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

	if ((sr & PSL_IPL) >= iinfo_p->ipl) {
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
