/*	$NetBSD: bonito_pci.c,v 1.4.78.2 2009/08/19 18:46:29 yamt Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 * PCI configuration space support for the Algorithmics BONITO
 * MIPS PCI and memory controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bonito_pci.c,v 1.4.78.2 2009/08/19 18:46:29 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/locore.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

/*
 * Bonito systems are always single-processor, so this is sufficient.
 */
#define	PCI_CONF_LOCK(s)	(s) = splhigh()
#define	PCI_CONF_UNLOCK(s)	splx((s))

void		bonito_attach_hook(device_t, device_t,
		    struct pcibus_attach_args *);
int		bonito_bus_maxdevs(void *, int);
pcitag_t	bonito_make_tag(void *, int, int, int);
void		bonito_decompose_tag(void *, pcitag_t, int *, int *, int *);
pcireg_t	bonito_conf_read(void *, pcitag_t, int);
void		bonito_conf_write(void *, pcitag_t, int, pcireg_t);

void
bonito_pci_init(pci_chipset_tag_t pc, struct bonito_config *bc)
{

	pc->pc_conf_v = bc;
	pc->pc_attach_hook = bonito_attach_hook;
	pc->pc_bus_maxdevs = bonito_bus_maxdevs;
	pc->pc_make_tag = bonito_make_tag;
	pc->pc_decompose_tag = bonito_decompose_tag;
	pc->pc_conf_read = bonito_conf_read;
	pc->pc_conf_write = bonito_conf_write;
}

void
bonito_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
bonito_bus_maxdevs(void *v, int busno)
{

	return (32);
}

pcitag_t
bonito_make_tag(void *v, int b, int d, int f)
{

	return ((b << 16) | (d << 11) | (f << 8));
}

void
bonito_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

static bool
bonito_conf_addr(struct bonito_config *bc, pcitag_t tag, int offset,
    u_int32_t *cfgoff, u_int32_t *pcimap_cfg)
{
	int b, d, f;

	bonito_decompose_tag(bc, tag, &b, &d, &f);

	if (b == 0) {
		if (d > (31 - bc->bc_adbase))
			return true;
		*cfgoff = (1UL << (d + bc->bc_adbase)) | (f << 8) | offset;
		*pcimap_cfg = 0;
	} else {
		*cfgoff = tag | offset;
		*pcimap_cfg = BONITO_PCIMAPCFG_TYPE1;
	}

	return false;
}

pcireg_t
bonito_conf_read(void *v, pcitag_t tag, int offset)
{
	struct bonito_config *bc = v;
	pcireg_t data;
	u_int32_t cfgoff, dummy, pcimap_cfg;
	int s;

	if (bonito_conf_addr(bc, tag, offset, &cfgoff, &pcimap_cfg))
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);

	/* clear aborts */
	REGVAL(BONITO_PCICMD) |=
	    PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;

	/* high 16 bits of address go into PciMapCfg register */
	REGVAL(BONITO_PCIMAP_CFG) = (cfgoff >> 16) | pcimap_cfg;

	wbflush();
	/* Issue a read to make sure the write is posted */
	dummy = REGVAL(BONITO_PCIMAP_CFG);

	/* low 16 bits of address are offset into config space */
	data = REGVAL(BONITO_PCICFG_BASE + (cfgoff & 0xfffc));

	/* check for error */
	if (REGVAL(BONITO_PCICMD) &
	    (PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT))
		data = (pcireg_t) -1;

	PCI_CONF_UNLOCK(s);

	return (data);
}

void
bonito_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	struct bonito_config *vt = v;
	u_int32_t cfgoff, dummy, pcimap_cfg;
	int s;

	if (bonito_conf_addr(vt, tag, offset, &cfgoff, &pcimap_cfg))
		panic("bonito_conf_write");

	PCI_CONF_LOCK(s);

	/* clear aborts */
	REGVAL(BONITO_PCICMD) |=
	    PCI_STATUS_MASTER_ABORT | PCI_STATUS_MASTER_TARGET_ABORT;

	/* high 16 bits of address go into PciMapCfg register */
	REGVAL(BONITO_PCIMAP_CFG) = (cfgoff >> 16) | pcimap_cfg;

	wbflush();
	/* Issue a read to make sure the write is posted */
	dummy = REGVAL(BONITO_PCIMAP_CFG);

	/* low 16 bits of address are offset into config space */
	REGVAL(BONITO_PCICFG_BASE + (cfgoff & 0xfffc)) = data;

	PCI_CONF_UNLOCK(s);
}
