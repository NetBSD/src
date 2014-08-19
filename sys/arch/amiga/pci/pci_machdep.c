/*	$NetBSD: pci_machdep.c,v 1.1.12.1 2014/08/20 00:02:43 tls Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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

/* Amiga-specific PCI MD stuff. */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/extent.h>
#include <sys/kmem.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/cpu.h>

#include <m68k/bus_dma.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/pci/mppbreg.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>
#include <dev/pci/pciconf.h>

pcitag_t
amiga_pci_make_tag(pci_chipset_tag_t pc, int bus, int device, int function)
{
	return (bus << 16) | (device << 11) | (function << 8);
}

void
amiga_pci_decompose_tag(pci_chipset_tag_t pc, pcitag_t tag, int *bp,
    int *dp, int *fp)
{
	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x07;
}

void *
amiga_pci_intr_establish(pci_chipset_tag_t pc, pci_intr_handle_t ih, int level, 
    int (*ih_fun)(void *), void *ih_arg)
{
	struct isr* pci_isr;
	pci_isr = kmem_zalloc(sizeof(struct isr), KM_SLEEP);

	/* TODO: check for bogus handle */

	pci_isr->isr_intr = ih_fun;
	pci_isr->isr_arg = ih_arg;
	pci_isr->isr_ipl = 2;		/* XXX: true for all current drivers */
	add_isr(pci_isr);
	return pci_isr;	
}

void
amiga_pci_intr_disestablish(pci_chipset_tag_t pc, void *cookie)
{
	remove_isr(cookie);
	kmem_free(cookie, sizeof(struct isr));
}

const char *
amiga_pci_intr_string(pci_chipset_tag_t pc, pci_intr_handle_t ih, char *buf,
    size_t len) 
{

	snprintf(buf, len, "INT%d", (int) ih);
	return buf;
}

int
amiga_pci_conf_hook(pci_chipset_tag_t pct, int bus, int dev, int func, 
    pcireg_t id)
{
	return PCI_CONF_DEFAULT;
}

void
amiga_pci_conf_interrupt(pci_chipset_tag_t pc, int bus, int dev, int func,
    int swiz, int *iline)
{
}

