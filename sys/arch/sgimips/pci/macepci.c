/*	$NetBSD: macepci.c,v 1.12 2003/07/15 03:35:54 lukem Exp $	*/

/*
 * Copyright (c) 2001 Christopher Sekiya
 * Copyright (c) 2000 Soren S. Jorvang
 * All rights reserved.
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: macepci.c,v 1.12 2003/07/15 03:35:54 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/vmparam.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <sgimips/dev/crimereg.h>

#include <sgimips/dev/macereg.h>
#include <sgimips/dev/macevar.h>

#include <sgimips/pci/macepcireg.h>
#include <sgimips/pci/pci_addr_fixup.h>

#define PCIBIOS_PRINTV(arg) \
        do { \
                        printf arg; \
        } while (0)
#define PCIBIOS_PRINTVN(n, arg) \
        do { \
                        printf arg; \
        } while (0)


#define PAGE_ALIGN(x)	(((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define MEG_ALIGN(x)	(((x) + 0x100000 - 1) & ~(0x100000 - 1))

#include "pci.h"

struct macepci_softc {
	struct device sc_dev;

	struct sgimips_pci_chipset sc_pc;
};

static int	macepci_match(struct device *, struct cfdata *, void *);
static void	macepci_attach(struct device *, struct device *, void *);
static int	macepci_print(void *, const char *);
pcireg_t	macepci_conf_read(pci_chipset_tag_t, pcitag_t, int);
void		macepci_conf_write(pci_chipset_tag_t, pcitag_t, int, pcireg_t);
int		macepci_intr(void *);

struct pciaddr pciaddr;

bus_addr_t pciaddr_ioaddr(u_int32_t val);

int pciaddr_do_resource_allocate(pci_chipset_tag_t pc, pcitag_t tag, int mapreg, void *ctx, int type, bus_addr_t *addr, bus_size_t size);

unsigned int ioaddr_base = 0x3000;
unsigned int memaddr_base = 0x80100000;

CFATTACH_DECL(macepci, sizeof(struct macepci_softc),
	macepci_match, macepci_attach, NULL, NULL);

static int
macepci_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
macepci_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct macepci_softc *sc = (struct macepci_softc *)self;
	pci_chipset_tag_t pc = &sc->sc_pc;
	struct mace_attach_args *maa = aux;
	struct pcibus_attach_args pba;
	u_int32_t control;
	pcitag_t devtag;
	int device, rev;

	rev = bus_space_read_4(maa->maa_st, maa->maa_sh, MACEPCI_REVISION);
	printf(": rev %d\n", rev);

	pc->pc_conf_read = macepci_conf_read;
	pc->pc_conf_write = macepci_conf_write;

        *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_ADDR) = 0;
        *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_ERROR_FLAGS) = 0;
        *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(MACE_PCI_CONTROL) = 0xff008500;
        *(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_HARDINT) = 0;
        *(volatile u_int64_t *)MIPS_PHYS_TO_KSEG1(CRIME_SOFTINT) = 0;

	/* Only fix up the PCI slot, leave SCSI 0 & 1 as is */
	for (device = 3; device < 4; device++) {
		const struct pci_quirkdata *qd;
		int function, nfuncs;
		pcireg_t bhlcr, id;

		devtag = pci_make_tag(0, 0, device, 0);
		id = pci_conf_read(pc, devtag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(id) == 0)
			continue;

		qd = pci_lookup_quirkdata(PCI_VENDOR(id), PCI_PRODUCT(id));
		bhlcr = pci_conf_read(pc, devtag, PCI_BHLC_REG);

		if (PCI_HDRTYPE_MULTIFN(bhlcr) ||
		    (qd != NULL &&
		     (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0))
			nfuncs = 8;
		else
			nfuncs = 1;

		for (function = 0; function < nfuncs; function++) {
			devtag = pci_make_tag(0, 0, device, function);
			id = pci_conf_read(pc, devtag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
				continue;
			/* Not invalid, but we've done this ~forever */
			if (PCI_VENDOR(id) == 0)
				continue;

			pciaddr_resource_manage(0, devtag, NULL, NULL);
		}
	}

	/*
	 * Enable all MACE PCI interrupts. They will be masked by
	 * the CRIME code.
	 */
	control = bus_space_read_4(maa->maa_st, maa->maa_sh, MACEPCI_CONTROL);
	control |= CONTROL_INT_MASK;
	bus_space_write_4(maa->maa_st, maa->maa_sh, MACEPCI_CONTROL, control);

#if NPCI > 0
	memset(&pba, 0, sizeof pba);
	pba.pba_busname = "pci";
/*XXX*/	pba.pba_iot = 4;
/*XXX*/	pba.pba_memt = 2;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_dmat64 = NULL;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	pba.pba_pc = pc;

#ifdef MACEPCI_IO_WAS_BUGGY
	if (rev == 0)
		pba.pba_flags &= ~PCI_FLAGS_IO_ENABLED;		/* Buggy? */
#endif

	mace_intr_establish(7, IPL_NONE, macepci_intr, sc);
	/*mace_intr_establish(maa->maa_intr, IPL_NONE, macepci_intr, sc);*/

	config_found(self, &pba, macepci_print);
#endif
}


static int
macepci_print(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pba = aux;

	if (pnp != 0)
		aprint_normal("%s at %s", pba->pba_busname, pnp);
	else
		aprint_normal(" bus %d", pba->pba_bus);

	/* Mega XXX */
	*(volatile u_int32_t *)0xb4000034 = 0;	/* prime timer */

	return UNCONF;
}

pcireg_t
macepci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;

	/* This should be handled by a real interrupt handler */
	if ((*(volatile u_int32_t *)0xbf080004 & ~0x00100000) != 6)
		panic("pcierr: %x %x", *(volatile u_int32_t *)0xbf080000,
		    *(volatile u_int32_t *)0xbf080004);

	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cf8) = tag | reg;
	data = *(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cfc);
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cf8) = 0;

	if (*(volatile u_int32_t *)0xbf080004 & 0xf0000000) {
		*(volatile u_int32_t *)0xbf080004 = 0;
		return (pcireg_t)-1;
	}

	return data;
}

void
macepci_conf_write(pc, tag, reg, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
	pcireg_t data;
{
	/* XXX O2 soren */
	if (tag == 0)
		return;

	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cf8) = tag | reg;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cfc) = data;
	*(volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cf8) = 0;
}


/*
 * Handle PCI error interrupts.
 */
int
macepci_intr(arg)
	void *arg;
{
	printf("macepci_intr!\n");
	return 0;
}

/* PCI Address fixup routines */

void
pciaddr_resource_manage(pc, tag, func, ctx)
        pci_chipset_tag_t pc;
        pcitag_t tag;
        pciaddr_resource_manage_func_t func;
        void                          *ctx;
{
        pcireg_t val, mask;
        bus_addr_t addr;
        bus_size_t size;
        int error, mapreg, type, reg_start, reg_end, width;

        val = macepci_conf_read(pc, tag, PCI_BHLC_REG);
        switch (PCI_HDRTYPE_TYPE(val)) {
        default:
                printf("WARNING: unknown PCI device header.");
                pciaddr.nbogus++;
                return;
        case 0: 
                reg_start = PCI_MAPREG_START;
                reg_end   = PCI_MAPREG_END;
                break;
        case 1: /* PCI-PCI bridge */
                reg_start = PCI_MAPREG_START;
                reg_end   = PCI_MAPREG_PPB_END;
                break;
        case 2: /* PCI-CardBus bridge */
                reg_start = PCI_MAPREG_START;
                reg_end   = PCI_MAPREG_PCB_END;
                break;
        }
        error = 0;
    
        for (mapreg = reg_start; mapreg < reg_end; mapreg += width) {
                /* inquire PCI device bus space requirement */
                val = macepci_conf_read(pc, tag, mapreg);
                macepci_conf_write(pc, tag, mapreg, ~0);

                mask = macepci_conf_read(pc, tag, mapreg);
                macepci_conf_write(pc, tag, mapreg, val);
        
                type = PCI_MAPREG_TYPE(val);
                width = 4;

                if (type == PCI_MAPREG_TYPE_MEM) {
                        size = PCI_MAPREG_MEM_SIZE(mask);

			/* 
 			 * XXXrkb: for MEM64 BARs, to be totally kosher
			 * about the requested size, need to read mask
			 * from top 32bits of BAR and stir that into the
			 * size calculation, like so:
			 * 
			 * case PCI_MAPREG_MEM_TYPE_64BIT:
			 *	bar64 = pci_conf_read(pb->pc, tag, br + 4);
			 *	pci_conf_write(pb->pc, tag, br + 4, 0xffffffff);
			 *	mask64 = pci_conf_read(pb->pc, tag, br + 4);
			 *	pci_conf_write(pb->pc, tag, br + 4, bar64);
			 *	size = (u_int64_t) PCI_MAPREG_MEM64_SIZE(
			 *	      (((u_int64_t) mask64) << 32) | mask);
			 *	width = 8;
			 * 
			 * Fortunately, anything with all-zeros mask in the
			 * lower 32-bits will have size no less than 1 << 32,
			 * which we're not prepared to deal with, so I don't
			 * feel bad punting on it...
			 */
                        if (PCI_MAPREG_MEM_TYPE(val) == 
                            PCI_MAPREG_MEM_TYPE_64BIT) {
				/* 
				 * XXX We could examine the upper 32 bits
				 * XXX of the BAR here, but we are totally 
				 * XXX unprepared to handle a non-zero value,
				 * XXX either here or anywhere else in the
				 * XXX sgimips code (not sure about MI code).
				 * XXX
				 * XXX So just arrange to skip the top 32
				 * XXX bits of the BAR and zero then out 
				 * XXX if the BAR is in use.
				 */
				width = 8;

				if (size != 0)
					macepci_conf_write(pc, tag, 
							   mapreg + 4, 0);
                        }
                } else {
                        /*
                         * Upper 16 bits must be one.  Devices may hardwire
                         * them to zero, though, per PCI 2.2, 6.2.5.1, p 203.
                         */
                        mask |= 0xffff0000;
                        size = PCI_MAPREG_IO_SIZE(mask);
                }

                if (size == 0) /* unused register */
                        continue;

                addr = pciaddr_ioaddr(val);
        
                /* reservation/allocation phase */
                error += pciaddr_do_resource_allocate (pc, tag, mapreg, 
						       ctx, type, &addr, size);

/*                PCIBIOS_PRINTV(("\n\t%02xh %s 0x%08x 0x%08x", 
                                mapreg, type ? "port" : "mem ", 
                                (unsigned int)addr, (unsigned int)size)); */
        }
    
        /* enable/disable PCI device */
        val = macepci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG);   

        if (error == 0)
                val |= (PCI_COMMAND_IO_ENABLE | 
			PCI_COMMAND_MEM_ENABLE |
                        PCI_COMMAND_MASTER_ENABLE | 
			PCI_COMMAND_SPECIAL_ENABLE |
			PCI_COMMAND_INVALIDATE_ENABLE | 
			PCI_COMMAND_PARITY_ENABLE);
        else
                val &= ~(PCI_COMMAND_IO_ENABLE | 
			 PCI_COMMAND_MEM_ENABLE |
                         PCI_COMMAND_MASTER_ENABLE);

        macepci_conf_write(pc, tag, PCI_COMMAND_STATUS_REG, val);
    
        if (error)
                pciaddr.nbogus++;

}

bus_addr_t
pciaddr_ioaddr(val)
        u_int32_t val;
{
        return ((PCI_MAPREG_TYPE(val) == PCI_MAPREG_TYPE_MEM)
                ? PCI_MAPREG_MEM_ADDR(val)
                : PCI_MAPREG_IO_ADDR(val));
}

int
pciaddr_do_resource_allocate(pc, tag, mapreg, ctx, type, addr, size)
        pci_chipset_tag_t pc;
        pcitag_t tag;
        void       *ctx;
        int mapreg, type;
        bus_addr_t *addr;
        bus_size_t size;
{
	switch (type) {
	case PCI_MAPREG_TYPE_IO:
		*addr = ioaddr_base;
		ioaddr_base += PAGE_ALIGN(size);
		break;

	case PCI_MAPREG_TYPE_MEM:
		*addr = memaddr_base;
		memaddr_base += MEG_ALIGN(size);
		break;

	default:
		PCIBIOS_PRINTV(("attempt to remap unknown region (addr 0x%lx, "
				"size 0x%lx, type %d)\n", *addr, size, type));
		return 0;
	}


        /* write new address to PCI device configuration header */
        macepci_conf_write(pc, tag, mapreg, *addr);

        /* check */
#ifdef PCIBIOSVERBOSE
        if (!pcibiosverbose)
#endif 
        {
                printf("pci_addr_fixup: ");
                pciaddr_print_devid(pc, tag);
        }
        if (pciaddr_ioaddr(macepci_conf_read(pc, tag, mapreg)) != *addr) {
                macepci_conf_write(pc, tag, mapreg, 0); /* clear */
                printf("fixup failed. (new address=%#x)\n", (unsigned)*addr);
                return (1);
        }
#ifdef PCIBIOSVERBOSE
        if (!pcibiosverbose)
#endif
                printf("new address 0x%08x (size 0x%x)\n", (unsigned)*addr, 
							   (unsigned)size);

        return (0);
}

void
pciaddr_print_devid(pc, tag)
        pci_chipset_tag_t pc;
        pcitag_t tag;
{
        int bus, device, function;      
        pcireg_t id;
        
        id = macepci_conf_read(pc, tag, PCI_ID_REG);
        pci_decompose_tag(pc, tag, &bus, &device, &function);
        printf("%03d:%02d:%d 0x%04x 0x%04x ", bus, device, function, 
               PCI_VENDOR(id), PCI_PRODUCT(id));
}

