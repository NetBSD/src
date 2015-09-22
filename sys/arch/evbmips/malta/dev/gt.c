/*	$NetBSD: gt.c,v 1.13.30.1 2015/09/22 12:05:41 skrll Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe and Simon Burge for Wasabi Systems, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: gt.c,v 1.13.30.1 2015/09/22 12:05:41 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/pci/pcivar.h>

#include <mips/cpuregs.h>

#include <evbmips/malta/maltareg.h>
#include <evbmips/malta/maltavar.h>

#include <evbmips/malta/dev/gtreg.h>
#include <evbmips/malta/dev/gtvar.h>

#include "pci.h"

/*
 * Galileo systems (so far) are always single-processor, so this is sufficient.
 */
#define	PCI_CONF_LOCK(s)	(s) = splhigh()
#define	PCI_CONF_UNLOCK(s)	splx((s))

static void	gt_attach_hook(device_t, device_t, struct pcibus_attach_args *);
static int	gt_bus_maxdevs(void *, int);
static pcitag_t	gt_make_tag(void *, int, int, int);
static void	gt_decompose_tag(void *, pcitag_t, int *, int *, int *);
static pcireg_t	gt_conf_read(void *, pcitag_t, int);
static void	gt_conf_write(void *, pcitag_t, int, pcireg_t);

void
gt_pci_init(pci_chipset_tag_t pc, struct gt_config *mcp)
{

	pc->pc_conf_v = mcp;
	pc->pc_attach_hook = gt_attach_hook;
	pc->pc_bus_maxdevs = gt_bus_maxdevs;
	pc->pc_make_tag = gt_make_tag;
	pc->pc_decompose_tag = gt_decompose_tag;
	pc->pc_conf_read = gt_conf_read;
	pc->pc_conf_write = gt_conf_write;
}

static void
gt_attach_hook(device_t parent, device_t self, struct pcibus_attach_args *pba)
{

	/* Nothing to do... */
}

static int	gt_match(device_t, cfdata_t, void *);
static void	gt_attach(device_t, device_t, void *);
static int	gt_print(void *aux, const char *pnp);

CFATTACH_DECL_NEW(gt, 0,
    gt_match, gt_attach, NULL, NULL);

static int
gt_match(device_t parent, cfdata_t match, void *aux)
{
	return 1;
}

static void
gt_attach(device_t parent, device_t self, void *aux)
{
	struct malta_config *mcp = &malta_configuration;
	struct pcibus_attach_args pba;

	printf("\n");

#if NPCI > 0
	pba.pba_flags = PCI_FLAGS_IO_OKAY | PCI_FLAGS_MEM_OKAY;
	pba.pba_bus = 0;
	pba.pba_bridgetag = NULL;
	pba.pba_iot = &mcp->mc_iot;
	pba.pba_memt = &mcp->mc_memt;
	pba.pba_dmat = &mcp->mc_pci_dmat;	/* pci_bus_dma_tag */
	pba.pba_dmat64 = NULL;
	pba.pba_pc = &mcp->mc_pc;

	config_found_ia(self, "pcibus", &pba, gt_print);
#endif
}

static int
gt_print(void *aux, const char *pnp)
{
	/* XXX */
	return 0;
}

static int
gt_bus_maxdevs(void *v, int busno)
{

	/* The galileo has problems accessing device 31. */
	if (busno == 0)
		return (31);
	return (32);
}

static pcitag_t
gt_make_tag(void *v, int b, int d, int f)
{

	return ((b << 16) | (d << 11) | (f << 8));
}

static void
gt_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

static pcireg_t
gt_conf_read(void *v, pcitag_t tag, int offset)
{
	pcireg_t data;
	int bus, dev, func, s;

	gt_decompose_tag(NULL /* XXX */, tag, &bus, &dev, &func);

	/* The galileo has problems accessing device 31. */
	if (bus == 0 && dev == 31)
		return ((pcireg_t) -1);

	/* XXX: no support for bus > 0 yet */
	if (bus > 0)
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);

	/* Clear cause register bits. */
	GT_REGVAL(GT_INTR_CAUSE) = 0;

	GT_REGVAL(GT_PCI0_CFG_ADDR) = (1 << 31) | tag | offset;
	data = GT_REGVAL(GT_PCI0_CFG_DATA);

	/* Check for master abort. */
	if (GT_REGVAL(GT_INTR_CAUSE) & (GTIC_MASABORT0 | GTIC_TARABORT0))
		data = (pcireg_t) -1;

	PCI_CONF_UNLOCK(s);

	return (data);
}

static void
gt_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	int bus, dev, func, s;

	gt_decompose_tag(NULL /* XXX */, tag, &bus, &dev, &func);

	/* The galileo has problems accessing device 31. */
	if (bus == 0 && dev == 31)
		return;

	/* XXX: no support for bus > 0 yet */
	if (bus > 0)
		return;

	PCI_CONF_LOCK(s);

	/* Clear cause register bits. */
	GT_REGVAL(GT_INTR_CAUSE) = 0;

	GT_REGVAL(GT_PCI0_CFG_ADDR) = (1 << 31) | tag | offset;
	GT_REGVAL(GT_PCI0_CFG_DATA) = data;

	PCI_CONF_UNLOCK(s);
}
