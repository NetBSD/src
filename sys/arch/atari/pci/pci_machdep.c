/*	$NetBSD: pci_machdep.c,v 1.7 1997/04/10 23:12:20 cgd Exp $	*/

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
#include <atari/atari/device.h>

int	pcibusprint __P((void *auxp, const char *));
int	pcibusmatch __P((struct device *, struct cfdata *, void *));
void	pcibusattach __P((struct device *, struct device *, void *));

static int pci_config_offset __P((pcitag_t));

/*
 * Swap a long (abcd -> dcba). The pci-config area has the wrong
 * endianess....
 */
#define swapl(x)	\
	{ asm volatile ("rorw #8,%0\nswap %0\nrorw #8,%0" : : "r" (x)); }

struct cfattach pcibus_ca = {
	sizeof(struct device), pcibusmatch, pcibusattach
};

struct cfdriver pcibus_cd = {
	NULL, "pcibus", DV_DULL
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
	pba.pba_flags	= PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;

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
	swapl(data);
	return(data);
}

void
pci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	swapl(data);
	*((u_long *)(pci_conf_addr + pci_config_offset(tag) + reg)) = data;
}

/*
 * XXX: All pci_intr_*() functions are no-op's for now...
 */
int
pci_intr_map(pc, intrtag, pin, line, ihp)
	pci_chipset_tag_t pc;
	pcitag_t intrtag;
	int pin, line;
	pci_intr_handle_t *ihp;
{
	/* XXXXXXXXX */
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char irqstr[8];		/* 4 + 2 + NULL + sanity */

	if (ih == 0)
		panic("pci_intr_string: bogus handle 0x%x\n", ih);

	sprintf(irqstr, "irq %d", ih);
	return (irqstr);
	
}

void *
pci_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level, (*func) __P((void *));
	void *arg;
{
	/* XXXX */
	return NULL;
}

void
pci_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	/* XXXX */
}
