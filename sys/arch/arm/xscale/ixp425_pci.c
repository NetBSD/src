/*	$NetBSD: ixp425_pci.c,v 1.12 2015/10/02 05:22:50 msaitoh Exp $ */

/*
 * Copyright (c) 2003
 *	Ichiro FUKUHARA <ichiro@ichiro.org>.
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
 *
 * THIS SOFTWARE IS PROVIDED BY ICHIRO FUKUHARA ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ICHIRO FUKUHARA OR THE VOICES IN HIS HEAD BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ixp425_pci.c,v 1.12 2015/10/02 05:22:50 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>

#include <arm/xscale/ixp425reg.h>
#include <arm/xscale/ixp425var.h>

#include <evbarm/ixdp425/ixdp425reg.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include "opt_pci.h"
#include "pci.h"

void	ixp425_pci_attach_hook(device_t, device_t,
	    struct pcibus_attach_args *);
int	ixp425_pci_bus_maxdevs(void *, int);
void	ixp425_pci_decompose_tag(void *, pcitag_t, int *, int *, int *);
void	ixp425_pci_conf_setup(void *, struct ixp425_softc *, pcitag_t, int);
void	ixp425_pci_conf_write(void *, pcitag_t, int, pcireg_t);
void	ixp425_pci_conf_interrupt(void *, int, int, int, int, int *);
pcitag_t ixp425_pci_make_tag(void *, int, int, int);
pcireg_t ixp425_pci_conf_read(void *, pcitag_t, int);

#define	MAX_PCI_DEVICES	32

void
ixp425_pci_init(struct ixp425_softc *sc)
{
	pci_chipset_tag_t pc = &sc->ia_pci_chipset;
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct extent *ioext, *memext;
#endif
	/*
	 * Initialise the PCI chipset tag
	 */
	pc->pc_conf_v = sc;
	pc->pc_attach_hook = ixp425_pci_attach_hook;
	pc->pc_bus_maxdevs = ixp425_pci_bus_maxdevs;
	pc->pc_make_tag = ixp425_pci_make_tag;
	pc->pc_decompose_tag = ixp425_pci_decompose_tag;
	pc->pc_conf_read = ixp425_pci_conf_read;
	pc->pc_conf_write = ixp425_pci_conf_write;
	pc->pc_conf_interrupt = ixp425_pci_conf_interrupt;

	/*
	 * Initialize the bus space tags.
	 */
	ixp425_io_bs_init(&sc->sc_pci_iot, sc);
	ixp425_mem_bs_init(&sc->sc_pci_memt, sc);

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	ioext  = extent_create("pciio", 0, IXP425_PCI_IO_SIZE - 1,
				NULL, 0, EX_NOWAIT);
	/* PCI MEM space is mapped same address as real memory */
	memext = extent_create("pcimem", IXP425_PCI_MEM_HWBASE,
				IXP425_PCI_MEM_HWBASE +
				IXP425_PCI_MEM_SIZE - 1,
				NULL, 0, EX_NOWAIT);
	aprint_normal_dev(sc->sc_dev, "configuring PCI bus\n");
	pci_configure_bus(pc, ioext, memext, NULL, 0 /* XXX bus = 0 */,
			  arm_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

void
ixp425_pci_conf_interrupt(void *v, int a, int b, int c, int d, int *p)
{
}

void
ixp425_pci_attach_hook(device_t parent, device_t self,
	struct pcibus_attach_args *pba)
{
	/* Nothing to do. */
}

int
ixp425_pci_bus_maxdevs(void *v, int busno)
{
	return(MAX_PCI_DEVICES);
}

pcitag_t
ixp425_pci_make_tag(void *v, int bus, int device, int function)
{
#ifdef PCI_DEBUG
	printf("ixp425_pci_make_tag(v=%p, bus=%d, device=%d, function=%d)\n",
		v, bus, device, function);
#endif
	return ((bus << 16) | (device << 11) | (function << 8));
}

void
ixp425_pci_decompose_tag(void *v, pcitag_t tag, int *busp, int *devicep,
	int *functionp)
{
#ifdef PCI_DEBUG
	printf("ixp425_pci_decompose_tag(v=%p, tag=0x%08lx, bp=%x, dp=%x, fp=%x)\n",
		v, tag, (int)busp, (int)devicep, (int)functionp);
#endif
	if (busp != NULL)
		*busp = (tag >> 16) & 0xff;
	if (devicep != NULL)
		*devicep = (tag >> 11) & 0x1f;
	if (functionp != NULL)
		*functionp = (tag >> 8) & 0x7;
}

void
ixp425_pci_conf_setup(void *v, struct ixp425_softc *sc, pcitag_t tag, int offset)
{
	int bus, device, function;

	ixp425_pci_decompose_tag(v, tag, &bus, &device, &function);

	if (bus == 0) { 
		if (device == 0 && function == 0) {
			PCI_CSR_WRITE_4(sc, PCI_NP_AD, (offset & ~3));
		} else {
			/* configuration type 0 */
			PCI_CSR_WRITE_4(sc, PCI_NP_AD, (1U << (32 - device)) |
				(function << 8) | (offset & ~3));
		}
	} else {
			/* configuration type 1 */
		PCI_CSR_WRITE_4(sc, PCI_NP_AD,
			(bus << 16) | (device << 11) |
			(function << 8) | (offset & ~3) | 1);
	}
}

/* read/write PCI Non-Pre-fetch Data */

pcireg_t
ixp425_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	struct ixp425_softc *sc = v;
	uint32_t data;
	pcireg_t rv;
	int s;
#define PCI_NP_HAVE_BUG
#ifdef PCI_NP_HAVE_BUG
	int i;
#endif

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	PCI_CONF_LOCK(s);
	ixp425_pci_conf_setup(v, sc, tag, offset);

#ifdef PCI_DEBUG
	printf("ixp425_pci_conf_read: tag=%lx,offset=%x\n",
		tag, offset);
#endif

#ifdef PCI_NP_HAVE_BUG
	/* PCI NP Bug workaround */
	for (i = 0; i < 8; i++) {
		PCI_CSR_WRITE_4(sc, PCI_NP_CBE, COMMAND_NP_CONF_READ);
		rv = PCI_CSR_READ_4(sc, PCI_NP_RDATA);
		rv = PCI_CSR_READ_4(sc, PCI_NP_RDATA);
	}
#else
	PCI_CSR_WRITE_4(sc, PCI_NP_CBE, COMMAND_NP_CONF_READ);
	rv = PCI_CSR_READ_4(sc, PCI_NP_RDATA);
#endif

	/* check&clear PCI abort */
	data = PCI_CSR_READ_4(sc, PCI_ISR);
	if (data & ISR_PFE) {
		PCI_CSR_WRITE_4(sc, PCI_ISR, ISR_PFE);
		PCI_CONF_UNLOCK(s);
		return -1;
	} else {
		PCI_CONF_UNLOCK(s);
		return rv;
	}
}

void
ixp425_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct ixp425_softc *sc = v;
	uint32_t data;
	int s;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return;

	PCI_CONF_LOCK(s);

	ixp425_pci_conf_setup(v, sc, tag, offset);
#ifdef PCI_DEBUG
	printf("ixp425_pci_conf_write: tag=%lx offset=%x <- val=%x\n",
		tag, offset, val);
#endif
	PCI_CSR_WRITE_4(sc, PCI_NP_CBE, COMMAND_NP_CONF_WRITE);
	PCI_CSR_WRITE_4(sc, PCI_NP_WDATA, val);

	/* check&clear PCI abort */
	data = PCI_CSR_READ_4(sc, PCI_ISR);
	if (data & ISR_PFE)
		PCI_CSR_WRITE_4(sc, PCI_ISR, ISR_PFE);

	PCI_CONF_UNLOCK(s);
}

/* read/write pci configuration data */

uint32_t
ixp425_pci_conf_reg_read(struct ixp425_softc *sc, uint32_t reg)
{
	uint32_t data;

	bus_space_write_4(sc->sc_iot, sc->sc_pci_ioh,
		PCI_CRP_AD_CBE, ((reg & ~3) | COMMAND_CRP_READ));
	data = bus_space_read_4(sc->sc_iot, sc->sc_pci_ioh,
		PCI_CRP_AD_RDATA);

	return data;
}

void
ixp425_pci_conf_reg_write(struct ixp425_softc *sc, uint32_t reg,
	uint32_t data)
{
	bus_space_write_4(sc->sc_iot, sc->sc_pci_ioh,
		PCI_CRP_AD_CBE, ((reg & ~3) | COMMAND_CRP_WRITE));
	bus_space_write_4(sc->sc_iot, sc->sc_pci_ioh,
		PCI_CRP_AD_WDATA, data);
}
