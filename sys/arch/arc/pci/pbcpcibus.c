/*	$NetBSD: pbcpcibus.c,v 1.5.2.3 2001/01/18 09:22:12 bouyer Exp $	*/
/*	$OpenBSD: pbcpcibus.c,v 1.7 1998/03/25 11:52:48 pefo Exp $ */

/*
 * Copyright (c) 1997, 1998 Per Fogelstrom, Opsycon AB
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
 *	This product includes software developed under OpenBSD by
 *	Per Fogelstrom, Opsycon AB.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/*
 * ARC PCI BUS Bridge driver.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/proc.h>

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <arc/arc/arctype.h>
#include <arc/algor/algor.h>
#include <arc/pci/pcibrvar.h>
#include <arc/pci/v962pcbreg.h>

extern vm_map_t phys_map;
extern char eth_hw_addr[];	/* Hardware ethernet address stored elsewhere */

int	 pbcpcibrmatch __P((struct device *, struct cfdata *, void *));
void	 pbcpcibrattach __P((struct device *, struct device *, void *));

void	 pbc_attach_hook __P((struct device *, struct device *,
				struct pcibus_attach_args *));
int	 pbc_bus_maxdevs __P((pci_chipset_tag_t, int));
pcitag_t pbc_make_tag __P((pci_chipset_tag_t, int, int, int));
void	 pbc_decompose_tag __P((pci_chipset_tag_t, pcitag_t, int *, int *,
	    int *));
pcireg_t pbc_conf_read __P((pci_chipset_tag_t, pcitag_t, int));
void	 pbc_conf_write __P((pci_chipset_tag_t, pcitag_t, int, pcireg_t));

int      pbc_intr_map __P((struct pci_attach_args *, pci_intr_handle_t *));
const char *pbc_intr_string __P((pci_chipset_tag_t, pci_intr_handle_t));
void     *pbc_intr_establish __P((pci_chipset_tag_t, pci_intr_handle_t,
	    int, int (*)(void *), void *));
void     pbc_intr_disestablish __P((pci_chipset_tag_t, void *));

struct cfattach pbcpcibr_ca = {
        sizeof(struct pcibr_softc), pbcpcibrmatch, pbcpcibrattach,
};

extern struct cfdriver pbcpcibr_cd;

static int      pbcpcibrprint __P((void *, const char *pnp));

static int pbc_version;

#ifdef __OpenBSD__
/*
 * Code from "pci/if_de.c" used to calculate crc32 of ether rom data.
 * Another example can be found in document EC-QPQWA-TE from DEC.
 */
#define	TULIP_CRC32_POLY	0xEDB88320UL
static __inline__ unsigned
srom_crc32(const unsigned char *databuf, size_t datalen)
{
	u_int idx, bit, data, crc = 0xFFFFFFFFUL;

	for (idx = 0; idx < datalen; idx++) {
		for (data = *databuf++, bit = 0; bit < 8; bit++, data >>= 1) {
			crc = (crc >> 1) ^
			    (((crc ^ data) & 1) ? TULIP_CRC32_POLY : 0);
		}
	}
	return (crc);
}
#endif


int
pbcpcibrmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct confargs *ca = aux;

	/* Make sure that we're looking for a PCI bridge. */
	if (strcmp(ca->ca_name, pbcpcibr_cd.cd_name) != 0)
		return (0);

	return (1);
}

void
pbcpcibrattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct pcibr_softc *sc = (struct pcibr_softc *)self;
	struct pcibus_attach_args pba;

	switch(cputype) {
	case ALGOR_P4032:
	case ALGOR_P5064:
		V96X_PCI_BASE0 = V96X_PCI_BASE0 & 0xffff0000;
		pbc_version = V96X_PCI_CC_REV;

		arc_bus_space_init(&sc->sc_bus_space, "pbcpci",
		    0LL, MIPS_KSEG1_START,
		    V96X_PCI_MEM_SPACE, MIPS_KSEG2_START - MIPS_KSEG1_START);

		_bus_dma_tag_init(&sc->sc_dmat);
		if (pbc_version < V96X_VREV_C0) {
			/* XXX - Is this OK? */
			/* BUG in early V962PBC's: Use aparture II */
			sc->sc_dmat.dma_offset = 0xc0000000;
		}

		sc->sc_pc.pc_attach_hook = pbc_attach_hook;
		sc->sc_pc.pc_bus_maxdevs = pbc_bus_maxdevs;
		sc->sc_pc.pc_make_tag = pbc_make_tag;
		sc->sc_pc.pc_conf_read = pbc_conf_read;
		sc->sc_pc.pc_conf_write = pbc_conf_write;
		sc->sc_pc.pc_intr_map = pbc_intr_map;
		sc->sc_pc.pc_intr_string = pbc_intr_string;
		sc->sc_pc.pc_intr_establish = pbc_intr_establish;
		sc->sc_pc.pc_intr_disestablish = pbc_intr_disestablish;

		printf(": V3 V962, Revision %x.\n", pbc_version);
		break;
	}

	pba.pba_busname = "pci";
	pba.pba_iot = &sc->sc_bus_space;
	pba.pba_memt = &sc->sc_bus_space;
	pba.pba_dmat = &sc->sc_dmat;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED;
	pba.pba_bus = 0;
	config_found(self, &pba, pbcpcibrprint);

}

static int
pbcpcibrprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct pcibus_attach_args *pba = aux;

	if(pnp)
		printf("%s at %s", pba->pba_busname, pnp);
	printf(" bus %d", pba->pba_bus);
	return(UNCONF);
}

#ifdef __OpenBSD__
/*
 *  Get PCI physical address from given viritual address.
 */

paddr_t
vtophysaddr(dp, va)
	struct device *dp;
	vaddr_t va;
{
	paddr_t pa;

	if(va >= UADDR) {	/* Stupid driver have buf on stack!! */
		va = (vaddr_t)curproc->p_addr + (va & ~UADDR);
	}
	if(va < VM_MIN_KERNEL_ADDRESS) {
		pa = MIPS_KSEG0_TO_PHYS(va);
	}
	else if (!pmap_extract(vm_map_pmap(phys_map), va, &pa)) {
		panic("pbcpcibus.c:vtophysaddr(): pmap_extract %p", va);
	}
	if(dp->dv_class == DV_IFNET && pbc_version < V96X_VREV_C0) { 
					/* BUG in early V962PBC's */
		pa |= 0xc0000000;	/* Use aparture II */
	}
	return(pa);
}
#endif

void
pbc_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
}

int
pbc_bus_maxdevs(pc, busno)
	pci_chipset_tag_t pc;
	int busno;
{
	return(16);
}

pcitag_t
pbc_make_tag(pc, bus, dev, fnc)
	pci_chipset_tag_t pc;
	int bus, dev, fnc;
{
	return (bus << 16) | (dev << 11) | (fnc << 8);
}

void
pbc_decompose_tag(pc, tag, busp, devp, fncp)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int *busp, *devp, *fncp;
{
	if (busp != NULL)
		*busp = (tag >> 16) & 0x7;
	if (devp != NULL)
		*devp = (tag >> 11) & 0x1f;
	if (fncp != NULL)
		*fncp = (tag >> 8) & 0x7;
}

pcireg_t
pbc_conf_read(pc, tag, offset)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int offset;
{
	pcireg_t data;
	u_int32_t addr;
	int bus, device, func, ad_low;
	int s;

#ifdef __GNUC__
	addr = 0;		/* XXX: shut up gcc warning */
#endif

	if(offset & 3 || offset < 0 || offset >= 0x100) {
		printf ("pci_conf_read: bad reg %x\n", offset);
		return(~0);
	}
	pbc_decompose_tag(pc, tag, &bus, &device, &func);
	ad_low = 0;

	switch (cputype) {
	case ALGOR_P4032:
		if(bus != 0 || device > 5 || func > 7) {
			return(~0);
		}
		addr = (0x800 << device) | (func << 8) | offset;
		ad_low = 0;
		break;

	case ALGOR_P5064:
		if(bus == 0) {
			if(device > 5 || func > 7) {
				return(~0);
			}
			addr = (1L << (device + 24)) | (func << 8) | offset;
			ad_low = 0;
		}
		else if(pbc_version >= V96X_VREV_C0) {
			if(bus > 255 || device > 15 || func > 7) {
				return(~0);
			}
			addr = (bus << 16) | (device << 11) | (func << 8);
			ad_low = V96X_LB_MAPx_AD_LOW_EN;
		}
		else {
			return(~0);
		}
		break;
	}

	s = splhigh();

	/* high 12 bits of address go in map register, and set for conf space */
	V96X_LB_MAP0 = ((addr >> 16) & V96X_LB_MAPx_MAP_ADR) | ad_low | V96X_LB_TYPE_CONF;
	/* clear aborts */
	V96X_PCI_STAT |= V96X_PCI_STAT_M_ABORT | V96X_PCI_STAT_T_ABORT;

	wbflush();

	/* low 20 bits of address are in the actual address */
	data = *(volatile pcireg_t *) (V96X_PCI_CONF_SPACE + (addr&0xfffff));

	if (V96X_PCI_STAT & V96X_PCI_STAT_M_ABORT) {
		V96X_PCI_STAT |= V96X_PCI_STAT_M_ABORT;
		return(~0);	/* Nothing there */
	}

	if (V96X_PCI_STAT & V96X_PCI_STAT_T_ABORT) {
		V96X_PCI_STAT |= V96X_PCI_STAT_T_ABORT;
		printf ("PCI slot %d: target abort!\n", device);
		return(~0);	/* Ooops! */
	}

	splx(s);
	return(data);
}

void
pbc_conf_write(pc, tag, offset, data)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int offset;
	pcireg_t data;
{
	u_int32_t addr;
	int bus, device, func, ad_low;
	int s;

#ifdef __GNUC__
	addr = 0;		/* XXX: shut up gcc warning */
#endif
	pbc_decompose_tag(pc, tag, &bus, &device, &func);
	ad_low = 0;

	switch (cputype) {
	case ALGOR_P4032:
		if(bus != 0 || device > 5 || func > 7) {
			return;
		}
		addr = (0x800 << device) | (func << 8) | offset;
		ad_low = 0;
		break;

	case ALGOR_P5064:
		if(bus == 0) {
			if(device > 5 || func > 7) {
				return;
			}
			addr = (1L << (device + 24)) | (func << 8) | offset;
			ad_low = 0;
		}
		else if(pbc_version >= V96X_VREV_C0) {
			if(bus > 255 || device > 15 || func > 7) {
				return;
			}
			addr = (bus << 16) | (device << 11) | (func << 8);
			ad_low = V96X_LB_MAPx_AD_LOW_EN;
		}
		else {
			return;
		}
		break;
	}
  
	s = splhigh();

	/* high 12 bits of address go in map register, and set for conf space */
	V96X_LB_MAP0 = ((addr >> 16) & V96X_LB_MAPx_MAP_ADR) | ad_low | V96X_LB_TYPE_CONF;
	/* clear aborts */
	V96X_PCI_STAT |= V96X_PCI_STAT_M_ABORT | V96X_PCI_STAT_T_ABORT;

	wbflush();

	/* low 20 bits of address are in the actual address */
	*(volatile pcireg_t *) (V96X_PCI_CONF_SPACE + (addr&0xfffff)) = data;

	/* wait for write FIFO to empty */
	do {
	} while (V96X_FIFO_STAT & V96X_FIFO_STAT_L2P_WR);

	if (V96X_PCI_STAT & V96X_PCI_STAT_M_ABORT) {
		V96X_PCI_STAT |= V96X_PCI_STAT_M_ABORT;
		printf ("PCI slot %d: conf_write: master abort\n", device);
	}

	if (V96X_PCI_STAT & V96X_PCI_STAT_T_ABORT) {
		V96X_PCI_STAT |= V96X_PCI_STAT_T_ABORT;
		printf ("PCI slot %d: conf_write: target abort!\n", device);
	}

	splx(s);
}

#ifdef __OpenBSD__
/*
 *	Build the serial rom info normaly stored in an EEROM on
 *	PCI DEC21x4x boards. Cheapo designs skips the rom so
 *	we do the job here. The setup is not 100% correct but
 *	close enough to make the driver happy!
 */
int
pbc_ether_hw_addr(p)
	u_int8_t *p;
{
	int i;

	for(i = 0; i < 128; i++)
		p[i] = 0x00;
	p[18] = 0x03;	/* Srom version. */
	p[19] = 0x01;	/* One chip. */
	/* Next six, ethernet address. */
	bcopy(eth_hw_addr, &p[20], 6);

	p[26] = 0x00;	/* Chip 0 device number */
	p[27] = 30;		/* Descriptor offset */
	p[28] = 00;
	p[29] = 00;		/* MBZ */
					/* Descriptor */
	p[30] = 0x00;	/* Autosense. */
	p[31] = 0x08;
	switch (cputype) {
	case ALGOR_P4032:
	case ALGOR_P5064:
		p[32] = 0x01;	/* Block cnt */
		p[33] = 0x02;	/* Medium type is AUI */
		break;

	default:
		p[32] = 0xff;	/* GP cntrl */
		p[33] = 0x01;	/* Block cnt */
#define GPR_LEN 0
#define	RES_LEN 0
		p[34] = 0x80 + 12 + GPR_LEN + RES_LEN;
		p[35] = 0x01;	/* MII PHY type */
		p[36] = 0x00;	/* PHY number 0 */
		p[37] = 0x00;	/* GPR Length */
		p[38] = 0x00;	/* Reset Length */
		p[39] = 0x00;	/* Media capabilities */
		p[40] = 0x78;	/* Media capabilities */
		p[41] = 0x00;	/* Autoneg advertisment */
		p[42] = 0x78;	/* Autoneg advertisment */
		p[43] = 0x00;	/* Full duplex map */
		p[44] = 0x50;	/* Full duplex map */
		p[45] = 0x00;	/* Treshold map */
		p[46] = 0x18;	/* Treshold map */
		break;
	}

	i = (srom_crc32(p, 126) & 0xFFFF) ^ 0xFFFF;
	p[126] = i;
	p[127] = i >> 8;
	return(1);	/* Got it! */
}
#endif

int
pbc_intr_map(pa, ihp)
	struct pci_attach_args *pa;
	pci_intr_handle_t *ihp;
{
	pci_chipset_tag_t pc = pa->pa_pc;
	pcitag_t intrtag = pa->pa_intrtag;
	int buspin = pa->pa_intrpin;
	int device, pirq;

        if (buspin == 0) {
                /* No IRQ used. */
		*ihp = -1;
                return 1;
        }
        if (buspin > 4) {
                printf("pbc_intr_map: bad interrupt pin %d\n", buspin);
		*ihp = -1;
                return 1;
        }

	pbc_decompose_tag(pc, intrtag, NULL, &device, NULL);
	pirq = buspin - 1;

	switch(device) {
	case 0:				/* DC21041 */
		pirq = 9;
		break;
	case 1:				/* NCR SCSI */
		pirq = 10;
		break;
	default:
		switch (buspin) {
		case PCI_INTERRUPT_PIN_A:
			pirq = 0;
			break;
		case PCI_INTERRUPT_PIN_B:
			pirq = 1;
			break;
		case PCI_INTERRUPT_PIN_C:
			pirq = 2;
			break;
		case PCI_INTERRUPT_PIN_D:
			pirq = 3;
			break;
		}
	}
	*ihp = pirq;
	return 0;
}

const char *
pbc_intr_string(pc, ih)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
{
	static char str[16];

	sprintf(str, "pciirq%ld", ih);
	return(str);
}

void *
pbc_intr_establish(pc, ih, level, func, arg)
	pci_chipset_tag_t pc;
	pci_intr_handle_t ih;
	int level;
	int (*func) __P((void *));
	void *arg;
{
	return algor_pci_intr_establish(ih, level, func, arg);
}

void
pbc_intr_disestablish(pc, cookie)
	pci_chipset_tag_t pc;
	void *cookie;
{
	algor_pci_intr_disestablish(cookie);
}
