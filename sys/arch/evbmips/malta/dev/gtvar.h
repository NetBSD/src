/*	$NetBSD: gtvar.h,v 1.1.10.2 2002/06/23 17:36:00 jdolecek Exp $	*/

#ifndef _MALTA_GTVAR_H_
#define	_MALTA_GTVAR_H_

#include <dev/pci/pcivar.h>

struct gt_config {
	int foo;
};

#ifdef _KERNEL
void	gt_pci_init(pci_chipset_tag_t, struct gt_config *);
#endif
#endif /* !_MALTA_GTVAR_H_ */
