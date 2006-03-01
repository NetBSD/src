/*	$NetBSD: iyonixvar.h,v 1.2.2.1 2006/03/01 09:27:58 yamt Exp $	*/

#ifndef _IYONIXVAR_H_
#define _IYONIXVAR_H_

#include <dev/pci/pcivar.h>

void iyonix_pci_init(pci_chipset_tag_t, void *);
extern char iyonix_macaddr[];

#endif /* _IYONIXVAR_H_ */
