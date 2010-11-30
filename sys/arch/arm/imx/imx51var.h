/*	$NetBSD: imx51var.h,v 1.2 2010/11/30 13:05:27 bsh Exp $ */

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


/* iomux utility functions */
struct iomux_conf {
	u_int pin;
#define	IOMUX_CONF_EOT	((u_int)(-1))
	u_short mux;
	u_short pad;
};

void iomux_set_function(u_int, u_int);
void iomux_set_pad(u_int, u_int);
//void iomux_set_input(u_int, u_int);
void iomux_mux_config(const struct iomux_conf *);

#endif	/* _ARM_IMX_IMX51VAR_H */
