/*	$NetBSD: macepci.c,v 1.2 2000/06/14 16:32:22 soren Exp $	*/

/*
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/locore.h>
#include <machine/autoconf.h>
#include <machine/bus.h>

#include <dev/pci/pcivar.h>

#include <sgimips/dev/macereg.h>
#include <sgimips/dev/macevar.h>

#include <sgimips/pci/macepcireg.h>

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

struct cfattach macepci_ca = {
	sizeof(struct macepci_softc), macepci_match, macepci_attach
};

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
	pcitag_t devtag;
	int rev;

	rev = bus_space_read_4(maa->maa_st, maa->maa_sh, PCI_REV_INFO_R);
	printf(": rev %d\n", rev);

#if 0
	mace_intr_establish(maa->maa_intr, IPL_NONE , macepci_intr, sc);
#endif

	pc->pc_conf_read = macepci_conf_read;
	pc->pc_conf_write = macepci_conf_write;

	/*
	 * Fixup O2 PCI slot.
	 */
	devtag = pci_make_tag(0, 0, 3, 0);
#if 1
	macepci_conf_write(0, devtag, 0x4, 0x00000007);
#endif
#if 1	/* 1 for canuck */
	macepci_conf_write(0, devtag, 0x10, 0x00001000);
#endif
#if 0	/* 1 for hut */
	macepci_conf_write(0, devtag, 0x18, 0x00000000);
#endif

#if 1
	printf("macepci0: ctrl %x\n", *(volatile u_int32_t *)0xbf080008);
	*(volatile u_int32_t *)0xbf080008 |= 0x000000ff;
	printf("macepci0: ctrl %x\n", *(volatile u_int32_t *)0xbf080008);
#endif

#if NPCI > 0
	memset(&pba, 0, sizeof pba);
	pba.pba_busname = "pci";
/*XXX*/	pba.pba_iot = 4;
/*XXX*/	pba.pba_memt = 2;
	pba.pba_dmat = &pci_bus_dma_tag;
	pba.pba_bus = 0;
	pba.pba_flags = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	pba.pba_pc = pc;

	if (rev == 0)
		pba.pba_flags &= ~PCI_FLAGS_IO_ENABLED;		/* Buggy? */

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
                printf("%s at %s", pba->pba_busname, pnp);
	else
		printf(" bus %d", pba->pba_bus);

	return UNCONF;
}

#define PCI_CFG_ADDR	((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cf8))
#define PCI_CFG_DATA	((volatile u_int32_t *)MIPS_PHYS_TO_KSEG1(0x1f080cfc))

pcireg_t
macepci_conf_read(pc, tag, reg)
	pci_chipset_tag_t pc;
	pcitag_t tag;
	int reg;
{
	pcireg_t data;

	/* XXX more generic pci error checking */
#if 1
	if ((*(volatile u_int32_t *)0xbf080004 & ~0x00100000) != 6)
		panic("pcierr: %x %x", *(volatile u_int32_t *)0xbf080004,
		    *(volatile u_int32_t *)0xbf080000);
#endif
	*PCI_CFG_ADDR = 0x80000000 | tag | reg;
	data = *PCI_CFG_DATA;
	*PCI_CFG_ADDR = 0;

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

	*PCI_CFG_ADDR = 0x80000000 | tag | reg;
	*PCI_CFG_DATA = data;
	*PCI_CFG_ADDR = 0;
}


/*
 * Handle PCI error interrupts.
 */
int
macepci_intr(arg)
	void *arg;
{

	return 0;
}
