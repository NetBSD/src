/*	$NetBSD: bus.h,v 1.5 2005/12/11 12:18:41 christos Exp $	*/

#ifndef _PMPPC_BUS_H_
#define _PMPPC_BUS_H_

extern struct powerpc_bus_space pmppc_mem_tag;
extern struct powerpc_bus_space pmppc_pci_io_tag;

void	pmppc_bus_space_mallocok(void);

#include <powerpc/bus.h>

#endif /* _PMPPC_BUS_H_ */
