/*	$NetBSD: imx51var.h,v 1.1 2010/11/13 07:11:03 bsh Exp $ */

#ifndef _ARM_IMX_IMX51VAR_H
#define	_ARM_IMX_IMX51VAR_H

extern struct bus_space imx_bs_tag;
extern struct arm32_bus_dma_tag imx_bus_dma_tag;

void gpio_set_direction(uint32_t, uint32_t);
void gpio_data_write(uint32_t, uint32_t);
bool gpio_data_read(uint32_t);

struct axi_attach_args {
	const char	*aa_name;
	bus_space_tag_t	aa_iot;
	bus_dma_tag_t	aa_dmat;
	bus_addr_t	aa_addr;
	bus_size_t	aa_size;
	int		aa_irq;
	int		aa_irqbase;
};

#endif	/* _ARM_IMX_IMX51VAR_H */
