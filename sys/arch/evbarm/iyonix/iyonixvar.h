/*	$NetBSD: iyonixvar.h,v 1.1 2019/02/14 21:47:52 macallan Exp $	*/

#ifndef _IYONIXVAR_H_
#define _IYONIXVAR_H_

#include <dev/pci/pcivar.h>

void iyonix_pci_init(pci_chipset_tag_t, void *);
extern char iyonix_macaddr[];

#endif /* _IYONIXVAR_H_ */
