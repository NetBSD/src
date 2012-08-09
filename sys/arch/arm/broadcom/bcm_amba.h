/*	$NetBSD: bcm_amba.h,v 1.1.2.2 2012/08/09 06:36:50 jdc Exp $	*/

#ifndef _ARM_BROADCOM_BCM_AMBA_H_
#define _ARM_BROADCOM_BCM_AMBA_H_

/* Broadcom AMBA AXI Peripheral Bus */

struct amba_attach_args {
	const char	*aaa_name;	/* name */
	bus_space_tag_t	aaa_iot;	/* Bus tag */
	bus_addr_t	aaa_addr;	/* Address */
	bus_size_t	aaa_size;	/* Size of peripheral address space */
	int		aaa_intr;	/* IRQ number */
	bus_dma_tag_t	aaa_dmat;	/* DMA channel */
};

struct ambadev_locators {
	const char	*ad_name;
	bus_addr_t	ad_addr;
	bus_size_t	ad_size;
	int		ad_instance;
	int		ad_intr;
};

extern struct bus_space bcm2835_bs_tag;
extern struct arm32_bus_dma_tag bcm2835_bus_dma_tag;
// extern const struct ambadev_locators *md_ambadev_locs;

#endif /* _ARM_BROADCOM_BCM_AMBA_H_ */
