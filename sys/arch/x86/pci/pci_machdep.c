/*	$NetBSD: pci_machdep.c,v 1.11.2.5 2008/01/21 09:40:12 yamt Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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

/*
 * Machine-specific functions for PCI autoconfiguration.
 *
 * On PCs, there are two methods of generating PCI configuration cycles.
 * We try to detect the appropriate mechanism for this machine and set
 * up a few function pointers to access the correct method directly.
 *
 * The configuration method can be hard-coded in the config file by
 * using `options PCI_CONF_MODE=N', where `N' is the configuration mode
 * as defined section 3.6.4.1, `Generating Configuration Cycles'.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.11.2.5 2008/01/21 09:40:12 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <uvm/uvm_extern.h>

#include <machine/bus_private.h>

#include <machine/pio.h>
#include <machine/lock.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include "acpi.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

#if NACPI > 0
#include <machine/mpacpi.h>
#endif

#include <machine/mpconfig.h>

#include "opt_pci_conf_mode.h"

#ifdef __i386__
#include "opt_xbox.h"
#ifdef XBOX
#include <machine/xbox.h>
#endif
#endif

int pci_mode = -1;

static void pci_bridge_hook(pci_chipset_tag_t, pcitag_t, void *);
struct pci_bridge_hook_arg {
	void (*func)(pci_chipset_tag_t, pcitag_t, void *); 
	void *arg; 
}; 


__cpu_simple_lock_t pci_conf_lock = __SIMPLELOCK_UNLOCKED;

#define	PCI_CONF_LOCK(s)						\
do {									\
	(s) = splhigh();						\
	__cpu_simple_lock(&pci_conf_lock);				\
} while (0)

#define	PCI_CONF_UNLOCK(s)						\
do {									\
	__cpu_simple_unlock(&pci_conf_lock);				\
	splx((s));							\
} while (0)

#define	PCI_MODE1_ENABLE	0x80000000UL
#define	PCI_MODE1_ADDRESS_REG	0x0cf8
#define	PCI_MODE1_DATA_REG	0x0cfc

#define	PCI_MODE2_ENABLE_REG	0x0cf8
#define	PCI_MODE2_FORWARD_REG	0x0cfa

#define _m1tag(b, d, f) \
	(PCI_MODE1_ENABLE | ((b) << 16) | ((d) << 11) | ((f) << 8))
#define _qe(bus, dev, fcn, vend, prod) \
	{_m1tag(bus, dev, fcn), PCI_ID_CODE(vend, prod)}
struct {
	u_int32_t tag;
	pcireg_t id;
} pcim1_quirk_tbl[] = {
	_qe(0, 0, 0, PCI_VENDOR_COMPAQ, PCI_PRODUCT_COMPAQ_TRIFLEX1),
	/* XXX Triflex2 not tested */
	_qe(0, 0, 0, PCI_VENDOR_COMPAQ, PCI_PRODUCT_COMPAQ_TRIFLEX2),
	_qe(0, 0, 0, PCI_VENDOR_COMPAQ, PCI_PRODUCT_COMPAQ_TRIFLEX4),
	/* Triton needed for Connectix Virtual PC */
	_qe(0, 0, 0, PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82437FX),
	/* Connectix Virtual PC 5 has a 440BX */
	_qe(0, 0, 0, PCI_VENDOR_INTEL, PCI_PRODUCT_INTEL_82443BX_NOAGP),
	/* Parallels Desktop for Mac */
	_qe(0, 2, 0, PCI_VENDOR_PARALLELS, PCI_PRODUCT_PARALLELS_VIDEO),
	_qe(0, 3, 0, PCI_VENDOR_PARALLELS, PCI_PRODUCT_PARALLELS_TOOLS),
	/* SIS 741 */
	_qe(0, 0, 0, PCI_VENDOR_SIS, PCI_PRODUCT_SIS_741),
	{0, 0xffffffff} /* patchable */
};
#undef _m1tag
#undef _id
#undef _qe

/*
 * PCI doesn't have any special needs; just use the generic versions
 * of these functions.
 */
struct x86_bus_dma_tag pci_bus_dma_tag = {
	0,				/* tag_needs_free */
#if defined(_LP64) || defined(PAE)
	PCI32_DMA_BOUNCE_THRESHOLD,	/* bounce_thresh */
	ISA_DMA_BOUNCE_THRESHOLD,	/* bounce_alloclo */
	PCI32_DMA_BOUNCE_THRESHOLD,	/* bounce_allochi */
#else
	0,
	0,
	0,
#endif
	NULL,			/* _may_bounce */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmatag_subregion,
	_bus_dmatag_destroy,
};

#ifdef _LP64
struct x86_bus_dma_tag pci_bus_dma64_tag = {
	0,				/* tag_needs_free */
	0,
	0,
	0,
	NULL,			/* _may_bounce */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmatag_subregion,
	_bus_dmatag_destroy,
};
#endif

void
pci_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{

	if (pba->pba_bus == 0)
		aprint_normal(": configuration mode %d", pci_mode);
#ifdef MPBIOS
	mpbios_pci_attach_hook(parent, self, pba);
#endif
#if NACPI > 0
	mpacpi_pci_attach_hook(parent, self, pba);
#endif
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

#if defined(__i386__) && defined(XBOX)
	/*
	 * Scanning above the first device is fatal on the Microsoft Xbox.
	 * If busno=1, only allow for one device.
	 */
	if (arch_i386_is_xbox) {
		if (busno == 1)
			return 1;
		else if (busno > 1)
			return 0;
	}
#endif

	/*
	 * Bus number is irrelevant.  If Configuration Mechanism 2 is in
	 * use, can only have devices 0-15 on any bus.  If Configuration
	 * Mechanism 1 is in use, can have devices 0-32 (i.e. the `normal'
	 * range).
	 */
	if (pci_mode == 2)
		return (16);
	else
		return (32);
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_make_tag: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag.mode1 = PCI_MODE1_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
	return tag;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	if (bus >= 256 || device >= 16 || function >= 8)
		panic("pci_make_tag: bad request");

	tag.mode2.port = 0xc000 | (device << 8);
	tag.mode2.enable = 0xf0 | (function << 1);
	tag.mode2.forward = bus;
	return tag;
#endif
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
    int *bp, int *dp, int *fp)
{

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_decompose_tag: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	if (bp != NULL)
		*bp = (tag.mode1 >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag.mode1 >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag.mode1 >> 8) & 0x7;
	return;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	if (bp != NULL)
		*bp = tag.mode2.forward & 0xff;
	if (dp != NULL)
		*dp = (tag.mode2.port >> 8) & 0xf;
	if (fp != NULL)
		*fp = (tag.mode2.enable >> 1) & 0x7;
#endif
}

pcireg_t
pci_conf_read( pci_chipset_tag_t pc, pcitag_t tag,
    int reg)
{
	pcireg_t data;
	int s;

	KASSERT((reg & 0x3) == 0);
#if defined(__i386__) && defined(XBOX)
	if (arch_i386_is_xbox) {
		int bus, dev, fn;
		pci_decompose_tag(pc, tag, &bus, &dev, &fn);
		if (bus == 0 && dev == 0 && (fn == 1 || fn == 2))
			return (pcireg_t)-1;
	}
#endif

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_conf_read: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	PCI_CONF_LOCK(s);
	outl(PCI_MODE1_ADDRESS_REG, tag.mode1 | reg);
	data = inl(PCI_MODE1_DATA_REG);
	outl(PCI_MODE1_ADDRESS_REG, 0);
	PCI_CONF_UNLOCK(s);
	return data;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	PCI_CONF_LOCK(s);
	outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
	outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
	data = inl(tag.mode2.port | reg);
	outb(PCI_MODE2_ENABLE_REG, 0);
	PCI_CONF_UNLOCK(s);
	return data;
#endif
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg,
    pcireg_t data)
{
	int s;

	KASSERT((reg & 0x3) == 0);
#if defined(__i386__) && defined(XBOX)
	if (arch_i386_is_xbox) {
		int bus, dev, fn;
		pci_decompose_tag(pc, tag, &bus, &dev, &fn);
		if (bus == 0 && dev == 0 && (fn == 1 || fn == 2))
			return;
	}
#endif

#ifndef PCI_CONF_MODE
	switch (pci_mode) {
	case 1:
		goto mode1;
	case 2:
		goto mode2;
	default:
		panic("pci_conf_write: mode not configured");
	}
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 1)
#ifndef PCI_CONF_MODE
mode1:
#endif
	PCI_CONF_LOCK(s);
	outl(PCI_MODE1_ADDRESS_REG, tag.mode1 | reg);
	outl(PCI_MODE1_DATA_REG, data);
	outl(PCI_MODE1_ADDRESS_REG, 0);
	PCI_CONF_UNLOCK(s);
	return;
#endif

#if !defined(PCI_CONF_MODE) || (PCI_CONF_MODE == 2)
#ifndef PCI_CONF_MODE
mode2:
#endif
	PCI_CONF_LOCK(s);
	outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
	outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
	outl(tag.mode2.port | reg, data);
	outb(PCI_MODE2_ENABLE_REG, 0);
	PCI_CONF_UNLOCK(s);
#endif
}

int
pci_mode_detect()
{

#ifdef PCI_CONF_MODE
#if (PCI_CONF_MODE == 1) || (PCI_CONF_MODE == 2)
	return (pci_mode = PCI_CONF_MODE);
#else
#error Invalid PCI configuration mode.
#endif
#else
	u_int32_t sav, val;
	int i;
	pcireg_t idreg;

	if (pci_mode != -1)
		return pci_mode;

	/*
	 * We try to divine which configuration mode the host bridge wants.
	 */

	sav = inl(PCI_MODE1_ADDRESS_REG);

	pci_mode = 1; /* assume this for now */
	/*
	 * catch some known buggy implementations of mode 1
	 */
	for (i = 0; i < __arraycount(pcim1_quirk_tbl); i++) {
		pcitag_t t;

		if (!pcim1_quirk_tbl[i].tag)
			break;
		t.mode1 = pcim1_quirk_tbl[i].tag;
		idreg = pci_conf_read(0, t, PCI_ID_REG); /* needs "pci_mode" */
		if (idreg == pcim1_quirk_tbl[i].id) {
#ifdef DEBUG
			printf("known mode 1 PCI chipset (%08x)\n",
			       idreg);
#endif
			return (pci_mode);
		}
	}

	/*
	 * Strong check for standard compliant mode 1:
	 * 1. bit 31 ("enable") can be set
	 * 2. byte/word access does not affect register
	 */
	outl(PCI_MODE1_ADDRESS_REG, PCI_MODE1_ENABLE);
	outb(PCI_MODE1_ADDRESS_REG + 3, 0);
	outw(PCI_MODE1_ADDRESS_REG + 2, 0);
	val = inl(PCI_MODE1_ADDRESS_REG);
	if ((val & 0x80fffffc) != PCI_MODE1_ENABLE) {
#ifdef DEBUG
		printf("pci_mode_detect: mode 1 enable failed (%x)\n",
		       val);
#endif
		goto not1;
	}
	outl(PCI_MODE1_ADDRESS_REG, 0);
	val = inl(PCI_MODE1_ADDRESS_REG);
	if ((val & 0x80fffffc) != 0)
		goto not1;
	return (pci_mode);
not1:
	outl(PCI_MODE1_ADDRESS_REG, sav);

	/*
	 * This mode 2 check is quite weak (and known to give false
	 * positives on some Compaq machines).
	 * However, this doesn't matter, because this is the
	 * last test, and simply no PCI devices will be found if
	 * this happens.
	 */
	outb(PCI_MODE2_ENABLE_REG, 0);
	outb(PCI_MODE2_FORWARD_REG, 0);
	if (inb(PCI_MODE2_ENABLE_REG) != 0 ||
	    inb(PCI_MODE2_FORWARD_REG) != 0)
		goto not2;
	return (pci_mode = 2);
not2:

	return (pci_mode = 0);
#endif
}

/*
 * Determine which flags should be passed to the primary PCI bus's
 * autoconfiguration node.  We use this to detect broken chipsets
 * which cannot safely use memory-mapped device access.
 */
int
pci_bus_flags()
{
	int rval = PCI_FLAGS_IO_ENABLED | PCI_FLAGS_MEM_ENABLED |
	    PCI_FLAGS_MRL_OKAY | PCI_FLAGS_MRM_OKAY | PCI_FLAGS_MWI_OKAY;
	int device, maxndevs;
	pcitag_t tag;
	pcireg_t id;

	maxndevs = pci_bus_maxdevs(NULL, 0);

	for (device = 0; device < maxndevs; device++) {
		tag = pci_make_tag(NULL, 0, device, 0);
		id = pci_conf_read(NULL, tag, PCI_ID_REG);

		/* Invalid vendor ID value? */
		if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
			continue;
		/* XXX Not invalid, but we've done this ~forever. */
		if (PCI_VENDOR(id) == 0)
			continue;

		switch (PCI_VENDOR(id)) {
		case PCI_VENDOR_SIS:
			switch (PCI_PRODUCT(id)) {
			case PCI_PRODUCT_SIS_85C496:
				goto disable_mem;
			}
			break;
		}
	}

	return (rval);

 disable_mem:
	printf("Warning: broken PCI-Host bridge detected; "
	    "disabling memory-mapped access\n");
	rval &= ~(PCI_FLAGS_MEM_ENABLED|PCI_FLAGS_MRL_OKAY|PCI_FLAGS_MRM_OKAY|
	    PCI_FLAGS_MWI_OKAY);
	return (rval);
}

void
pci_device_foreach(pci_chipset_tag_t pc, int maxbus,
	void (*func)(pci_chipset_tag_t, pcitag_t, void *), void *context)
{
	pci_device_foreach_min(pc, 0, maxbus, func, context);
}

void
pci_device_foreach_min(pci_chipset_tag_t pc, int minbus, int maxbus,
	void (*func)(pci_chipset_tag_t, pcitag_t, void *), void *context)
{
	const struct pci_quirkdata *qd;
	int bus, device, function, maxdevs, nfuncs;
	pcireg_t id, bhlcr;
	pcitag_t tag;

	for (bus = minbus; bus <= maxbus; bus++) {
		maxdevs = pci_bus_maxdevs(pc, bus);
		for (device = 0; device < maxdevs; device++) {
			tag = pci_make_tag(pc, bus, device, 0);
			id = pci_conf_read(pc, tag, PCI_ID_REG);

			/* Invalid vendor ID value? */
			if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
				continue;
			/* XXX Not invalid, but we've done this ~forever. */
			if (PCI_VENDOR(id) == 0)
				continue;

			qd = pci_lookup_quirkdata(PCI_VENDOR(id),
				PCI_PRODUCT(id));

			bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);
			if (PCI_HDRTYPE_MULTIFN(bhlcr) ||
			     (qd != NULL &&
		  	     (qd->quirks & PCI_QUIRK_MULTIFUNCTION) != 0))
				nfuncs = 8;
			else
				nfuncs = 1;

			for (function = 0; function < nfuncs; function++) {
				tag = pci_make_tag(pc, bus, device, function);
				id = pci_conf_read(pc, tag, PCI_ID_REG);

				/* Invalid vendor ID value? */
				if (PCI_VENDOR(id) == PCI_VENDOR_INVALID)
					continue;
				/*
				 * XXX Not invalid, but we've done this
				 * ~forever.
				 */
				if (PCI_VENDOR(id) == 0)
					continue;
				(*func)(pc, tag, context);
			}
		}
	}
}

void
pci_bridge_foreach(pci_chipset_tag_t pc, int minbus, int maxbus,
	void (*func)(pci_chipset_tag_t, pcitag_t, void *), void *ctx)
{
	struct pci_bridge_hook_arg bridge_hook;

	bridge_hook.func = func;
	bridge_hook.arg = ctx;  

	pci_device_foreach_min(pc, minbus, maxbus, pci_bridge_hook,
		&bridge_hook);      
}

static void
pci_bridge_hook(pci_chipset_tag_t pc, pcitag_t tag, void *ctx)
{
	struct pci_bridge_hook_arg *bridge_hook = (void *)ctx;
	pcireg_t reg;

	reg = pci_conf_read(pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(reg) == PCI_CLASS_BRIDGE &&
 	     (PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_PCI ||
		PCI_SUBCLASS(reg) == PCI_SUBCLASS_BRIDGE_CARDBUS)) {
		(*bridge_hook->func)(pc, tag, bridge_hook->arg);
	}
}
