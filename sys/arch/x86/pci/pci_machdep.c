/*	$NetBSD: pci_machdep.c,v 1.48 2011/08/28 06:04:18 dyoung Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.48 2011/08/28 06:04:18 dyoung Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/kmem.h>

#include <prop/proplib.h>
#include <ppath/ppath.h>

#include <uvm/uvm_extern.h>

#include <machine/bus_private.h>

#include <machine/pio.h>
#include <machine/lock.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pccbbreg.h>
#include <dev/pci/pcidevs.h>

#include "acpica.h"
#include "opt_mpbios.h"
#include "opt_acpi.h"

#ifdef MPBIOS
#include <machine/mpbiosvar.h>
#endif

#if NACPICA > 0
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

#ifdef PCI_CONF_MODE
#if (PCI_CONF_MODE == 1) || (PCI_CONF_MODE == 2)
static int pci_mode = PCI_CONF_MODE;
#else
#error Invalid PCI configuration mode.
#endif
#else
static int pci_mode = -1;
#endif

struct pci_conf_lock {
	uint32_t cl_cpuno;	/* 0: unlocked
				 * 1 + n: locked by CPU n (0 <= n)
				 */
	uint32_t cl_sel;	/* the address that's being read. */
};

static void pci_conf_unlock(struct pci_conf_lock *);
static uint32_t pci_conf_selector(pcitag_t, int);
static unsigned int pci_conf_port(pcitag_t, int);
static void pci_conf_select(uint32_t);
static void pci_conf_lock(struct pci_conf_lock *, uint32_t);
static void pci_bridge_hook(pci_chipset_tag_t, pcitag_t, void *);
struct pci_bridge_hook_arg {
	void (*func)(pci_chipset_tag_t, pcitag_t, void *); 
	void *arg; 
}; 

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
	uint32_t tag;
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
	/* SIS 740 */
	_qe(0, 0, 0, PCI_VENDOR_SIS, PCI_PRODUCT_SIS_740),
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
	._tag_needs_free	= 0,
#if defined(_LP64) || defined(PAE)
	._bounce_thresh		= PCI32_DMA_BOUNCE_THRESHOLD,
	._bounce_alloc_lo	= ISA_DMA_BOUNCE_THRESHOLD,
	._bounce_alloc_hi	= PCI32_DMA_BOUNCE_THRESHOLD,
#else
	._bounce_thresh		= 0,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= 0,
#endif
	._may_bounce		= NULL,

	._dmamap_create		= _bus_dmamap_create,
	._dmamap_destroy	= _bus_dmamap_destroy,
	._dmamap_load		= _bus_dmamap_load,
	._dmamap_load_mbuf	= _bus_dmamap_load_mbuf,
	._dmamap_load_uio	= _bus_dmamap_load_uio,
	._dmamap_load_raw	= _bus_dmamap_load_raw,
	._dmamap_unload		= _bus_dmamap_unload,
	._dmamap_sync 		= _bus_dmamap_sync,

	._dmamem_alloc		= _bus_dmamem_alloc,
	._dmamem_free		= _bus_dmamem_free,
	._dmamem_map		= _bus_dmamem_map,
	._dmamem_unmap		= _bus_dmamem_unmap,
	._dmamem_mmap		= _bus_dmamem_mmap,

	._dmatag_subregion	= _bus_dmatag_subregion,
	._dmatag_destroy	= _bus_dmatag_destroy,
};

#ifdef _LP64
struct x86_bus_dma_tag pci_bus_dma64_tag = {
	._tag_needs_free	= 0,
	._bounce_thresh		= 0,
	._bounce_alloc_lo	= 0,
	._bounce_alloc_hi	= 0,
	._may_bounce		= NULL,

	._dmamap_create		= _bus_dmamap_create,
	._dmamap_destroy	= _bus_dmamap_destroy,
	._dmamap_load		= _bus_dmamap_load,
	._dmamap_load_mbuf	= _bus_dmamap_load_mbuf,
	._dmamap_load_uio	= _bus_dmamap_load_uio,
	._dmamap_load_raw	= _bus_dmamap_load_raw,
	._dmamap_unload		= _bus_dmamap_unload,
	._dmamap_sync 		= NULL,

	._dmamem_alloc		= _bus_dmamem_alloc,
	._dmamem_free		= _bus_dmamem_free,
	._dmamem_map		= _bus_dmamem_map,
	._dmamem_unmap		= _bus_dmamem_unmap,
	._dmamem_mmap		= _bus_dmamem_mmap,

	._dmatag_subregion	= _bus_dmatag_subregion,
	._dmatag_destroy	= _bus_dmatag_destroy,
};
#endif

static struct pci_conf_lock cl0 = {
	  .cl_cpuno = 0UL
	, .cl_sel = 0UL
};

static struct pci_conf_lock * const cl = &cl0;

static void
pci_conf_lock(struct pci_conf_lock *ocl, uint32_t sel)
{
	uint32_t cpuno;

	KASSERT(sel != 0);

	kpreempt_disable();
	cpuno = cpu_number() + 1;
	/* If the kernel enters pci_conf_lock() through an interrupt
	 * handler, then the CPU may already hold the lock.
	 *
	 * If the CPU does not already hold the lock, spin until
	 * we can acquire it.
	 */
	if (cpuno == cl->cl_cpuno) {
		ocl->cl_cpuno = cpuno;
	} else {
		u_int spins;

		ocl->cl_cpuno = 0;

		spins = SPINLOCK_BACKOFF_MIN;
		while (atomic_cas_32(&cl->cl_cpuno, 0, cpuno) != 0) {
			SPINLOCK_BACKOFF(spins);
#ifdef LOCKDEBUG
			if (SPINLOCK_SPINOUT(spins)) {
				panic("%s: cpu %" PRId32
				    " spun out waiting for cpu %" PRId32,
				    __func__, cpuno, cl->cl_cpuno);
			}
#endif	/* LOCKDEBUG */
		}
	}

	/* Only one CPU can be here, so an interlocked atomic_swap(3)
	 * is not necessary.
	 *
	 * Evaluating atomic_cas_32_ni()'s argument, cl->cl_sel,
	 * and applying atomic_cas_32_ni() is not an atomic operation,
	 * however, any interrupt that, in the middle of the
	 * operation, modifies cl->cl_sel, will also restore
	 * cl->cl_sel.  So cl->cl_sel will have the same value when
	 * we apply atomic_cas_32_ni() as when we evaluated it,
	 * before.
	 */
	ocl->cl_sel = atomic_cas_32_ni(&cl->cl_sel, cl->cl_sel, sel);
	pci_conf_select(sel);
}

static void
pci_conf_unlock(struct pci_conf_lock *ocl)
{
	uint32_t sel;

	sel = atomic_cas_32_ni(&cl->cl_sel, cl->cl_sel, ocl->cl_sel);
	pci_conf_select(ocl->cl_sel);
	if (ocl->cl_cpuno != cl->cl_cpuno)
		atomic_cas_32(&cl->cl_cpuno, cl->cl_cpuno, ocl->cl_cpuno);
	kpreempt_enable();
}

static uint32_t
pci_conf_selector(pcitag_t tag, int reg)
{
	static const pcitag_t mode2_mask = {
		.mode2 = {
			  .enable = 0xff
			, .forward = 0xff
		}
	};

	switch (pci_mode) {
	case 1:
		return tag.mode1 | reg;
	case 2:
		return tag.mode1 & mode2_mask.mode1;
	default:
		panic("%s: mode not configured", __func__);
	}
}

static unsigned int
pci_conf_port(pcitag_t tag, int reg)
{
	switch (pci_mode) {
	case 1:
		return PCI_MODE1_DATA_REG;
	case 2:
		return tag.mode2.port | reg;
	default:
		panic("%s: mode not configured", __func__);
	}
}

static void
pci_conf_select(uint32_t sel)
{
	pcitag_t tag;

	switch (pci_mode) {
	case 1:
		outl(PCI_MODE1_ADDRESS_REG, sel);
		return;
	case 2:
		tag.mode1 = sel;
		outb(PCI_MODE2_ENABLE_REG, tag.mode2.enable);
		if (tag.mode2.enable != 0)
			outb(PCI_MODE2_FORWARD_REG, tag.mode2.forward);
		return;
	default:
		panic("%s: mode not configured", __func__);
	}
}

void
pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{

	if (pba->pba_bus == 0)
		aprint_normal(": configuration mode %d", pci_mode);
#ifdef MPBIOS
	mpbios_pci_attach_hook(parent, self, pba);
#endif
#if NACPICA > 0
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
	pci_chipset_tag_t ipc;
	pcitag_t tag;

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_MAKE_TAG) == 0)
			continue;
		return (*ipc->pc_ov->ov_make_tag)(ipc->pc_ctx,
		    pc, bus, device, function);
	}

	switch (pci_mode) {
	case 1:
		if (bus >= 256 || device >= 32 || function >= 8)
			panic("%s: bad request", __func__);

		tag.mode1 = PCI_MODE1_ENABLE |
			    (bus << 16) | (device << 11) | (function << 8);
		return tag;
	case 2:
		if (bus >= 256 || device >= 16 || function >= 8)
			panic("%s: bad request", __func__);

		tag.mode2.port = 0xc000 | (device << 8);
		tag.mode2.enable = 0xf0 | (function << 1);
		tag.mode2.forward = bus;
		return tag;
	default:
		panic("%s: mode not configured", __func__);
	}
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
    int *bp, int *dp, int *fp)
{
	pci_chipset_tag_t ipc;

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_DECOMPOSE_TAG) == 0)
			continue;
		(*ipc->pc_ov->ov_decompose_tag)(ipc->pc_ctx,
		    pc, tag, bp, dp, fp);
		return;
	}

	switch (pci_mode) {
	case 1:
		if (bp != NULL)
			*bp = (tag.mode1 >> 16) & 0xff;
		if (dp != NULL)
			*dp = (tag.mode1 >> 11) & 0x1f;
		if (fp != NULL)
			*fp = (tag.mode1 >> 8) & 0x7;
		return;
	case 2:
		if (bp != NULL)
			*bp = tag.mode2.forward & 0xff;
		if (dp != NULL)
			*dp = (tag.mode2.port >> 8) & 0xf;
		if (fp != NULL)
			*fp = (tag.mode2.enable >> 1) & 0x7;
		return;
	default:
		panic("%s: mode not configured", __func__);
	}
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	pci_chipset_tag_t ipc;
	pcireg_t data;
	struct pci_conf_lock ocl;

	KASSERT((reg & 0x3) == 0);

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_CONF_READ) == 0)
			continue;
		return (*ipc->pc_ov->ov_conf_read)(ipc->pc_ctx, pc, tag, reg);
	}

#if defined(__i386__) && defined(XBOX)
	if (arch_i386_is_xbox) {
		int bus, dev, fn;
		pci_decompose_tag(pc, tag, &bus, &dev, &fn);
		if (bus == 0 && dev == 0 && (fn == 1 || fn == 2))
			return (pcireg_t)-1;
	}
#endif

	pci_conf_lock(&ocl, pci_conf_selector(tag, reg));
	data = inl(pci_conf_port(tag, reg));
	pci_conf_unlock(&ocl);
	return data;
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{
	pci_chipset_tag_t ipc;
	struct pci_conf_lock ocl;

	KASSERT((reg & 0x3) == 0);

	for (ipc = pc; ipc != NULL; ipc = ipc->pc_super) {
		if ((ipc->pc_present & PCI_OVERRIDE_CONF_WRITE) == 0)
			continue;
		(*ipc->pc_ov->ov_conf_write)(ipc->pc_ctx, pc, tag, reg,
		    data);
		return;
	}

#if defined(__i386__) && defined(XBOX)
	if (arch_i386_is_xbox) {
		int bus, dev, fn;
		pci_decompose_tag(pc, tag, &bus, &dev, &fn);
		if (bus == 0 && dev == 0 && (fn == 1 || fn == 2))
			return;
	}
#endif

	pci_conf_lock(&ocl, pci_conf_selector(tag, reg));
	outl(pci_conf_port(tag, reg), data);
	pci_conf_unlock(&ocl);
}

void
pci_mode_set(int mode)
{
	KASSERT(pci_mode == -1 || pci_mode == mode);

	pci_mode = mode;
}

int
pci_mode_detect(void)
{
	uint32_t sav, val;
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
}

/*
 * Determine which flags should be passed to the primary PCI bus's
 * autoconfiguration node.  We use this to detect broken chipsets
 * which cannot safely use memory-mapped device access.
 */
int
pci_bus_flags(void)
{
	int rval = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY |
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
	rval &= ~(PCI_FLAGS_MEM_OKAY|PCI_FLAGS_MRL_OKAY|PCI_FLAGS_MRM_OKAY|
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

typedef enum pci_alloc_regtype {
	  PCI_ALLOC_REGTYPE_NONE = 0
	, PCI_ALLOC_REGTYPE_BAR = 1
	, PCI_ALLOC_REGTYPE_WIN = 2
	, PCI_ALLOC_REGTYPE_CBWIN = 3
	, PCI_ALLOC_REGTYPE_VGA_EN = 4
} pci_alloc_regtype_t;

typedef enum pci_alloc_space {
	  PCI_ALLOC_SPACE_IO = 0
	, PCI_ALLOC_SPACE_MEM = 1
} pci_alloc_space_t;

typedef enum pci_alloc_flags {
	  PCI_ALLOC_F_PREFETCHABLE = 0x1
} pci_alloc_flags_t;

typedef struct pci_alloc {
	TAILQ_ENTRY(pci_alloc)		pal_link;
	pcitag_t			pal_tag;
	uint64_t			pal_addr;
	uint64_t			pal_size;
	pci_alloc_regtype_t		pal_type;
	struct pci_alloc_reg {
		int			r_ofs;
		pcireg_t		r_val;
		pcireg_t		r_mask;
	}				pal_reg[3];
	pci_alloc_space_t		pal_space;
	pci_alloc_flags_t		pal_flags;
} pci_alloc_t;

typedef struct pci_alloc_reg pci_alloc_reg_t;

TAILQ_HEAD(pci_alloc_list, pci_alloc);

typedef struct pci_alloc_list pci_alloc_list_t;

static pci_alloc_t *
pci_alloc_dup(const pci_alloc_t *pal)
{
	pci_alloc_t *npal;

	if ((npal = kmem_alloc(sizeof(*npal), KM_SLEEP)) == NULL)
		return NULL;

	*npal = *pal;

	return npal;
}

static bool
pci_alloc_linkdup(pci_alloc_list_t *pals, const pci_alloc_t *pal)
{
	pci_alloc_t *npal;

	if ((npal = pci_alloc_dup(pal)) == NULL)
		return false;
	
	TAILQ_INSERT_TAIL(pals, npal, pal_link);

	return true;
}
 
struct range_infer_ctx {
	pci_chipset_tag_t	ric_pc;
	pci_alloc_list_t	ric_pals;
	bus_addr_t		ric_mmio_bottom;
	bus_addr_t		ric_mmio_top;
	bus_addr_t		ric_io_bottom;
	bus_addr_t		ric_io_top;
};

#if 1
static bool
io_range_extend(struct range_infer_ctx *ric, const pci_alloc_t *pal)
{
	if (ric->ric_io_bottom > pal->pal_addr)
		ric->ric_io_bottom = pal->pal_addr;
	if (ric->ric_io_top < pal->pal_addr + pal->pal_size)
		ric->ric_io_top = pal->pal_addr + pal->pal_size;

	return pci_alloc_linkdup(&ric->ric_pals, pal);
}

static bool
io_range_extend_by_bar(struct range_infer_ctx *ric, int bus, int dev, int fun,
    int ofs, pcireg_t curbar, pcireg_t sizebar)
{
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_BAR
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r->r_ofs = ofs;
	r->r_val = curbar;

	pal.pal_addr = PCI_MAPREG_IO_ADDR(curbar);
	pal.pal_size = PCI_MAPREG_IO_SIZE(sizebar);

	aprint_debug("%s: %d.%d.%d base at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return (pal.pal_size == 0) || io_range_extend(ric, &pal);
}

static bool
io_range_extend_by_vga_enable(struct range_infer_ctx *ric,
    int bus, int dev, int fun, pcireg_t csr, pcireg_t bcr)
{
	pci_alloc_reg_t *r;
	pci_alloc_t tpal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_VGA_EN
		, .pal_reg = {{
			  .r_ofs = PCI_COMMAND_STATUS_REG
			, .r_mask = PCI_COMMAND_IO_ENABLE
		  }, {
			  .r_ofs = PCI_BRIDGE_CONTROL_REG
			, .r_mask =
			    PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT
		  }}
	}, pal[2];

	aprint_debug("%s: %d.%d.%d enter\n", __func__, bus, dev, fun);

	if ((csr & PCI_COMMAND_IO_ENABLE) == 0 ||
	    (bcr & (PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT)) == 0) {
		aprint_debug("%s: %d.%d.%d I/O or VGA disabled\n",
		    __func__, bus, dev, fun);
		return true;
	}

	r = &tpal.pal_reg[0];
	tpal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_val = csr;
	r[1].r_val = bcr;

	pal[0] = pal[1] = tpal;

	pal[0].pal_addr = 0x3b0;
	pal[0].pal_size = 0x3bb - 0x3b0 + 1;

	pal[1].pal_addr = 0x3c0;
	pal[1].pal_size = 0x3df - 0x3c0 + 1;

	/* XXX add aliases for pal[0..1] */

	return io_range_extend(ric, &pal[0]) && io_range_extend(ric, &pal[1]);
}

static bool
io_range_extend_by_win(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, int ofshigh,
    pcireg_t io, pcireg_t iohigh)
{
	const int fourkb = 4 * 1024;
	pcireg_t baser, limitr;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_WIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = io;

	baser = ((io >> PCI_BRIDGE_STATIO_IOBASE_SHIFT) &
	    PCI_BRIDGE_STATIO_IOBASE_MASK) >> 4;
	limitr = ((io >> PCI_BRIDGE_STATIO_IOLIMIT_SHIFT) &
	    PCI_BRIDGE_STATIO_IOLIMIT_MASK) >> 4;

	if (PCI_BRIDGE_IO_32BITS(io)) {
		pcireg_t baseh, limith;

		r[1].r_mask = ~(pcireg_t)0;
		r[1].r_ofs = ofshigh;
		r[1].r_val = iohigh;

		baseh = (iohigh >> PCI_BRIDGE_IOHIGH_BASE_SHIFT) & PCI_BRIDGE_IOHIGH_BASE_MASK;
		limith = (iohigh >> PCI_BRIDGE_IOHIGH_LIMIT_SHIFT) & PCI_BRIDGE_IOHIGH_LIMIT_MASK;

		baser |= baseh << 4;
		limitr |= limith << 4;
	}

	/* XXX check with the PCI standard */
	if (baser > limitr)
		return true;

	pal.pal_addr = baser * fourkb;
	pal.pal_size = (limitr - baser + 1) * fourkb;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return io_range_extend(ric, &pal);
}

static bool
io_range_extend_by_cbwin(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t base0, pcireg_t limit0)
{
	pcireg_t base, limit;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_IO
		, .pal_type = PCI_ALLOC_REGTYPE_CBWIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }, {
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = base0;
	r[1].r_ofs = ofs + 4;
	r[1].r_val = limit0;

	base = base0 & __BITS(31, 2);
	limit = limit0 & __BITS(31, 2);

	if (base > limit)
		return true;

	pal.pal_addr = base;
	pal.pal_size = limit - base + 4;	/* XXX */

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return io_range_extend(ric, &pal);
}

static void
io_range_infer(pci_chipset_tag_t pc, pcitag_t tag, void *ctx)
{
	struct range_infer_ctx *ric = ctx;
	pcireg_t bhlcr, limit, io;
	int bar, bus, dev, fun, hdrtype, nbar;
	bool ok = true;

	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);

	hdrtype = PCI_HDRTYPE_TYPE(bhlcr);

	pci_decompose_tag(pc, tag, &bus, &dev, &fun);

	switch (hdrtype) {
	case PCI_HDRTYPE_PPB:
		nbar = 2;
		/* Extract I/O windows */
		ok = ok && io_range_extend_by_win(ric, bus, dev, fun,
		    PCI_BRIDGE_STATIO_REG,
		    PCI_BRIDGE_IOHIGH_REG,
		    pci_conf_read(pc, tag, PCI_BRIDGE_STATIO_REG),
		    pci_conf_read(pc, tag, PCI_BRIDGE_IOHIGH_REG));
		ok = ok && io_range_extend_by_vga_enable(ric, bus, dev, fun,
		    pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG),
		    pci_conf_read(pc, tag, PCI_BRIDGE_CONTROL_REG));
		break;
	case PCI_HDRTYPE_PCB:
		/* Extract I/O windows */
		io = pci_conf_read(pc, tag, PCI_CB_IOBASE0);
		limit = pci_conf_read(pc, tag, PCI_CB_IOLIMIT0);
		ok = ok && io_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_IOBASE0, io, limit);
		io = pci_conf_read(pc, tag, PCI_CB_IOBASE1);
		limit = pci_conf_read(pc, tag, PCI_CB_IOLIMIT1);
		ok = ok && io_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_IOBASE1, io, limit);
		nbar = 1;
		break;
	case PCI_HDRTYPE_DEVICE:
		nbar = 6;
		break;
	default:
		aprint_debug("%s: unknown header type %d at %d.%d.%d\n",
		    __func__, hdrtype, bus, dev, fun);
		return;
	}

	for (bar = 0; bar < nbar; bar++) {
		pcireg_t basebar, sizebar;

		basebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), 0xffffffff);
		sizebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), basebar);

		if (sizebar == 0)
			continue;
		if (PCI_MAPREG_TYPE(sizebar) != PCI_MAPREG_TYPE_IO)
			continue;

		ok = ok && io_range_extend_by_bar(ric, bus, dev, fun,
		    PCI_BAR(bar), basebar, sizebar);
	}
	if (!ok) {
		aprint_verbose("I/O range inference failed at PCI %d.%d.%d\n",
		    bus, dev, fun);
	}
}
#endif

static bool
mmio_range_extend(struct range_infer_ctx *ric, const pci_alloc_t *pal)
{
	if (ric->ric_mmio_bottom > pal->pal_addr)
		ric->ric_mmio_bottom = pal->pal_addr;
	if (ric->ric_mmio_top < pal->pal_addr + pal->pal_size)
		ric->ric_mmio_top = pal->pal_addr + pal->pal_size;

	return pci_alloc_linkdup(&ric->ric_pals, pal);
}

static bool
mmio_range_extend_by_bar(struct range_infer_ctx *ric, int bus, int dev, int fun,
    int ofs, pcireg_t curbar, pcireg_t sizebar)
{
	int type;
	bool prefetchable;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_BAR
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r->r_ofs = ofs;
	r->r_val = curbar;

	pal.pal_addr = PCI_MAPREG_MEM_ADDR(curbar);

	type = PCI_MAPREG_MEM_TYPE(curbar);
	prefetchable = PCI_MAPREG_MEM_PREFETCHABLE(curbar);

	if (prefetchable)
		pal.pal_flags |= PCI_ALLOC_F_PREFETCHABLE;

	switch (type) {
	case PCI_MAPREG_MEM_TYPE_32BIT:
		pal.pal_size = PCI_MAPREG_MEM_SIZE(sizebar);
		break;
	case PCI_MAPREG_MEM_TYPE_64BIT:
		pal.pal_size = PCI_MAPREG_MEM64_SIZE(sizebar);
		break;
	case PCI_MAPREG_MEM_TYPE_32BIT_1M:
	default:
		aprint_debug("%s: ignored memory type %d at %d.%d.%d\n",
		    __func__, type, bus, dev, fun);
		return false;
	}

	aprint_debug("%s: %d.%d.%d base at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return (pal.pal_size == 0) || mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_vga_enable(struct range_infer_ctx *ric,
    int bus, int dev, int fun, pcireg_t csr, pcireg_t bcr)
{
	pci_alloc_reg_t *r;
	pci_alloc_t tpal = {
		  .pal_flags = PCI_ALLOC_F_PREFETCHABLE	/* XXX a guess */
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_VGA_EN
		, .pal_reg = {{
			  .r_ofs = PCI_COMMAND_STATUS_REG
			, .r_mask = PCI_COMMAND_MEM_ENABLE
		  }, {
			  .r_ofs = PCI_BRIDGE_CONTROL_REG
			, .r_mask =
			    PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT
		  }}
	}, pal;

	aprint_debug("%s: %d.%d.%d enter\n", __func__, bus, dev, fun);

	if ((csr & PCI_COMMAND_MEM_ENABLE) == 0 ||
	    (bcr & (PCI_BRIDGE_CONTROL_VGA << PCI_BRIDGE_CONTROL_SHIFT)) == 0) {
		aprint_debug("%s: %d.%d.%d memory or VGA disabled\n",
		    __func__, bus, dev, fun);
		return true;
	}

	r = &tpal.pal_reg[0];
	tpal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_val = csr;
	r[1].r_val = bcr;

	pal = tpal;

	pal.pal_addr = 0xa0000;
	pal.pal_size = 0xbffff - 0xa0000 + 1;

	return mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_win(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t mem)
{
	const int onemeg = 1024 * 1024;
	pcireg_t baser, limitr;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_WIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r->r_ofs = ofs;
	r->r_val = mem;

	baser = (mem >> PCI_BRIDGE_MEMORY_BASE_SHIFT) &
	    PCI_BRIDGE_MEMORY_BASE_MASK;
	limitr = (mem >> PCI_BRIDGE_MEMORY_LIMIT_SHIFT) &
	    PCI_BRIDGE_MEMORY_LIMIT_MASK;

	/* XXX check with the PCI standard */
	if (baser > limitr || limitr == 0)
		return true;

	pal.pal_addr = baser * onemeg;
	pal.pal_size = (limitr - baser + 1) * onemeg;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_prememwin(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t mem,
    int hibaseofs, pcireg_t hibase,
    int hilimitofs, pcireg_t hilimit)
{
	const int onemeg = 1024 * 1024;
	uint64_t baser, limitr;
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = PCI_ALLOC_F_PREFETCHABLE
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_WIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = mem;

	baser = (mem >> PCI_BRIDGE_PREFETCHMEM_BASE_SHIFT) &
	    PCI_BRIDGE_PREFETCHMEM_BASE_MASK;
	limitr = (mem >> PCI_BRIDGE_PREFETCHMEM_LIMIT_SHIFT) &
	    PCI_BRIDGE_PREFETCHMEM_LIMIT_MASK;

	if (PCI_BRIDGE_PREFETCHMEM_64BITS(mem)) {
		r[1].r_mask = r[2].r_mask = ~(pcireg_t)0;
		r[1].r_ofs = hibaseofs;
		r[1].r_val = hibase;
		r[2].r_ofs = hilimitofs;
		r[2].r_val = hilimit;

		baser |= hibase << 12;
		limitr |= hibase << 12;
	}

	/* XXX check with the PCI standard */
	if (baser > limitr || limitr == 0)
		return true;

	pal.pal_addr = baser * onemeg;
	pal.pal_size = (limitr - baser + 1) * onemeg;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return mmio_range_extend(ric, &pal);
}

static bool
mmio_range_extend_by_cbwin(struct range_infer_ctx *ric,
    int bus, int dev, int fun, int ofs, pcireg_t base, pcireg_t limit,
    bool prefetchable)
{
	pci_alloc_reg_t *r;
	pci_alloc_t pal = {
		  .pal_flags = 0
		, .pal_space = PCI_ALLOC_SPACE_MEM
		, .pal_type = PCI_ALLOC_REGTYPE_CBWIN
		, .pal_reg = {{
			  .r_mask = ~(pcireg_t)0
		  }, {
			  .r_mask = ~(pcireg_t)0
		  }}
	};

	r = &pal.pal_reg[0];

	if (prefetchable)
		pal.pal_flags |= PCI_ALLOC_F_PREFETCHABLE;

	pal.pal_tag = pci_make_tag(ric->ric_pc, bus, dev, fun);
	r[0].r_ofs = ofs;
	r[0].r_val = base;
	r[1].r_ofs = ofs + 4;
	r[1].r_val = limit;

	if (base > limit)
		return true;

	if (limit == 0)
		return true;

	pal.pal_addr = base;
	pal.pal_size = limit - base + 4096;

	aprint_debug("%s: %d.%d.%d window at %" PRIx64 " size %" PRIx64 "\n",
	    __func__, bus, dev, fun, pal.pal_addr, pal.pal_size);

	return mmio_range_extend(ric, &pal);
}

static void
mmio_range_infer(pci_chipset_tag_t pc, pcitag_t tag, void *ctx)
{
	struct range_infer_ctx *ric = ctx;
	pcireg_t bcr, bhlcr, limit, mem, premem, hiprebase, hiprelimit;
	int bar, bus, dev, fun, hdrtype, nbar;
	bool ok = true;

	bhlcr = pci_conf_read(pc, tag, PCI_BHLC_REG);

	hdrtype = PCI_HDRTYPE_TYPE(bhlcr);

	pci_decompose_tag(pc, tag, &bus, &dev, &fun);

	switch (hdrtype) {
	case PCI_HDRTYPE_PPB:
		nbar = 2;
		/* Extract memory windows */
		ok = ok && mmio_range_extend_by_win(ric, bus, dev, fun,
		    PCI_BRIDGE_MEMORY_REG,
		    pci_conf_read(pc, tag, PCI_BRIDGE_MEMORY_REG));
		premem = pci_conf_read(pc, tag, PCI_BRIDGE_PREFETCHMEM_REG);
		if (PCI_BRIDGE_PREFETCHMEM_64BITS(premem)) {
			aprint_debug("%s: 64-bit prefetchable memory window "
			    "at %d.%d.%d\n", __func__, bus, dev, fun);
			hiprebase = pci_conf_read(pc, tag,
			    PCI_BRIDGE_PREFETCHBASE32_REG);
			hiprelimit = pci_conf_read(pc, tag,
			    PCI_BRIDGE_PREFETCHLIMIT32_REG);
		} else
			hiprebase = hiprelimit = 0;
		ok = ok &&
		    mmio_range_extend_by_prememwin(ric, bus, dev, fun,
		        PCI_BRIDGE_PREFETCHMEM_REG, premem,
		        PCI_BRIDGE_PREFETCHBASE32_REG, hiprebase,
		        PCI_BRIDGE_PREFETCHLIMIT32_REG, hiprelimit) &&
		    mmio_range_extend_by_vga_enable(ric, bus, dev, fun,
		        pci_conf_read(pc, tag, PCI_COMMAND_STATUS_REG),
		        pci_conf_read(pc, tag, PCI_BRIDGE_CONTROL_REG));
		break;
	case PCI_HDRTYPE_PCB:
		/* Extract memory windows */
		bcr = pci_conf_read(pc, tag, PCI_BRIDGE_CONTROL_REG);
		mem = pci_conf_read(pc, tag, PCI_CB_MEMBASE0);
		limit = pci_conf_read(pc, tag, PCI_CB_MEMLIMIT0);
		ok = ok && mmio_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_MEMBASE0, mem, limit,
		    (bcr & CB_BCR_PREFETCH_MEMWIN0) != 0);
		mem = pci_conf_read(pc, tag, PCI_CB_MEMBASE1);
		limit = pci_conf_read(pc, tag, PCI_CB_MEMLIMIT1);
		ok = ok && mmio_range_extend_by_cbwin(ric, bus, dev, fun,
		    PCI_CB_MEMBASE1, mem, limit,
		    (bcr & CB_BCR_PREFETCH_MEMWIN1) != 0);
		nbar = 1;
		break;
	case PCI_HDRTYPE_DEVICE:
		nbar = 6;
		break;
	default:
		aprint_debug("%s: unknown header type %d at %d.%d.%d\n",
		    __func__, hdrtype, bus, dev, fun);
		return;
	}

	for (bar = 0; bar < nbar; bar++) {
		pcireg_t basebar, sizebar;

		basebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), 0xffffffff);
		sizebar = pci_conf_read(pc, tag, PCI_BAR(bar));
		pci_conf_write(pc, tag, PCI_BAR(bar), basebar);

		if (sizebar == 0)
			continue;
		if (PCI_MAPREG_TYPE(sizebar) != PCI_MAPREG_TYPE_MEM)
			continue;

		ok = ok && mmio_range_extend_by_bar(ric, bus, dev, fun,
		    PCI_BAR(bar), basebar, sizebar);
	}
	if (!ok) {
		aprint_verbose("MMIO range inference failed at PCI %d.%d.%d\n",
		    bus, dev, fun);
	}
}

static const char *
pci_alloc_regtype_string(const pci_alloc_regtype_t t)
{
	switch (t) {
	case PCI_ALLOC_REGTYPE_BAR:
		return "bar";
	case PCI_ALLOC_REGTYPE_WIN:
	case PCI_ALLOC_REGTYPE_CBWIN:
		return "window";
	case PCI_ALLOC_REGTYPE_VGA_EN:
		return "vga-enable";
	default:
		return "<unknown>";
	}
}

static void
pci_alloc_print(pci_chipset_tag_t pc, const pci_alloc_t *pal)
{
	int bus, dev, fun;
	const pci_alloc_reg_t *r;

	pci_decompose_tag(pc, pal->pal_tag, &bus, &dev, &fun);
	r = &pal->pal_reg[0];

	aprint_normal("%s range [0x%08" PRIx64 ", 0x%08" PRIx64 ")"
	    " at %d.%d.%d %s%s 0x%02x\n",
	    (pal->pal_space == PCI_ALLOC_SPACE_IO) ? "IO" : "MMIO",
	    pal->pal_addr, pal->pal_addr + pal->pal_size,
	    bus, dev, fun,
	    (pal->pal_flags & PCI_ALLOC_F_PREFETCHABLE) ? "prefetchable " : "",
	    pci_alloc_regtype_string(pal->pal_type),
	    r->r_ofs);
}

prop_dictionary_t pci_rsrc_dict = NULL;

static bool
pci_range_record(pci_chipset_tag_t pc, prop_array_t rsvns,
    pci_alloc_list_t *pals, pci_alloc_space_t space)
{
	int bus, dev, fun, i;
	prop_array_t regs;
	prop_dictionary_t reg;
	const pci_alloc_t *pal;
	const pci_alloc_reg_t *r;
	prop_dictionary_t rsvn;

	TAILQ_FOREACH(pal, pals, pal_link) {
		bool ok = true;

		r = &pal->pal_reg[0];

		if (pal->pal_space != space)
			continue;

		if ((rsvn = prop_dictionary_create()) == NULL)
			return false;

		if ((regs = prop_array_create()) == NULL) {
			prop_object_release(rsvn);
			return false;
		}

		if (!prop_dictionary_set(rsvn, "regs", regs)) {
			prop_object_release(rsvn);
			prop_object_release(regs);
			return false;
		}

		for (i = 0; i < __arraycount(pal->pal_reg); i++) {
			r = &pal->pal_reg[i];

			if (r->r_mask == 0)
				break;

			ok = (reg = prop_dictionary_create()) != NULL;
			if (!ok)
				break;

			ok = prop_dictionary_set_uint16(reg, "offset",
			        r->r_ofs) &&
			    prop_dictionary_set_uint32(reg, "val", r->r_val) &&
			    prop_dictionary_set_uint32(reg, "mask",
			        r->r_mask) && prop_array_add(regs, reg);
			if (!ok) {
				prop_object_release(reg);
				break;
			}
		}

		pci_decompose_tag(pc, pal->pal_tag, &bus, &dev, &fun);

		ok = ok &&
		    prop_dictionary_set_cstring_nocopy(rsvn, "type",
		        pci_alloc_regtype_string(pal->pal_type)) &&
		    prop_dictionary_set_uint64(rsvn, "address",
		        pal->pal_addr) &&
		    prop_dictionary_set_uint64(rsvn, "size", pal->pal_size) &&
		    prop_dictionary_set_uint8(rsvn, "bus", bus) &&
		    prop_dictionary_set_uint8(rsvn, "device", dev) &&
		    prop_dictionary_set_uint8(rsvn, "function", fun) &&
		    prop_array_add(rsvns, rsvn);
		prop_object_release(rsvn);
		if (!ok)
			return false;
	}
	return true;
}

prop_dictionary_t
pci_rsrc_filter(prop_dictionary_t rsrcs0,
    bool (*predicate)(void *, prop_dictionary_t), void *arg)
{
	int i, space;
	prop_dictionary_t rsrcs;
	prop_array_t rsvns;
	ppath_t *op, *p;

	if ((rsrcs = prop_dictionary_copy(rsrcs0)) == NULL)
		return NULL;

	for (space = 0; space < 2; space++) {
		op = p = ppath_create();
		p = ppath_push_key(p, (space == 0) ? "memory" : "io");
		p = ppath_push_key(p, "bios-reservations");
		if (p == NULL) {
			ppath_release(op);
			return NULL;
		}
		if ((rsvns = ppath_lookup(rsrcs0, p)) == NULL) {
			printf("%s: reservations not found\n", __func__);
			ppath_release(p);
			return NULL;
		}
		for (i = prop_array_count(rsvns); --i >= 0; ) {
			prop_dictionary_t rsvn;

			if ((p = ppath_push_idx(p, i)) == NULL) {
				printf("%s: ppath_push_idx\n", __func__);
				ppath_release(op);
				prop_object_release(rsrcs);
				return NULL;
			}

			rsvn = ppath_lookup(rsrcs0, p);

			KASSERT(rsvn != NULL);

			if (!(*predicate)(arg, rsvn)) {
				ppath_copydel_object((prop_object_t)rsrcs0,
				    (prop_object_t *)&rsrcs, p);
			}

			if ((p = ppath_pop(p, NULL)) == NULL) {
				printf("%s: ppath_pop\n", __func__);
				ppath_release(p);
				prop_object_release(rsrcs);
				return NULL;
			}
		}
		ppath_release(op);
	}
	return rsrcs;
}
 
void
pci_ranges_infer(pci_chipset_tag_t pc, int minbus, int maxbus,
    bus_addr_t *iobasep, bus_size_t *iosizep,
    bus_addr_t *membasep, bus_size_t *memsizep)
{
	prop_dictionary_t iodict = NULL, memdict = NULL;
	prop_array_t iorsvns, memrsvns;
	struct range_infer_ctx ric = {
		  .ric_io_bottom = ~((bus_addr_t)0)
		, .ric_io_top = 0
		, .ric_mmio_bottom = ~((bus_addr_t)0)
		, .ric_mmio_top = 0
		, .ric_pals = TAILQ_HEAD_INITIALIZER(ric.ric_pals)
	};
	const pci_alloc_t *pal;

	ric.ric_pc = pc;
	pci_device_foreach_min(pc, minbus, maxbus, mmio_range_infer, &ric);
	pci_device_foreach_min(pc, minbus, maxbus, io_range_infer, &ric);
	if (membasep != NULL)
		*membasep = ric.ric_mmio_bottom;
	if (memsizep != NULL)
		*memsizep = ric.ric_mmio_top - ric.ric_mmio_bottom;
	if (iobasep != NULL)
		*iobasep = ric.ric_io_bottom;
	if (iosizep != NULL)
		*iosizep = ric.ric_io_top - ric.ric_io_bottom;
	aprint_verbose("%s: inferred %" PRIuMAX
	    " bytes of memory-mapped PCI space at 0x%" PRIxMAX "\n", __func__,
	    (uintmax_t)(ric.ric_mmio_top - ric.ric_mmio_bottom),
	    (uintmax_t)ric.ric_mmio_bottom); 
	aprint_verbose("%s: inferred %" PRIuMAX
	    " bytes of PCI I/O space at 0x%" PRIxMAX "\n", __func__,
	    (uintmax_t)(ric.ric_io_top - ric.ric_io_bottom),
	    (uintmax_t)ric.ric_io_bottom); 
	TAILQ_FOREACH(pal, &ric.ric_pals, pal_link)
		pci_alloc_print(pc, pal);

	if ((memdict = prop_dictionary_create()) == NULL) {
		aprint_error("%s: could not create PCI MMIO "
		    "resources dictionary\n", __func__);
	} else if ((memrsvns = prop_array_create()) == NULL) {
		aprint_error("%s: could not create PCI BIOS memory "
		    "reservations array\n", __func__);
	} else if (!prop_dictionary_set(memdict, "bios-reservations",
	    memrsvns)) {
		aprint_error("%s: could not record PCI BIOS memory "
		    "reservations array\n", __func__);
	} else if (!pci_range_record(pc, memrsvns, &ric.ric_pals,
	    PCI_ALLOC_SPACE_MEM)) {
		aprint_error("%s: could not record PCI BIOS memory "
		    "reservations\n", __func__);
	} else if (!prop_dictionary_set_uint64(memdict,
	    "start", ric.ric_mmio_bottom) ||
	    !prop_dictionary_set_uint64(memdict, "size",
	     ric.ric_mmio_top - ric.ric_mmio_bottom)) {
		aprint_error("%s: could not record PCI memory min & max\n",
		    __func__);
	} else if ((iodict = prop_dictionary_create()) == NULL) {
		aprint_error("%s: could not create PCI I/O "
		    "resources dictionary\n", __func__);
	} else if ((iorsvns = prop_array_create()) == NULL) {
		aprint_error("%s: could not create PCI BIOS I/O "
		    "reservations array\n", __func__);
	} else if (!prop_dictionary_set(iodict, "bios-reservations",
	    iorsvns)) {
		aprint_error("%s: could not record PCI BIOS I/O "
		    "reservations array\n", __func__);
	} else if (!pci_range_record(pc, iorsvns, &ric.ric_pals,
	    PCI_ALLOC_SPACE_IO)) {
		aprint_error("%s: could not record PCI BIOS I/O "
		    "reservations\n", __func__);
	} else if (!prop_dictionary_set_uint64(iodict,
	    "start", ric.ric_io_bottom) ||
	    !prop_dictionary_set_uint64(iodict, "size",
	     ric.ric_io_top - ric.ric_io_bottom)) {
		aprint_error("%s: could not record PCI I/O min & max\n",
		    __func__);
	} else if ((pci_rsrc_dict = prop_dictionary_create()) == NULL) {
		aprint_error("%s: could not create PCI resources dictionary\n",
		    __func__);
	} else if (!prop_dictionary_set(pci_rsrc_dict, "memory", memdict) ||
	           !prop_dictionary_set(pci_rsrc_dict, "io", iodict)) {
		aprint_error("%s: could not record PCI memory- or I/O-"
		    "resources dictionary\n", __func__);
		prop_object_release(pci_rsrc_dict);
		pci_rsrc_dict = NULL;
	}

	if (iodict != NULL)
		prop_object_release(iodict);
	if (memdict != NULL)
		prop_object_release(memdict);
	/* XXX release iorsvns, memrsvns */
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

static const void *
bit_to_function_pointer(const struct pci_overrides *ov, uint64_t bit)
{
	switch (bit) {
	case PCI_OVERRIDE_CONF_READ:
		return ov->ov_conf_read;
	case PCI_OVERRIDE_CONF_WRITE:
		return ov->ov_conf_write;
	case PCI_OVERRIDE_INTR_MAP:
		return ov->ov_intr_map;
	case PCI_OVERRIDE_INTR_STRING:
		return ov->ov_intr_string;
	case PCI_OVERRIDE_INTR_EVCNT:
		return ov->ov_intr_evcnt;
	case PCI_OVERRIDE_INTR_ESTABLISH:
		return ov->ov_intr_establish;
	case PCI_OVERRIDE_INTR_DISESTABLISH:
		return ov->ov_intr_disestablish;
	case PCI_OVERRIDE_MAKE_TAG:
		return ov->ov_make_tag;
	case PCI_OVERRIDE_DECOMPOSE_TAG:
		return ov->ov_decompose_tag;
	default:
		return NULL;
	}
}

void
pci_chipset_tag_destroy(pci_chipset_tag_t pc)
{
	kmem_free(pc, sizeof(struct pci_chipset_tag));
}

int
pci_chipset_tag_create(pci_chipset_tag_t opc, const uint64_t present,
    const struct pci_overrides *ov, void *ctx, pci_chipset_tag_t *pcp)
{
	uint64_t bit, bits, nbits;
	pci_chipset_tag_t pc;
	const void *fp;

	if (ov == NULL || present == 0)
		return EINVAL;

	pc = kmem_alloc(sizeof(struct pci_chipset_tag), KM_SLEEP);

	if (pc == NULL)
		return ENOMEM;

	pc->pc_super = opc;

	for (bits = present; bits != 0; bits = nbits) {
		nbits = bits & (bits - 1);
		bit = nbits ^ bits;
		if ((fp = bit_to_function_pointer(ov, bit)) == NULL) {
			printf("%s: missing bit %" PRIx64 "\n", __func__, bit);
			goto einval;
		}
	}

	pc->pc_ov = ov;
	pc->pc_present = present;
	pc->pc_ctx = ctx;

	*pcp = pc;

	return 0;
einval:
	kmem_free(pc, sizeof(struct pci_chipset_tag));
	return EINVAL;
}
