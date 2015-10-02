/*	$NetBSD: vtpbc.c,v 1.10 2015/10/02 05:22:49 msaitoh Exp $	*/

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
 * Support for the V3 Semiconductor i960 PCI bus controller.  This appears
 * on some MIPS boards (notably Algorithmics P-4032 and P-5064).
 *
 * Some help was provided by the Algorithmics PMON sources.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vtpbc.c,v 1.10 2015/10/02 05:22:49 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/intr.h>
#include <machine/locore.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <algor/pci/vtpbcreg.h>
#include <algor/pci/vtpbcvar.h>

struct vtpbc_config vtpbc_configuration;

#define	PCI_CONF_LOCK(s)	(s) = splhigh()
#define	PCI_CONF_UNLOCK(s)	splx((s))

const char *vtpbc_revs[] = {
	"A",
	"B0",
	"B1",
	"B2",
	"C0",
};
const int vtpbc_nrevs = sizeof(vtpbc_revs) / sizeof(vtpbc_revs[0]);

void	vtpbc_attach_hook(device_t, device_t,
	    struct pcibus_attach_args *);
int	vtpbc_bus_maxdevs(void *, int);
pcitag_t vtpbc_make_tag(void *, int, int, int);
void	vtpbc_decompose_tag(void *, pcitag_t, int *, int *, int *);
pcireg_t vtpbc_conf_read(void *, pcitag_t, int);
void	vtpbc_conf_write(void *, pcitag_t, int, pcireg_t);

/*
 * vtpbc_init:
 *
 *	Initialize the V3 PCI controller's software state.  We
 *	simply use the existing windows that the firmware has
 *	set up for us.
 */
void
vtpbc_init(pci_chipset_tag_t pc, struct vtpbc_config *vt)
{

	pc->pc_conf_v = vt;
	pc->pc_attach_hook = vtpbc_attach_hook;
	pc->pc_bus_maxdevs = vtpbc_bus_maxdevs;
	pc->pc_make_tag = vtpbc_make_tag;
	pc->pc_decompose_tag = vtpbc_decompose_tag;
	pc->pc_conf_read = vtpbc_conf_read;
	pc->pc_conf_write = vtpbc_conf_write;

	vt->vt_rev = V96X_PCI_CC_REV(vt) & V96X_PCI_CC_REV_VREV;

	/*
	 * Determine the PCI I/O space base that our PCI
	 * I/O window maps to.  NOTE: We disable this on
	 * PBC rev < B2.
	 *
	 * Also note that PMON has disabled the I/O space
	 * if the old-style PCI address map is in-use.
	 */
	if (vt->vt_rev < V96X_VREV_B2)
		vt->vt_pci_iobase = (bus_addr_t) -1;
	else {
		if ((V96X_LB_BASE2(vt) & V96X_LB_BASEx_ENABLE) == 0)
			vt->vt_pci_iobase = (bus_addr_t) -1;
		else
			vt->vt_pci_iobase =
			    (V96X_LB_MAP2(vt) & V96X_LB_MAPx_MAP_ADR) << 16;
	}

	/*
	 * Determine the PCI memory space base that our PCI
	 * memory window maps to.
	 */
	vt->vt_pci_membase = (V96X_LB_MAP1(vt) & V96X_LB_MAPx_MAP_ADR) << 16;

	/*
	 * Determine the PCI window base that maps host RAM for
	 * DMA.
	 */
	vt->vt_dma_winbase = V96X_PCI_BASE1(vt) & 0xfffffff0;
}

void
vtpbc_attach_hook(device_t parent, device_t self,
    struct pcibus_attach_args *pba)
{
}

int
vtpbc_bus_maxdevs(void *v, int busno)
{

	return (32);
}

pcitag_t
vtpbc_make_tag(void *v, int b, int d, int f)
{

	return ((b << 16) | (d << 11) | (f << 8));
}

void
vtpbc_decompose_tag(void *v, pcitag_t tag, int *bp, int *dp, int *fp)
{

	if (bp != NULL)
		*bp = (tag >> 16) & 0xff;
	if (dp != NULL)
		*dp = (tag >> 11) & 0x1f;
	if (fp != NULL)
		*fp = (tag >> 8) & 0x7;
}

static int
vtpbc_conf_addr(struct vtpbc_config *vt, pcitag_t tag, int offset,
    u_int32_t *cfgoff, u_int32_t *ad_low)
{
	int b, d, f;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (1);

	vtpbc_decompose_tag(vt, tag, &b, &d, &f);

	if (b == 0) {
		if (d > (31 - vt->vt_adbase))
			return (1);
		*cfgoff = (1UL << (d + vt->vt_adbase)) | (f << 8) |
		    offset;
		*ad_low = 0;
	} else if (vt->vt_rev >= V96X_VREV_C0) {
		*cfgoff = tag | offset;
		*ad_low = V96X_LB_MAPx_AD_LOW_EN;
	} else
		return (1);

	return (0);
}

pcireg_t
vtpbc_conf_read(void *v, pcitag_t tag, int offset)
{
	struct vtpbc_config *vt = v;
	pcireg_t data;
	u_int32_t cfgoff, ad_low;
	int s;
	u_int16_t errbits;

	if (vtpbc_conf_addr(vt, tag, offset, &cfgoff, &ad_low))
		return ((pcireg_t) -1);

	PCI_CONF_LOCK(s);

	/* high 12 bits of address go into map register */
	V96X_LB_MAP0(vt) = ((cfgoff >> 16) & V96X_LB_MAPx_MAP_ADR) |
	    ad_low | V96X_LB_TYPE_CONF;

	/* clear aborts */
	V96X_PCI_STAT(vt) |= V96X_PCI_STAT_M_ABORT | V96X_PCI_STAT_T_ABORT;

	wbflush();

	/* low 20 bits of address are offset into config space */
	data = *(volatile u_int32_t *) (vt->vt_cfgbase + (cfgoff & 0xfffff));

	errbits = V96X_PCI_STAT(vt) &
	    (V96X_PCI_STAT_M_ABORT|V96X_PCI_STAT_T_ABORT);
	if (errbits) {
		V96X_PCI_STAT(vt) |= errbits;
		data = (pcireg_t) -1;
	}

	PCI_CONF_UNLOCK(s);

	return (data);
}

void
vtpbc_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	struct vtpbc_config *vt = v;
	u_int32_t cfgoff, ad_low;
	int s;

	if (vtpbc_conf_addr(vt, tag, offset, &cfgoff, &ad_low))
		panic("vtpbc_conf_write");

	PCI_CONF_LOCK(s);

	/* high 12 bits of address go into map register */
	V96X_LB_MAP0(vt) = ((cfgoff >> 16) & V96X_LB_MAPx_MAP_ADR) |
	    ad_low | V96X_LB_TYPE_CONF;

	/* clear aborts */
	V96X_PCI_STAT(vt) |= V96X_PCI_STAT_M_ABORT | V96X_PCI_STAT_T_ABORT;

	wbflush();

	/* low 20 bits of address are offset into config space */
	*(volatile u_int32_t *) (vt->vt_cfgbase + (cfgoff & 0xfffff)) = data;

	/* wait for FIFO to drain */
	while (V96X_FIFO_STAT(vt) & V96X_FIFO_STAT_L2P_WR)
		/* spin */ ;

	PCI_CONF_UNLOCK(s);
}
