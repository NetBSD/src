/* $NetBSD: aubusvar.h,v 1.3.44.1 2006/06/15 16:40:43 gdamore Exp $ */

#ifndef _MIPS_ALCHEMY_DEV_AUBUSVAR_H_
#define	_MIPS_ALCHEMY_DEV_AUBUSVAR_H_

#include <machine/bus.h>

/*
 * Machine-dependent structures for autoconfiguration
 */
struct aubus_attach_args {
	const char	*aa_name;	/* device name */
	bus_space_tag_t	aa_st;		/* the space tag to use */
	bus_addr_t	aa_addrs[3];	/* system bus address(es) */
	int		aa_irq[2];	/* IRQ index(s) */
	bus_dma_tag_t	aa_dt;		/* the dma tag to use */
};
#define	aa_addr		aa_addrs[0]

/* order of attach addresses for aumac register addresses */
#define	AA_MAC_BASE	0
#define	AA_MAC_ENABLE	1
#define	AA_MAC_DMA_BASE	2

extern bus_space_tag_t	aubus_st;		/* XXX: for aubus.c */

int com_aubus_cnattach(bus_addr_t, int);

#endif	/* !_MIPS_ALCHEMY_DEV_AUBUSVAR_H_ */
