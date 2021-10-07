/*	$NetBSD: mp.c,v 1.7 2021/10/07 12:52:27 msaitoh Exp $	*/

/*
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Frank van der Linden for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mp.c,v 1.7 2021/10/07 12:52:27 msaitoh Exp $");

#include "opt_multiprocessor.h"
#include "opt_acpi.h"

#include "acpica.h"
#include "ioapic.h"
#include "pchb.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/queue.h>

#include <machine/specialreg.h>
#include <machine/cpuvar.h>
#include <sys/bus.h>
#include <machine/mpconfig.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include "pci.h"
#include "locators.h"

#if NIOAPIC > 0 || NACPICA > 0
#include <machine/mpacpi.h>
#endif

#if NPCI > 0
#include <dev/pci/ppbreg.h>
#endif

#if NIOAPIC > 0 || NACPICA > 0
static int intr_scan_bus(int, int, intr_handle_t *);
#if NPCI > 0
static int intr_find_pcibridge(int, pcitag_t *, pci_chipset_tag_t *);
#endif
#endif

#if NPCI > 0
int
mp_pci_scan(device_t self, struct pcibus_attach_args *pba,
    cfprint_t print)
{
	int i, cnt;
	struct mp_bus *mpb;

	cnt = 0;
	for (i = 0; i < mp_nbus; i++) {
		mpb = &mp_busses[i];
		if (mpb->mb_name == NULL)
			continue;
		if (strcmp(mpb->mb_name, "pci") == 0 && mpb->mb_dev == NULL) {
			pba->pba_bus = i;
			mpb->mb_dev = config_found(self, pba, print,
			    CFARGS(.iattr = "pcibus"));
			cnt++;
		}
	}
	return cnt;
}

void
mp_pci_childdetached(device_t self, device_t child)
{
	int i;
	struct mp_bus *mpb;

	for (i = 0; i < mp_nbus; i++) {
		mpb = &mp_busses[i];
		if (mpb->mb_name == NULL)
			continue;
		if (strcmp(mpb->mb_name, "pci") == 0 && mpb->mb_dev == child)
			mpb->mb_dev = NULL;
	}
}
#endif

/*
 * List to keep track of PCI buses that are probed but not known
 * to the firmware. Used to
 *
 * XXX should maintain one list, not an array and a linked list.
 */
#if (NPCI > 0) && ((NIOAPIC > 0) || NACPICA > 0)
struct intr_extra_bus {
	int bus;
	pcitag_t *pci_bridge_tag;
	pci_chipset_tag_t pci_chipset_tag;
	LIST_ENTRY(intr_extra_bus) list;
};

LIST_HEAD(, intr_extra_bus) intr_extra_buses =
    LIST_HEAD_INITIALIZER(intr_extra_buses);


void
intr_add_pcibus(struct pcibus_attach_args *pba)
{
	struct intr_extra_bus *iebp;

	iebp = kmem_alloc(sizeof(*iebp), KM_SLEEP);
	iebp->bus = pba->pba_bus;
	iebp->pci_chipset_tag = pba->pba_pc;
	iebp->pci_bridge_tag = pba->pba_bridgetag;
	LIST_INSERT_HEAD(&intr_extra_buses, iebp, list);
}

static int
intr_find_pcibridge(int bus, pcitag_t *pci_bridge_tag,
		    pci_chipset_tag_t *pc)
{
	struct intr_extra_bus *iebp;
	struct mp_bus *mpb;

	if (bus < 0)
		return ENOENT;

	if (bus < mp_nbus) {
		mpb = &mp_busses[bus];
		if (mpb->mb_pci_bridge_tag == NULL)
			return ENOENT;
		*pci_bridge_tag = *mpb->mb_pci_bridge_tag;
		*pc = mpb->mb_pci_chipset_tag;
		return 0;
	}

	LIST_FOREACH(iebp, &intr_extra_buses, list) {
		if (iebp->bus == bus) {
			if (iebp->pci_bridge_tag == NULL)
				return ENOENT;
			*pci_bridge_tag = *iebp->pci_bridge_tag;
			*pc = iebp->pci_chipset_tag;
			return 0;
		}
	}
	return ENOENT;
}
#endif

#if NIOAPIC > 0 || NACPICA > 0
/*
 * 'pin' argument pci bus_pin encoding of a device/pin combination.
 */
int
intr_find_mpmapping(int bus, int pin, intr_handle_t *handle)
{

#if NPCI > 0
	while (intr_scan_bus(bus, pin, handle) != 0) {
		int dev, func;
		pcitag_t pci_bridge_tag;
		pci_chipset_tag_t pc;

		if (intr_find_pcibridge(bus, &pci_bridge_tag, &pc) != 0)
			return ENOENT;
		dev = pin >> 2;
		pin = pin & 3;
		pin = PPB_INTERRUPT_SWIZZLE(pin + 1, dev) - 1;
		pci_decompose_tag(pc, pci_bridge_tag, &bus, &dev, &func);
		pin |= (dev << 2);
	}
	return 0;
#else
	return intr_scan_bus(bus, pin, handle);
#endif
}

static int
intr_scan_bus(int bus, int pin, intr_handle_t *handle)
{
	struct mp_intr_map *mip, *intrs;

	if (bus < 0 || bus >= mp_nbus)
		return ENOENT;

	intrs = mp_busses[bus].mb_intrs;
	if (intrs == NULL)
		return ENOENT;

	for (mip = intrs; mip != NULL; mip = mip->next) {
		if (mip->bus_pin == pin) {
#if NACPICA > 0
			if (mip->linkdev != NULL)
				if (mpacpi_findintr_linkdev(mip) != 0)
					continue;
#endif
			*handle = mip->ioapic_ih;
			return 0;
		}
	}
	return ENOENT;
}
#endif
