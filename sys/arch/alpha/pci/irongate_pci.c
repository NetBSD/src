/* $NetBSD: irongate_pci.c,v 1.9.24.1 2015/12/27 12:09:28 skrll Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * PCI Configuration Space support for the AMD 751 (``Irongate'') core logic
 * chipset.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: irongate_pci.c,v 1.9.24.1 2015/12/27 12:09:28 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/irongatereg.h>
#include <alpha/pci/irongatevar.h>

void		irongate_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		irongate_bus_maxdevs(void *, int);
pcitag_t	irongate_make_tag(void *, int, int, int);
void		irongate_decompose_tag(void *, pcitag_t, int *, int *,
		    int *);
pcireg_t	irongate_conf_read(void *, pcitag_t, int);
void		irongate_conf_write(void *, pcitag_t, int, pcireg_t);

/* AMD 751 systems are always single-processor, so this is easy. */
#define	PCI_CONF_LOCK(s)	(s) = splhigh()
#define	PCI_CONF_UNLOCK(s)	splx((s))

#define	PCI_CONF_ADDR	(IRONGATE_IO_BASE|IRONGATE_CONFADDR)
#define	PCI_CONF_DATA	(IRONGATE_IO_BASE|IRONGATE_CONFDATA)

#define	REGVAL(r)	(*(volatile uint32_t *)ALPHA_PHYS_TO_K0SEG(r))

void
irongate_pci_init(pci_chipset_tag_t pc, void *v)
{

	pc->pc_conf_v = v;
	pc->pc_attach_hook = irongate_attach_hook;
	pc->pc_bus_maxdevs = irongate_bus_maxdevs;
	pc->pc_make_tag = irongate_make_tag;
	pc->pc_decompose_tag = irongate_decompose_tag;
	pc->pc_conf_read = irongate_conf_read;
	pc->pc_conf_write = irongate_conf_write;
}

void
irongate_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
irongate_bus_maxdevs(void *ipv, int busno)
{

	return 32;
}

pcitag_t
irongate_make_tag(void *ipv, int b, int d, int f)
{

	return (b << 16) | (d << 11) | (f << 8);
}

void
irongate_decompose_tag(void *ipv, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

pcireg_t
irongate_conf_read(void *ipv, pcitag_t tag, int offset)
{
	int d;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	/*
	 * The AMD 751 appears in PCI configuration space, but
	 * that is ... counter-intuitive to the way we normally
	 * attach PCI-Host bridges on the Alpha.  So, filter out
	 * the AMD 751 device here.  We provide a private entry
	 * point for getting at it from machdep code.
	 */
	irongate_decompose_tag(ipv, tag, NULL, &d, NULL);
	if (d == IRONGATE_PCIHOST_DEV && offset == PCI_ID_REG)
		return ((pcireg_t) -1);
	
	return (irongate_conf_read0(ipv, tag, offset));
}

pcireg_t
irongate_conf_read0(void *ipv, pcitag_t tag, int offset)
{
	pcireg_t data;
	int s;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	PCI_CONF_LOCK(s);
	REGVAL(PCI_CONF_ADDR) = (CONFADDR_ENABLE | tag | (offset & 0xff));
	alpha_mb();
	data = REGVAL(PCI_CONF_DATA);
	REGVAL(PCI_CONF_ADDR) = 0;
	alpha_mb();
	PCI_CONF_UNLOCK(s);

	return (data);
}

void
irongate_conf_write(void *ipv, pcitag_t tag, int offset, pcireg_t data)
{
	int s;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return;

	PCI_CONF_LOCK(s);
	REGVAL(PCI_CONF_ADDR) = (CONFADDR_ENABLE | tag | (offset & 0xff));
	alpha_mb();
	REGVAL(PCI_CONF_DATA) = data;
	alpha_mb();
	REGVAL(PCI_CONF_ADDR) = 0;
	alpha_mb();
	PCI_CONF_UNLOCK(s);
}
