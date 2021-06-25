/* $NetBSD: lca_pci.c,v 1.24 2021/06/25 03:52:41 thorpej Exp $ */

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: lca_pci.c,v 1.24 2021/06/25 03:52:41 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/lcareg.h>
#include <alpha/pci/lcavar.h>

static int	lca_bus_maxdevs(void *, int);
static pcireg_t	lca_conf_read(void *, pcitag_t, int);
static void	lca_conf_write(void *, pcitag_t, int, pcireg_t);

void
lca_pci_init(pci_chipset_tag_t pc, void *v)
{

	pc->pc_conf_v = v;
	pc->pc_bus_maxdevs = lca_bus_maxdevs;
	pc->pc_conf_read = lca_conf_read;
	pc->pc_conf_write = lca_conf_write;
}

static int
lca_bus_maxdevs(void *cpv, int busno)
{
	/*
	 * We have to drive the IDSEL directly on bus 0, so we are
	 * limited to 16 devices there.
	 */
	return busno == 0 ? 16 : 32;
}

static paddr_t
lca_make_type0addr(int d, int f)
{
	KASSERT(d < 16);
	return PCI_CONF_TYPE0_IDSEL(d) | __SHIFTIN(f, PCI_CONF_TYPE0_FUNCTION);
}

static pcireg_t
lca_conf_read(void *cpv, pcitag_t tag, int offset)
{
	struct lca_config *lcp = cpv;
	pcireg_t *datap, data;
	paddr_t confaddr;
	int s, secondary, d, f, ba;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	s = 0;					/* XXX gcc -Wuninitialized */

	/* secondary if bus # != 0 */
	pci_decompose_tag(&lcp->lc_pc, tag, &secondary, &d, &f);
	if (secondary) {
		s = splhigh();
		alpha_mb();
		REGVAL(LCA_IOC_CONF) = 0x01;
		alpha_mb();
		confaddr = tag;
	} else {
		confaddr = lca_make_type0addr(d, f);
	}

	datap = (pcireg_t *)ALPHA_PHYS_TO_K0SEG(LCA_PCI_CONF |
	    confaddr << 5UL |					/* XXX */
	    (offset & ~0x03) << 5 |				/* XXX */
	    0 << 5 |						/* XXX */
	    0x3 << 3);						/* XXX */
	data = (pcireg_t)-1;
	if (!(ba = badaddr(datap, sizeof *datap)))
		data = *datap;

	if (secondary) {
		alpha_mb();
		REGVAL(LCA_IOC_CONF) = 0x00;
		alpha_mb();
		splx(s);
	}

#if 0
	printf("lca_conf_read: tag 0x%lx, reg 0x%lx -> %x @ %p%s\n", tag, reg,
	    data, datap, ba ? " (badaddr)" : "");
#endif

	return data;
}

static void
lca_conf_write(void *cpv, pcitag_t tag, int offset, pcireg_t data)
{
	struct lca_config *lcp = cpv;
	pcireg_t *datap;
	paddr_t confaddr;
	int s, secondary, d, f;

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return;

	s = 0;					/* XXX gcc -Wuninitialized */

	/* secondary if bus # != 0 */
	pci_decompose_tag(&lcp->lc_pc, tag, &secondary, &d, &f);
	if (secondary) {
		s = splhigh();
		alpha_mb();
		REGVAL(LCA_IOC_CONF) = 0x01;
		alpha_mb();
		confaddr = tag;
	} else {
		confaddr = lca_make_type0addr(d, f);
	}

	datap = (pcireg_t *)ALPHA_PHYS_TO_K0SEG(LCA_PCI_CONF |
	    confaddr << 5UL |					/* XXX */
	    (offset & ~0x03) << 5 |				/* XXX */
	    0 << 5 |						/* XXX */
	    0x3 << 3);						/* XXX */
	*datap = data;

	if (secondary) {
		alpha_mb();
		REGVAL(LCA_IOC_CONF) = 0x00;	
		alpha_mb();
		splx(s);
	}

#if 0
	printf("lca_conf_write: tag 0x%lx, reg 0x%lx -> 0x%x @ %p\n", tag,
	    reg, data, datap);
#endif
}
