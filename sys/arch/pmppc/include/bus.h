/*	$NetBSD: bus.h,v 1.1.4.2 2002/06/23 17:39:19 jdolecek Exp $	*/

#ifndef _PMPPC_BUS_H_
#define _PMPPC_BUS_H_

#include <powerpc/bus.h>

/*
 * Address conversion as seen from a PCI master.
 */
#define PHYS_TO_PCI_MEM(x)	(x)
#define PCI_MEM_TO_PHYS(x)	(x)

extern const struct powerpc_bus_space pmppc_mem_tag;
extern const struct powerpc_bus_space pmppc_pci_io_tag;

void	pmppc_bus_space_init(void);
void	pmppc_bus_space_mallocok(void);

#endif /* _PMPPC_BUS_H_ */
