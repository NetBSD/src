/*	$NetBSD: bus.h,v 1.3 2001/06/10 03:16:30 briggs Exp $	*/
/*	$OpenBSD: bus.h,v 1.1 1997/10/13 10:53:42 pefo Exp $	*/

#ifndef _SANDPOINT_BUS_H_
#define _SANDPOINT_BUS_H_

#include <powerpc/bus.h>

/*
 * Values for the SandPoint bus space tag, not to be used directly by MI code.
 */
#define	SANDPOINT_BUS_SPACE_IO	0xFE000000	/* i/o space */
#define SANDPOINT_BUS_SPACE_MEM	0x80000000	/* mem space */
#define SANDPOINT_BUS_SPACE_EUMB	0xFC000000	/* EUMB space */
#define SANDPOINT_PCI_CONFIG_ADDR	0xFEC00CF8
#define SANDPOINT_PCI_CONFIG_DATA	0xFEE00CFC

/*
 * Address conversion as seen from a PCI master.
 */
#define PHYS_TO_PCI_MEM(x)	(x)
#define PCI_MEM_TO_PHYS(x)	(x)

extern const struct powerpc_bus_space sandpoint_io_bs_tag;
extern const struct powerpc_bus_space sandpoint_isa_io_bs_tag;
extern const struct powerpc_bus_space sandpoint_mem_bs_tag;

void	sandpoint_bus_space_init(void);
void	sandpoint_bus_space_mallocok(void);

#endif /* _SANDPOINT_BUS_H_ */
