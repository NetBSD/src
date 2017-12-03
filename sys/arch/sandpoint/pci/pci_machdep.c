/*	$NetBSD: pci_machdep.c,v 1.31.6.2 2017/12/03 11:36:40 jdolecek Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.31.6.2 2017/12/03 11:36:40 jdolecek Exp $");

#include "opt_pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/extent.h>
#include <sys/kmem.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>
#include <sys/time.h>

#define _POWERPC_BUS_DMA_PRIVATE
#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <dev/isa/isavar.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pciconf.h>
#include <dev/pci/pcidevs.h>

struct powerpc_bus_dma_tag pci_bus_dma_tag = {
	0,			/* _bounce_thresh */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	NULL,			/* _dmamap_sync */
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
};

/*#define EPIC_DEBUGIRQ*/

static int brdtype;
#define BRD_SANDPOINTX2		2
#define BRD_SANDPOINTX3		3
#define BRD_ENCOREPP1		10
#define BRD_KUROBOX		100
#define BRD_QNAPTS		101
#define BRD_SYNOLOGY		102
#define BRD_STORCENTER		103
#define BRD_DLINKDSM		104
#define BRD_NH230NAS		105
#define BRD_UNKNOWN		-1

#define	PCI_CONFIG_ENABLE	0x80000000UL

void
pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
	pcitag_t tag;
	pcireg_t dev11, dev22, dev15, dev13, dev16;

	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 11, 0);
	dev11 = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(dev11) == PCI_CLASS_BRIDGE) {
		/* WinBond/Symphony Lab 83C553 at dev 11 */
		/*
		 * XXX distinguish SP3 from SP2 by fiddling ISA GPIO #7/6.
		 * XXX SP3 #7 output values loopback to #6 input.
		 */
		brdtype = BRD_SANDPOINTX3;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 22, 0);
	dev22 = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(dev22) == PCI_CLASS_BRIDGE) {
		/* VIA 82C686B at dev 22 */
		brdtype = BRD_ENCOREPP1;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 11, 0);
	dev11 = pci_conf_read(pba->pba_pc, tag, PCI_CLASS_REG);
	if (PCI_CLASS(dev11) == PCI_CLASS_NETWORK) {
		/* tlp (ADMtek AN985) or re (RealTek 8169S) at dev 11 */
		brdtype = BRD_KUROBOX;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 15, 0);
	dev15 = pci_conf_read(pba->pba_pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(dev15) == PCI_VENDOR_MARVELL) {
		/* Marvell GbE at dev 15 */
		brdtype = BRD_SYNOLOGY;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 13, 0);
	dev13 = pci_conf_read(pba->pba_pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(dev13) == PCI_VENDOR_VIATECH) {
		/* VIA 6410 PCIIDE at dev 13 */
		brdtype = BRD_STORCENTER;
		return;
	}
	tag = pci_make_tag(pba->pba_pc, pba->pba_bus, 16, 0);
	dev16 = pci_conf_read(pba->pba_pc, tag, PCI_ID_REG);
	if (PCI_VENDOR(dev16) == PCI_VENDOR_ACARD) {
		/* ACARD ATP865 at dev 16 */
		brdtype = BRD_DLINKDSM;
		return;
	}
	if (PCI_VENDOR(dev16) == PCI_VENDOR_ITE
	    || PCI_VENDOR(dev16) == PCI_VENDOR_CMDTECH) {
		brdtype = BRD_NH230NAS;
		return;
	}
	if (PCI_VENDOR(dev15) == PCI_VENDOR_INTEL
	    || PCI_VENDOR(dev15) == PCI_VENDOR_REALTEK) {
		/* Intel or Realtek GbE at dev 15 */
		brdtype = BRD_QNAPTS;
		return;
	}

	brdtype = BRD_UNKNOWN;
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return 32;
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag = PCI_CONFIG_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
	return tag;
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag,
    int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
	return;
}

/*
 * The Kahlua documentation says that "reg" should be left-shifted by two
 * and be in bits 2-7.  Apparently not.  It doesn't work that way, and the
 * DINK32 ROM doesn't do it that way (I peeked at 0xfec00000 after running
 * the DINK32 "pcf" command).
 */
pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{
	pcireg_t data;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	out32rb(SANDPOINT_PCI_CONFIG_ADDR, tag | reg);
	data = in32rb(SANDPOINT_PCI_CONFIG_DATA);
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0);
	return data;
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	out32rb(SANDPOINT_PCI_CONFIG_ADDR, tag | reg);
	out32rb(SANDPOINT_PCI_CONFIG_DATA, data);
	out32rb(SANDPOINT_PCI_CONFIG_ADDR, 0);
}

int
pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	int	pin = pa->pa_intrpin;
	int	line = pa->pa_intrline;

	/* No IRQ used. */
	if (pin == 0)
		goto bad;
	if (pin > 4) {
		aprint_error("pci_intr_map: bad interrupt pin %d\n", pin);
		goto bad;
	}

	/*
	 * Section 6.2.4, `Miscellaneous Functions', says that 255 means
	 * `unknown' or `no connection' on a PC.  We assume that a device with
	 * `no connection' either doesn't have an interrupt (in which case the
	 * pin number should be 0, and would have been noticed above), or
	 * wasn't configured by the BIOS (in which case we punt, since there's
	 * no real way we can know how the interrupt lines are mapped in the
	 * hardware).
	 *
	 * XXX
	 * Since IRQ 0 is only used by the clock, and we can't actually be sure
	 * that the BIOS did its job, we also recognize that as meaning that
	 * the BIOS has not configured the device.
	 */
	if (line == 255) {
		aprint_error("pci_intr_map: no mapping for pin %c\n",
		    '@' + pin);
		goto bad;
	}
#ifdef EPIC_DEBUGIRQ
	printf("line %d, pin %c", line, pin + '@');
#endif
	switch (brdtype) {
	/* Sandpoint has 4 PCI slots in a weird order.
	 * From next to MPMC mezzanine card toward the board edge,
	 * 	64bit slot PCI AD14
	 * 	64bit slot PCI AD13
	 * 	32bit slot PCI AD16
	 * 	32bit slot PCI AD15
	 * Don't believe identifying labels printed on PCB and
	 * documents confusing as well since Moto names the slots
	 * as number 1 origin.
	 */
	case BRD_SANDPOINTX3:
	/*
	 * Sandpoint X3 brd uses EPIC serial mode IRQ.
	 * - i8259 PIC interrupt to EPIC IRQ0.
	 * - WinBond IDE PCI C/D to EPIC IRQ8/9.
	 * - PCI AD13 pin A to EPIC IRQ2.
	 * - PCI AD14 pin A to EPIC IRQ3.
	 * - PCI AD15 pin A to EPIC IRQ4.
	 * - PCI AD16 pin A to EPIC IRQ5.
	 */
		if (line == 11
		    && pa->pa_function == 1 && pa->pa_bus == 0) {
			/* X3 wires 83c553 pin C,D to EPIC IRQ8,9 */
			*ihp = 8; /* pin C only, indeed */
			break;
		}
		if (line < 13 || line > 16) {
			aprint_error("pci_intr_map: bad interrupt line %d,%c\n",
				line, pin + '@');
			goto bad;
		}
		line -= 13; /* B/C/D is not available */
		*ihp = 2 + line;
		break;
	case BRD_SANDPOINTX2:
	/*
	 * Sandpoint X2 brd uses EPIC direct mode IRQ.
	 * - i8259 PIC interrupt EPIC IRQ2.
	 * - PCI AD13 pin A,B,C,D to EPIC IRQ0,1,2,3.
	 * - PCI AD14 pin A,B,C,D to EPIC IRQ1,2,3,0.
	 * - PCI AD15 pin A,B,C,D to EPIC IRQ2,3,0,1.
	 * - PCI AD16 pin A,B,C,D to EPIC IRQ3,0,1,2.
	 * - PCI AD12 is wired to PMPC device itself.
	 */
		if (line == 11
		    && pa->pa_function == 1 && pa->pa_bus == 0) {
			/* 83C553 PCI IDE comes thru EPIC IRQ2 */
			*ihp = 2;
			break;
		}
		if (line < 13 || line > 16) {
			aprint_error("pci_intr_map: bad interrupt line %d,%c\n",
				line, pin + '@');
			goto bad;
		}
		line -= 13; pin -= 1;
		*ihp = (line + pin) & 03;
		break;
	case BRD_ENCOREPP1:
	/*
	 * Ampro EnCorePP1 brd uses EPIC direct mode IRQ.
	 * PDF says VIA 686B SB i8259 interrupt goes through EPC IRQ0,
	 * while  PCI pin A-D are tied with EPIC IRQ1-4.
	 *
	 * It mentions i82559 is at AD24, however, found at AD25 instead.
	 * Heuristics show that i82559 responds to EPIC 2 (!).  Then we
	 * decided to return EPIC 2 here since i82559 is the only one PCI
	 * device ENCPP1 can have;
	 */
		if (pa->pa_device != 25)
			goto bad; /* eeh !? */
		*ihp = 2;
		break;
	case BRD_KUROBOX:
		/* map line 11,12,13,14 to EPIC IRQ 0,1,4,3 */
		*ihp = (line == 13) ? 4 : line - 11;
		break;
	case BRD_QNAPTS:
		/* map line 13-16 to EPIC IRQ0-3 */
		*ihp = line - 13;
		break;
	case BRD_SYNOLOGY:
		/* map line 12,13-15 to EPIC IRQ 4,0-2 */
		*ihp = (line == 12) ? 4 : line - 13;
		break;
	case BRD_DLINKDSM:
		/* map line 13,14A,14B,14C,15,16 to EPIC IRQ 0,1,1,2,3,4 */
		*ihp = (line < 15) ? line - 13 : line - 12;
		if (line == 14 && pin == 3)
			*ihp += 1;	/* USB pin C (EHCI) uses next IRQ */
		break;
	case BRD_NH230NAS:
		/* map line 13,14,15,16 to EPIC IRQ0,3,1,2 */
		*ihp =  (line == 16) ? 2 :
			(line == 15) ? 1 :
			(line == 14) ? 3 : 0;
		break;
	case BRD_STORCENTER:
		/* map line 13,14A,14B,14C,15 to EPIC IRQ 1,2,3,4,0 */
		*ihp =	(line == 15) ? 0 :
			(line == 13) ? 1 : 1 + pin;
		break;
	default:
		/* simply map line 12-15 to EPIC IRQ0-3 */
		*ihp = line - 12;
#if defined(DIAGNOSTIC) || defined(DEBUG)
		printf("pci_intr_map: line %d, pin %c for unknown board"
		    " mapped to irq %d\n", line, pin + '@', *ihp);
#endif
		break;
	}
#ifdef EPIC_DEBUGIRQ
	printf(" = EPIC %d\n", *ihp);
#endif
	return 0;
  bad:
	*ihp = -1;
	return 1;
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	if (ih < 0 || ih >= OPENPIC_ICU)
		panic("pci_intr_string: bogus handle 0x%x", ih);

	snprintf(buf, len, "irq %d", ih + I8259_ICU);
	return buf;
	
}

const struct evcnt *
pci_intr_evcnt(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

int
pci_intr_setattr(pci_chipset_tag_t pc, pci_intr_handle_t *ih,
		 int attr, uint64_t data)
{

	switch (attr) {
	case PCI_INTR_MPSAFE:
		return 0;
	default:
		return ENODEV;
	}
}

void *
pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg)
{

	return pci_intr_establish_xname(pc, ih, level, func, arg, NULL);
}

void *
pci_intr_establish_xname(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg, const char *xname)
{
	int type;

	if (brdtype == BRD_STORCENTER && ih == 1) {
		/*
		 * XXX This is a workaround for the VT6410 IDE controller!
		 * Apparently its interrupt cannot be disabled and remains
		 * asserted during the whole device probing procedure,
		 * causing an interrupt storm.
		 * Using an edge-trigger fixes that and triggers the
		 * interrupt only once during probing.
		 */
		type = IST_EDGE;
	} else
		type = IST_LEVEL;

	/*
	 * ih is the value assigned in pci_intr_map(), above.
	 * It's the EPIC IRQ #.
	 */
	return intr_establish_xname(ih + I8259_ICU, type, level, func, arg,
	    xname);
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	intr_disestablish(cookie);
}

#if defined(PCI_NETBSD_CONFIGURE)
void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev,
    int pin, int swiz, int *iline)
{
	if (bus == 0) {
		*iline = dev;
	} else {
		/*
		 * If we are not on bus zero, we're behind a bridge, so we
		 * swizzle.
		 *
		 * The documentation lies about this.  In slot 3 (numbering
		 * from 0) aka device 16, INTD# becomes an interrupt for
		 * slot 2.  INTC# becomes an interrupt for slot 1, etc.
		 * In slot 2 aka device 16, INTD# becomes an interrupt for
		 * slot 1, etc.
		 *
		 * Verified for INTD# on device 16, INTC# on device 16,
		 * INTD# on device 15, INTD# on device 13, and INTC# on
		 * device 14.  I presume that the rest follow the same
		 * pattern.
		 *
		 * Slot 0 is device 13, and is the base for the rest.
		 */
		*iline = 13 + ((swiz + dev + 3) & 3);
	}
}
#endif

pci_intr_type_t
pci_intr_type(pci_chipset_tag_t pc, pci_intr_handle_t ih)
{

	return PCI_INTR_TYPE_INTX;
}

int
pci_intr_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *counts, pci_intr_type_t max_type)
{

	if (counts != NULL && counts[PCI_INTR_TYPE_INTX] == 0)
		return EINVAL;

	return pci_intx_alloc(pa, ihps);
}

void
pci_intr_release(pci_chipset_tag_t pc, pci_intr_handle_t *pih, int count)
{

	kmem_free(pih, sizeof(*pih));
}

int
pci_intx_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihpp)
{
	pci_intr_handle_t *ihp;

	ihp = kmem_alloc(sizeof(*ihp), KM_SLEEP);
	if (pci_intr_map(pa, ihp)) {
		kmem_free(ihp, sizeof(*ihp));
		return EINVAL;
	}

	*ihpp = ihp;
	return 0;
}

/* experimental MSI support */
int
pci_msi_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count)
{

	return EOPNOTSUPP;
}

int
pci_msi_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int count)
{

	return EOPNOTSUPP;
}

/* experimental MSI-X support */
int
pci_msix_alloc(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int *count)
{

	return EOPNOTSUPP;
}

int
pci_msix_alloc_exact(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    int count)
{

	return EOPNOTSUPP;
}

int
pci_msix_alloc_map(const struct pci_attach_args *pa, pci_intr_handle_t **ihps,
    u_int *table_indexes, int count)
{

	return EOPNOTSUPP;
}
