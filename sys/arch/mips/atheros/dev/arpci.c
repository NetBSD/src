/*	$NetBSD: arpci.c,v 1.2.12.1 2014/08/20 00:03:12 tls Exp $	*/
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: arpci.c,v 1.2.12.1 2014/08/20 00:03:12 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

#include <mips/atheros/include/arbusvar.h>
#include <mips/atheros/include/ar9344reg.h>

#define	PCI_CMD_CFG_READ	0xa
#define	PCI_CMD_CFG_WRITE	0xb

struct arpci_softc {
	device_t sc_dev;
	bus_dma_tag_t sc_dmat;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	struct mips_bus_space sc_memt;
	struct mips_pci_chipset sc_pc;
	bool sc_pcie;
	u_int sc_pba_flags;
};

static void arpci_bus_mem_init(bus_space_tag_t, void *);

static void
arpci_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

static int
arpci_bus_maxdevs(void *v, int busno)
{
	struct arpci_softc * const sc = v;

	if (busno == 0)
		return (sc->sc_pcie ? 1 : 22);

	return 32;
}

static pcitag_t
arpci_make_tag(void *v, int bus, int dev, int func)
{
	if (bus == 0 && dev == 0) {
		/*
		 * Local access
		 */
		return (func << 8);
	}

	if (bus == 0 && dev < 21) {
		/*
		 * Type 0 can only access 21 (32 - 11) devices starting at			 * device 0 (0 is needed for inbound transactions).
		 * AD[11:32] encodes the idsel for the transaction
		 *	(only one bit can be set).
		 * AD[8:11] contains function
		 * AD[2:7] contains the register offset.
		 * AD[0:1] must be zero.
		 */
		return (1 << (dev + 11)) | (func << 8);
	}

	/*
	 * Type 1 Confugration Transaction.
	 */
	return (bus << 16) | (dev << 11) | (func << 8) | 1;
}

static void
arpci_decompose_tag(void *v, pcitag_t tag, int *busp, int *devp, int *funcp)
{
	if (tag & 1) {
		if (busp)
			*busp = (tag >> 16) & 255;
		if (devp)
			*devp = (tag >> 11) & 31;
	} else {
		if (busp)
			*busp = 0;
		if (devp) {
			if (tag & ~0x7ff) {
				*devp = ffs(tag >> 11) - 1;
			} else {
				*devp = 0;
			}
		}
	}
	if (funcp)
		*funcp = (tag >> 8) & 7;
}

static pcireg_t
arpci_conf_read(void *v, pcitag_t tag, int reg)
{
	struct arpci_softc * const sc = v;
	pcireg_t rv = 0xffffffff;

	if ((tag & 0x00ff0001) == 1) {
		KASSERT(((tag >> 11) & 31) > 20); 
		/*
		 * This was a type 0 transaction for a device > 20 which
		 * we can't support.
		 */
		return rv;
	}

	tag |= reg & -4;

#if 0
	bus_space_read_4(sc->sc_bst, sc->sc_bsh, AR7100_PCI_ERROR);
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, AR7100_PCI_ERROR,
	    bus_space_read_4(sc->sc_bst, sc->sc_bsh, AR7100_PCI_ERROR) & 3);
#endif

	bus_space_handle_t addr = sc->sc_bsh;
	if ((tag & ~0x7fe) == 0) {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_LCL_CFG_CMD, AR7100_PCI_LCL_CFG_CMD_READ | tag);
		addr += AR7100_PCI_LCL_CFG_RDATA;
		printf("%s: tag %#lx: ", __func__, tag);
	} else {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_CFG_ADDR, tag);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_CFG_CMD, PCI_CMD_CFG_READ);
		addr += AR7100_PCI_CFG_RDATA;
		printf("%s: AD[0:31] 0x%08lx: ", __func__, tag);
	}

	rv = kfetch_32((void *)addr, 0xffffffff);
	printf("%#x\n", rv);

	return rv;
}

static void
arpci_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	struct arpci_softc * const sc = v;

	if ((tag & 0x00ff0001) == 1) {
		KASSERT(((tag >> 11) & 31) > 20); 
		/*
		 * This was a type 0 transaction for a device > 20 which
		 * we can't support.
		 */
		return;
	}

	tag |= reg & -4;

	if ((tag & ~0x7fe) == 0) {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_LCL_CFG_CMD, AR7100_PCI_LCL_CFG_CMD_WRITE | tag);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_LCL_CFG_WDATA, data);
	} else {
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_CFG_ADDR, tag);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_CFG_CMD, PCI_CMD_CFG_WRITE);
		bus_space_write_4(sc->sc_bst, sc->sc_bsh,
		    AR7100_PCI_CFG_WDATA, data);
	}
}

static int
arpci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	return EINVAL;
}

static const char *
arpci_intr_string(void *v, pci_intr_handle_t ih, char *buf, size_t len)
{
	snprintf(buf, len, "fixme!");
	return buf;
}

static const struct evcnt *
arpci_intr_evcnt(void *v, pci_intr_handle_t ih)
{
	return NULL;
}

static void *
arpci_intr_establish(void *v, pci_intr_handle_t ih,
	int ipl, int (*func)(void *), void *arg)
{
	return NULL;
}

static void
arpci_intr_disestablish(void *v, void *cookie)
{
}

static void
arpci_conf_interrupt(void *v, int bus, int dev, int func, int swiz, int *ilinep)
{
}

static void
arpci_chipset_init(struct arpci_softc *sc)
{
	pci_chipset_tag_t pc = &sc->sc_pc;

	pc->pc_conf_v =			sc;
	pc->pc_attach_hook =		arpci_attach_hook;
	pc->pc_bus_maxdevs =		arpci_bus_maxdevs;
	pc->pc_make_tag =		arpci_make_tag;
	pc->pc_decompose_tag =		arpci_decompose_tag;
	pc->pc_conf_read =		arpci_conf_read;
	pc->pc_conf_write =		arpci_conf_write;

	pc->pc_intr_v =			sc;
	pc->pc_intr_map =		arpci_intr_map;
	pc->pc_intr_string =		arpci_intr_string;
	pc->pc_intr_evcnt =		arpci_intr_evcnt;
	pc->pc_intr_establish =		arpci_intr_establish;
	pc->pc_intr_disestablish =	arpci_intr_disestablish;

	pc->pc_conf_interrupt =		arpci_conf_interrupt;

#ifdef __HAVE_PCIIDE_MACHDEP_COMPAT_INTR_ESTABLISH
	//pc->pc_pciide_compat_intr_establish = arpci_pciide_compat_intr_establish;
#endif
}

static int
arpci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct arbus_attach_args * const aa = aux;
	bus_space_handle_t bsh;

        if (strcmp(aa->aa_name, cf->cf_name) != 0)
		return 0;

	if (bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size, 0, &bsh))
		return 0;

	bus_space_unmap(aa->aa_bst, bsh, aa->aa_size);

	return 1;
}

static void
arpci_attach(device_t parent, device_t self, void *aux)
{
	struct arbus_attach_args * const aa = aux;
	struct arpci_softc * const sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_bst;
	sc->sc_dmat = aa->aa_dmat;
	sc->sc_pcie = (strcmp(device_cfdata(self)->cf_name, "arpcie") == 0);

	if (bus_space_map(aa->aa_bst, aa->aa_addr, aa->aa_size, 0,
		    &sc->sc_bsh)) {
		aprint_error(": failed to map registers\n");
		return;
	}

	aprint_normal(": PCI%s bus\n", (sc->sc_pcie ? "-Express x1" : ""));
	arpci_bus_mem_init(&sc->sc_memt, sc);
	arpci_chipset_init(sc);

	sc->sc_pba_flags |= PCI_FLAGS_MEM_OKAY;

	struct pcibus_attach_args pba;
	memset(&pba, 0, sizeof(pba));

	pba.pba_flags = sc->sc_pba_flags;
	if (pba.pba_flags & PCI_FLAGS_MEM_OKAY)
		pba.pba_memt = &sc->sc_memt;
	pba.pba_dmat = aa->aa_dmat;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_bus = 0;

	config_found_ia(self, "pcibus", &pba, pcibusprint);
}

CFATTACH_DECL_NEW(arpci, sizeof(struct arpci_softc),
    arpci_match, arpci_attach, NULL, NULL);
CFATTACH_DECL_NEW(arpcie, sizeof(struct arpci_softc),
    arpci_match, arpci_attach, NULL, NULL);

#define CHIP			arpci
#define CHIP_LITTLE_ENDIAN	/* defined */
#define CHIP_MEM		/* defined */      
#define CHIP_EXTENT		/* defined */      
#define	CHIP_EX_MALLOC_SAFE(v)	true
#define CHIP_W1_BUS_START(v)	0x10000000UL 
#define CHIP_W1_BUS_END(v)	0x16ffffffUL 
#define CHIP_W1_SYS_START(v)	CHIP_W1_BUS_START(v)    
#define CHIP_W1_SYS_END(v)	CHIP_W1_BUS_END(v)

#include <mips/mips/bus_space_alignstride_chipdep.c>
