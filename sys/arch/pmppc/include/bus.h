/*	$NetBSD: bus.h,v 1.3 2003/07/28 07:58:51 scw Exp $	*/

#ifndef _PMPPC_BUS_H_
#define _PMPPC_BUS_H_

/*
 * Address conversion as seen from a PCI master.
 */
#define PHYS_TO_BUS_MEM(t,x)	(x)
#define BUS_MEM_TO_PHYS(t,x)	(x)

extern struct powerpc_bus_space pmppc_mem_tag;
extern struct powerpc_bus_space pmppc_pci_io_tag;

void	pmppc_bus_space_init(void);
void	pmppc_bus_space_mallocok(void);

#include <powerpc/bus.h>

#endif /* _PMPPC_BUS_H_ */
