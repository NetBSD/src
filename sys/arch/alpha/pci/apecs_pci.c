/* $NetBSD: apecs_pci.c,v 1.28 2021/06/25 03:45:59 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: apecs_pci.c,v 1.28 2021/06/25 03:45:59 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <alpha/pci/apecsreg.h>
#include <alpha/pci/apecsvar.h>

static pcireg_t	apecs_conf_read(void *, pcitag_t, int);
static void	apecs_conf_write(void *, pcitag_t, int, pcireg_t);

void
apecs_pci_init(pci_chipset_tag_t pc, void *v)
{

	pc->pc_conf_v = v;
	pc->pc_conf_read = apecs_conf_read;
	pc->pc_conf_write = apecs_conf_write;
}

static pcireg_t
apecs_conf_read(void *cpv, pcitag_t tag, int offset)
{
	struct apecs_config *acp = cpv;
	pcireg_t *datap, data;
	int s, secondary, ba;
	int32_t old_haxr2;					/* XXX */

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return (pcireg_t) -1;

	s = 0;					/* XXX gcc -Wuninitialized */
	old_haxr2 = 0;				/* XXX gcc -Wuninitialized */

	/* secondary if bus # != 0 */
	pci_decompose_tag(&acp->ac_pc, tag, &secondary, 0, 0);
	if (secondary) {
		s = splhigh();
		old_haxr2 = REGVAL(EPIC_HAXR2);
		alpha_mb();
		REGVAL(EPIC_HAXR2) = old_haxr2 | 0x1;
		alpha_mb();
	}

	datap = (pcireg_t *)ALPHA_PHYS_TO_K0SEG(APECS_PCI_CONF |
	    tag << 5UL |					/* XXX */
	    (offset & ~0x03) << 5 |				/* XXX */
	    0 << 5 |						/* XXX */
	    0x3 << 3);						/* XXX */
	data = (pcireg_t)-1;
	if (!(ba = badaddr(datap, sizeof *datap)))
		data = *datap;

	if (secondary) {
		alpha_mb();
		REGVAL(EPIC_HAXR2) = old_haxr2;
		alpha_mb();
		splx(s);
	}

#if 0
	printf("apecs_conf_read: tag 0x%lx, reg 0x%lx -> %x @ %p%s\n", tag, reg,
	    data, datap, ba ? " (badaddr)" : "");
#endif

	return data;
}

static void
apecs_conf_write(void *cpv, pcitag_t tag, int offset, pcireg_t data)
{
	struct apecs_config *acp = cpv;
	pcireg_t *datap;
	int s, secondary;
	int32_t old_haxr2;					/* XXX */

	if ((unsigned int)offset >= PCI_CONF_SIZE)
		return;

	s = 0;					/* XXX gcc -Wuninitialized */
	old_haxr2 = 0;				/* XXX gcc -Wuninitialized */

	/* secondary if bus # != 0 */
	pci_decompose_tag(&acp->ac_pc, tag, &secondary, 0, 0);
	if (secondary) {
		s = splhigh();
		old_haxr2 = REGVAL(EPIC_HAXR2);
		alpha_mb();
		REGVAL(EPIC_HAXR2) = old_haxr2 | 0x1;
		alpha_mb();
	}

	datap = (pcireg_t *)ALPHA_PHYS_TO_K0SEG(APECS_PCI_CONF |
	    tag << 5UL |					/* XXX */
	    (offset & ~0x03) << 5 |				/* XXX */
	    0 << 5 |						/* XXX */
	    0x3 << 3);						/* XXX */

	alpha_mb();
	*datap = data;
	alpha_mb();
	alpha_mb();

	if (secondary) {
		alpha_mb();
		REGVAL(EPIC_HAXR2) = old_haxr2;	
		alpha_mb();
		splx(s);
	}

#if 0
	printf("apecs_conf_write: tag 0x%lx, reg 0x%lx -> 0x%x @ %p\n", tag,
	    reg, data, datap);
#endif
}
