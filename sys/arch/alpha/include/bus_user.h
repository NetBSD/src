/* $NetBSD: bus_user.h,v 1.1.12.1 2017/12/03 11:35:46 jdolecek Exp $ */
/*
 * XXX This file is a stopgap intended to keep NetBSD/alpha buildable
 * XXX while developers figure out whether/how to expose to userland
 * XXX the bus_space(9) and pci(9) facilities used by libalpha (which
 * XXX is used by X11).
 * XXX
 * XXX Do NOT add new definitions to this file.
 */
#ifndef _ALPHA_BUS_USER_H_
#define _ALPHA_BUS_USER_H_

#include <sys/types.h>

/*
 * Addresses (in bus space).
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Translation of an Alpha bus address; INTERNAL USE ONLY.
 */
struct alpha_bus_space_translation {
	bus_addr_t	abst_bus_start;	/* start of bus window */
	bus_addr_t	abst_bus_end;	/* end of bus window */
	__paddr_t	abst_sys_start;	/* start of sysBus window */
	__paddr_t	abst_sys_end;	/* end of sysBus window */
	int		abst_addr_shift;/* address shift */
	int		abst_size_shift;/* size shift */
	int		abst_flags;	/* flags; see below */
};

#define	ABST_BWX		0x01	/* use BWX to access the bus */
#define	ABST_DENSE		0x02	/* space is dense */

#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE     	0x04

#endif /* _ALPHA_BUS_USER_H_ */
