/*	$NetBSD: iyonixvar.h,v 1.2.4.1 2006/09/09 02:40:48 rpaulo Exp $	*/

#ifndef _IYONIXVAR_H_
#define _IYONIXVAR_H_

#include <dev/pci/pcivar.h>

void iyonix_pci_init(pci_chipset_tag_t, void *);
extern char iyonix_macaddr[];

#endif /* _IYONIXVAR_H_ */
