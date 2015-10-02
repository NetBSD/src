/* $NetBSD: pciconf_indirect.c,v 1.6 2015/10/02 05:22:52 msaitoh Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
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
 * Generic PowerPC functions for talking to an indirect configuration style
 * PCI Bridge.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pciconf_indirect.c,v 1.6 2015/10/02 05:22:52 msaitoh Exp $");

#define _POWERPC_BUS_DMA_PRIVATE

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <uvm/uvm_extern.h>

#include <machine/pio.h>

#if NISA > 0
#include <dev/isa/isavar.h>
#endif

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#define	PCI_MODE1_ENABLE	0x80000000UL

void
genppc_pci_indirect_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{

	if (pba->pba_bus != 0)
		return;

	printf(": indirect configuration space access");
}

pcitag_t
genppc_pci_indirect_make_tag(void *v, int bus, int device, int function)
{
	pcitag_t tag;

	if (bus >= 256 || device >= 32 || function >= 8)
		panic("pci_make_tag: bad request");

	tag = PCI_MODE1_ENABLE |
		    (bus << 16) | (device << 11) | (function << 8);
	return tag;
}

void
genppc_pci_indirect_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp,
    int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
	return;
}

pcireg_t
genppc_pci_indirect_conf_read(void *v, pcitag_t tag, int reg)
{
	pci_chipset_tag_t pc = (pci_chipset_tag_t)v;
	pcireg_t data;
	int s;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	s = splhigh();
	out32rb(pc->pc_addr, tag | reg);
	data = in32rb(pc->pc_data);
	out32rb(pc->pc_addr, 0);
	splx(s);

	return data;
}

void
genppc_pci_indirect_conf_write(void *v, pcitag_t tag, int reg, pcireg_t data)
{
	pci_chipset_tag_t pc = (pci_chipset_tag_t)v;
	int s;

	if ((unsigned int)reg >= PCI_CONF_SIZE)
		return;

	s = splhigh();
	out32rb(pc->pc_addr, tag | reg);
	out32rb(pc->pc_data, data);
	out32rb(pc->pc_addr, 0);
	splx(s);
}
