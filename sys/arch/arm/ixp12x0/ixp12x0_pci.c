/* $NetBSD: ixp12x0_pci.c,v 1.12.2.3 2017/12/03 11:35:54 jdolecek Exp $ */
/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
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
__KERNEL_RCSID(0, "$NetBSD: ixp12x0_pci.c,v 1.12.2.3 2017/12/03 11:35:54 jdolecek Exp $");

/*
 * PCI configuration support for IXP12x0 Network Processor chip.
 */

#include "opt_pci.h"
#include "pci.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/extent.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <arm/ixp12x0/ixp12x0reg.h>
#include <arm/ixp12x0/ixp12x0var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pciconf.h>

#include <arm/locore.h>

void ixp12x0_pci_attach_hook(device_t, device_t,
	struct pcibus_attach_args *);
int ixp12x0_pci_bus_maxdevs(void *, int);
pcitag_t ixp12x0_pci_make_tag(void *, int, int, int);
void ixp12x0_pci_decompose_tag(void *, pcitag_t, int *, int *, int *);
pcireg_t ixp12x0_pci_conf_read(void *, pcitag_t, int);
void ixp12x0_pci_conf_write(void *, pcitag_t, int, pcireg_t);
void ixp12x0_pci_conf_interrupt(void *, int, int, int, int, int *);

static vaddr_t ixp12x0_pci_conf_setup(void *, struct ixp12x0_softc *, pcitag_t, int);

#define PCI_CONF_LOCK(s)	(s) = disable_interrupts(I32_bit)
#define PCI_CONF_UNLOCK(s)	restore_interrupts((s))

#define	MAX_PCI_DEVICES	4

/*
 * IXM1200 PCI configuration Cycles
 *  Device               Address
 * -------------------------------------
 *   0    IXP1200        0x0800 - 0x08FF
 *   1    i21555         0x1000 - 0x10FF
 *   2    i82559         0x2000 - 0x20FF
 *   3    PMC expansion  0x4000 - 0x40FF
 */

void
ixp12x0_pci_init(pci_chipset_tag_t pc, void *cookie)
{
#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	struct ixp12x0_softc *sc = cookie;
	struct extent *ioext, *memext;
#endif
	pc->pc_conf_v = cookie;
	pc->pc_attach_hook = ixp12x0_pci_attach_hook;
	pc->pc_bus_maxdevs = ixp12x0_pci_bus_maxdevs;
	pc->pc_make_tag = ixp12x0_pci_make_tag;
	pc->pc_decompose_tag = ixp12x0_pci_decompose_tag;
	pc->pc_conf_read = ixp12x0_pci_conf_read;
	pc->pc_conf_write = ixp12x0_pci_conf_write;
	pc->pc_conf_interrupt = ixp12x0_pci_conf_interrupt;

#if NPCI > 0 && defined(PCI_NETBSD_CONFIGURE)
	ioext  = extent_create("pciio", 0, IXP12X0_PCI_IO_SIZE - 1,
				NULL, 0, EX_NOWAIT);
	/* PCI MEM space is mapped same address as real memory */
	memext = extent_create("pcimem", IXP12X0_PCI_MEM_HWBASE,
				IXP12X0_PCI_MEM_HWBASE +
				IXP12X0_PCI_MEM_SIZE - 1,
				NULL, 0, EX_NOWAIT);
	aprint_normal_dev(sc->sc_dev, "configuring PCI bus\n");
	pci_configure_bus(pc, ioext, memext, NULL, 0 /* XXX bus = 0 */,
			  arm_dcache_align);

	extent_destroy(ioext);
	extent_destroy(memext);
#endif
}

void
ixp12x0_pci_conf_interrupt(void *v, int a, int b, int c, int d, int *p)
{
	/* Nothing */
}

void
ixp12x0_pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
	/* Nothing to do. */
}

int
ixp12x0_pci_bus_maxdevs(void *v, int busno)
{
	return(MAX_PCI_DEVICES);
}

pcitag_t
ixp12x0_pci_make_tag(void *v, int bus, int device, int function)
{
#ifdef PCI_DEBUG
	printf("ixp12x0_pci_make_tag(v=%p, bus=%d, device=%d, function=%d)\n",
		v, bus, device, function);
#endif
	return ((bus << 16) | (device << 11) | (function << 8));
}

void
ixp12x0_pci_decompose_tag(void *v, pcitag_t tag, int *busp, int *devicep, int *functionp)
{
#ifdef PCI_DEBUG
	printf("ixp12x0_pci_decompose_tag(v=%p, tag=0x%08lx, bp=%x, dp=%x, fp=%x)\n",
		v, tag, (int)busp, (int)devicep, (int)functionp);
#endif

	if (busp != NULL)
		*busp = (tag >> 16) & 0xff;
	if (devicep != NULL)
		*devicep = (tag >> 11) & 0x1f;
	if (functionp != NULL)
		*functionp = (tag >> 8) & 0x7;
}

static vaddr_t
ixp12x0_pci_conf_setup(void *v, struct ixp12x0_softc *sc, pcitag_t tag, int offset)
{
	int bus, device, function;
	vaddr_t addr;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return 0;

	ixp12x0_pci_decompose_tag(v, tag, &bus, &device, &function);

	if (bus == 0) { 
		/* configuration type 0 */
		addr = (vaddr_t) bus_space_vaddr(sc->sc_iot, sc->sc_conf0_ioh) +
			((1 << (device + 10)) | (offset & ~3));
	} else {
		/* configuration type 1 */
		addr = (vaddr_t) bus_space_vaddr(sc->sc_iot, sc->sc_conf1_ioh) +
			((bus << 16) | (device << 11) |
			 (function << 8) | (offset & ~3) | 1);
	}
		return addr;
}

pcireg_t
ixp12x0_pci_conf_read(void *v, pcitag_t tag, int offset)
{
	struct ixp12x0_softc *sc = v;
	vaddr_t va = ixp12x0_pci_conf_setup(v, sc, tag, offset);
	pcireg_t rv;
	int s;

#ifdef PCI_DEBUG
	printf("ixp12x0_pci_conf_read: base=%lx,va=%lx,tag=%lx,offset=%x\n",
		sc->sc_conf0_ioh, va, tag, offset);
#endif
	if (va == 0)
		return -1;

	PCI_CONF_LOCK(s);

	if (badaddr_read((void *) va, sizeof(rv), &rv)) {
#ifdef PCI_DEBUG
		printf("conf_read: %lx bad address\n", va);
#endif
		rv = (pcireg_t) - 1;
	}

	PCI_CONF_UNLOCK(s);

	return rv;
}

void
ixp12x0_pci_conf_write(void *v, pcitag_t tag, int offset, pcireg_t val)
{
	struct ixp12x0_softc *sc = v;
	vaddr_t va = ixp12x0_pci_conf_setup(v, sc, tag, offset);
	int s;

#ifdef PCI_DEBUG
	printf("ixp12x0_pci_conf_write: tag=%lx offset=%x -> va=%lx (base=%lx)\n",
		tag, offset, va, sc->sc_conf0_ioh);
#endif

	PCI_CONF_LOCK(s);

	*(pcireg_t *) va = val;

	PCI_CONF_UNLOCK(s);
}
