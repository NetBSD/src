/* $NetBSD: dwlpx_pci.c,v 1.6.2.2 1997/06/01 04:13:14 cgd Exp $ */

/*
 * Copyright (c) 1997 by Matthew Jacob
 * NASA AMES Research Center.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <machine/options.h>		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: dwlpx_pci.c,v 1.6.2.2 1997/06/01 04:13:14 cgd Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <vm/vm.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/tlsb/tlsbreg.h>
#include <alpha/pci/dwlpxreg.h>
#include <alpha/pci/dwlpxvar.h>

#define	DO_SECONDARIES	1

#define	KV(_addr)	((caddr_t)ALPHA_PHYS_TO_K0SEG((_addr)))

void		dwlpx_attach_hook __P((struct device *, struct device *,
		    struct pcibus_attach_args *));
int		dwlpx_bus_maxdevs __P((void *, int));
pcitag_t	dwlpx_make_tag __P((void *, int, int, int));
void		dwlpx_decompose_tag __P((void *, pcitag_t, int *, int *,
		    int *));
pcireg_t	dwlpx_conf_read __P((void *, pcitag_t, int));
void		dwlpx_conf_write __P((void *, pcitag_t, int, pcireg_t));

void
dwlpx_pci_init(pc, v)
	pci_chipset_tag_t pc;
	void *v;
{
	pc->pc_conf_v = v;
	pc->pc_attach_hook = dwlpx_attach_hook;
	pc->pc_bus_maxdevs = dwlpx_bus_maxdevs;
	pc->pc_make_tag = dwlpx_make_tag;
	pc->pc_decompose_tag = dwlpx_decompose_tag;
	pc->pc_conf_read = dwlpx_conf_read;
	pc->pc_conf_write = dwlpx_conf_write;
}

void
dwlpx_attach_hook(parent, self, pba)
	struct device *parent, *self;
	struct pcibus_attach_args *pba;
{
#if	0
	struct dwlpx_config *ccp = pba->pba_pc->pc_conf_v;
	printf("dwlpx_attach_hook for %s\n", ccp->cc_sc->dwlpx_dev.dv_xname);
#endif
}

int
dwlpx_bus_maxdevs(cpv, busno)
	void *cpv;
	int busno;
{
	return DWLPX_MAXDEV;
}

pcitag_t
dwlpx_make_tag(cpv, b, d, f)
	void *cpv;
	int b, d, f;
{
	pcitag_t tag;
	int hpcdev, pci_idsel;

	pci_idsel = (1 << ((d & 0x3) + 2));
	hpcdev = d >> 2;
	tag = (b << 24) | (hpcdev << 22) | (pci_idsel << 16) | (f << 13);
	return (tag);
}

void
dwlpx_decompose_tag(cpv, tag, bp, dp, fp)
	void *cpv;
	pcitag_t tag;
	int *bp, *dp, *fp;
{

	if (bp != NULL)
		*bp = (tag >> 24) & 0xff;
	if (dp != NULL) {
		int j, i = (tag >> 18) & 0xf;
		j = -1;
		while (i != 0) {
			j++;
			i >>= 1;
		}
		j += (((tag >> 22) & 3) << 2);
		*dp = j;
	}
	if (fp != NULL)
		*fp = (tag >> 13) & 0x7;
}

pcireg_t
dwlpx_conf_read(cpv, tag, offset)
	void *cpv;
	pcitag_t tag;
	int offset;
{
	struct dwlpx_config *ccp = cpv;
	struct dwlpx_softc *sc;
	pcireg_t *dp, data = (pcireg_t) -1;
	unsigned long paddr;
	int secondary, i, s = 0;
	u_int32_t rvp;

	if (ccp == NULL) {
		panic("NULL ccp in dwlpx_conf_read\n");
	}
	sc = ccp->cc_sc;
	secondary = tag >> 24;
	if (secondary) {
#ifdef	DO_SECONDARIES
		tag &= 0x1fffff;
		tag |= (secondary << 21);

#if	0
		printf("read secondary %d reg %x (tag %x)",
		    secondary, offset, tag);
#endif

		alpha_pal_draina();
		s = splhigh();
		/*
		 * Set up HPCs for type 1 cycles.
		 */
		for (i = 0; i < sc->dwlpx_nhpc; i++) {
			rvp = REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) | 1;
			alpha_mb();
			REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) = rvp;
			alpha_mb();
		}
#else
		return (data);
#endif
	}
	paddr = (unsigned long) tag;
	paddr |= DWLPX_PCI_CONF;
	paddr |= ((unsigned long) ((offset >> 2) << 7));
	paddr |= (((unsigned long) sc->dwlpx_hosenum) << 34);
	paddr |= (((u_long) sc->dwlpx_node - 4) << 36);
	paddr |= (1LL << 39);
	paddr |= (1LL << 3);	/* 32 Bit PCI byte enables */

	dp = (pcireg_t *)KV(paddr);
	if (badaddr(dp, sizeof (*dp)) == 0) {
		data = *dp;
	}
	if (secondary) {
		alpha_pal_draina();
		for (i = 0; i < sc->dwlpx_nhpc; i++) {
			rvp = REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) & ~1;
			alpha_mb();
			REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) = rvp;
			alpha_mb();
		}
		(void) splx(s);
#ifdef	DO_SECONDARIES
#if	0
		printf("=%x\n", data);
#endif
#endif
	}
	return (data);
}

void
dwlpx_conf_write(cpv, tag, offset, data)
	void *cpv;
	pcitag_t tag;
	int offset;
	pcireg_t data;
{
	struct dwlpx_config *ccp = cpv;
	struct dwlpx_softc *sc;
	pcireg_t *dp;
	unsigned long paddr;
	int secondary, i, s = 0;
	u_int32_t rvp;

	if (ccp == NULL) {
		panic("NULL ccp in dwlpx_conf_write\n");
	}
	sc = ccp->cc_sc;
	secondary = tag >> 24;
	if (secondary) {
		tag &= 0x1fffff;
		tag |= (secondary << 21);
#if	0
		printf("write secondary %d reg %x (tag %x) with %x\n",
		    secondary, offset, tag, data);
#endif

		alpha_pal_draina();
		s = splhigh();
		/*
		 * Set up HPCs for type 1 cycles.
		 */
		for (i = 0; i < sc->dwlpx_nhpc; i++) {
			rvp = REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) | 1;
			alpha_mb();
			REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) = rvp;
			alpha_mb();
		}
	}
	paddr = (unsigned long) tag;
	paddr |= DWLPX_PCI_CONF;
	paddr |= ((unsigned long) ((offset >> 2) << 7));
	paddr |= (((unsigned long) sc->dwlpx_hosenum) << 34);
	paddr |= (((u_long) sc->dwlpx_node - 4) << 36);
	paddr |= (1LL << 39);
	paddr |= (1LL << 3);	/* 32 bit PCI byte enables */

	dp = (pcireg_t *)KV(paddr);
	*dp = data;
	alpha_mb();
	if (secondary) {
		alpha_pal_draina();
		for (i = 0; i < sc->dwlpx_nhpc; i++) {
			rvp = REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) & ~1;
			alpha_mb();
			REGVAL(PCIA_CTL(i) + ccp->cc_sysbase) = rvp;
			alpha_mb();
		}
		(void) splx(s);
	}
}
