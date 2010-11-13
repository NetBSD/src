/*	$NetBSD: imx31var.h,v 1.3 2010/11/13 05:00:31 bsh Exp $	*/

#ifndef _ARM_IMX_IMX31VAR_H
#define _ARM_IMX_IMX31VAR_H

extern struct bus_space imx_bs_tag;

struct aips_attach_args {
	const char	*aipsa_name;
	bus_space_tag_t	aipsa_memt;
	bus_addr_t	aipsa_addr;
	bus_size_t	aipsa_size;
	int		aipsa_intr;
};

struct ahb_attach_args {
	const char	*ahba_name;
	bus_space_tag_t	ahba_memt;
	bus_dma_tag_t	ahba_dmat;
	bus_addr_t	ahba_addr;
	bus_size_t	ahba_size;
	int		ahba_intr;
	int		ahba_irqbase;
};

#endif	/* _ARM_IMX_IMX31VAR_H */
