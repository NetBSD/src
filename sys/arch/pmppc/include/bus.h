/*	$NetBSD: bus.h,v 1.2.2.2 2004/09/18 14:39:04 skrll Exp $	*/

#ifndef _PMPPC_BUS_H_
#define _PMPPC_BUS_H_

extern struct powerpc_bus_space pmppc_mem_tag;
extern struct powerpc_bus_space pmppc_pci_io_tag;

void	pmppc_bus_space_mallocok(void);

#include <powerpc/bus.h>

#endif /* _PMPPC_BUS_H_ */
