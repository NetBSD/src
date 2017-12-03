/*	$NetBSD: pci_machdep.c,v 1.22.12.3 2017/12/03 11:36:41 jdolecek Exp $	*/

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
 *          NetBSD Project.  See http://www.NetBSD.org/ for
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
__KERNEL_RCSID(0, "$NetBSD: pci_machdep.c,v 1.22.12.3 2017/12/03 11:36:41 jdolecek Exp $");

#include "opt_pci.h"
#include "pci.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <uvm/uvm_extern.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/sysconf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

struct mips_bus_dma_tag pci_bus_dma_tag;

void
pci_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{
	/*
	 * PCI doesn't have any special needs; just use
	 * the generic versions of these functions as
	 * established in sgimips_bus_dma_init().
	 */
	pci_bus_dma_tag = sgimips_default_bus_dma_tag;	/* struct copy */

	/* XXX */

	return;
}

int
pci_bus_maxdevs(pci_chipset_tag_t pc, int busno)
{

	return (*pc->pc_bus_maxdevs)(pc, busno);
}

pcitag_t
pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{

	return (bus << 16) | (device << 11) | (function << 8);
}

void
pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

pcireg_t
pci_conf_read(pci_chipset_tag_t pc, pcitag_t tag, int reg)
{

	return (*pc->pc_conf_read)(pc, tag, reg);
}

void
pci_conf_write(pci_chipset_tag_t pc, pcitag_t tag, int reg, pcireg_t data)
{

	(*pc->pc_conf_write)(pc, tag, reg, data);
}

int
pci_intr_map(const struct pci_attach_args *pa, pci_intr_handle_t *ihp)
{
	return (*pa->pa_pc->pc_intr_map)(pa, ihp);
}

const char *
pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len)
{
	return (*pc->pc_intr_string)(pc, ih, buf, len);
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

	return (void *)(*pc->intr_establish)(ih, 0, func, arg);
}

void
pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{

	(*pc->intr_disestablish)(cookie);
}

#ifdef PCI_NETBSD_CONFIGURE
int
pci_conf_hook(pci_chipset_tag_t pc, int bus, int device, int function,
    pcireg_t id)
{

	if (pc->pc_conf_hook)
		return (*pc->pc_conf_hook)(pc, bus, device, function, id);
	else
		return (PCI_CONF_DEFAULT);
}

void
pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int pin, int swiz,
    int *iline)
{

	return;
}
#endif
