/*	$NetBSD: iyonixvar.h,v 1.3 2006/02/22 00:23:52 gavan Exp $	*/

#ifndef _IYONIXVAR_H_
#define _IYONIXVAR_H_

#include <dev/pci/pcivar.h>

void iyonix_pci_init(pci_chipset_tag_t, void *);
extern char iyonix_macaddr[];

#endif /* _IYONIXVAR_H_ */
