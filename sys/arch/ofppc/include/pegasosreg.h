/* $NetBSD: pegasosreg.h,v 1.3 2010/12/17 19:18:49 phx Exp $ */

#ifndef _MACHINE_PEGASOSREG_H_
#define _MACHINE_PEGASOSREG_H_

/* Marvell Discovery System Controller */
#define PEGASOS2_GT_REGBASE		0xf1000000

/* GPIO to control AGP configuration space access */
#define PEGASOS2_AGP_CONF_ENABLE	(1 << 23)

#define PEGASOS2_SRAM_BASE		0xf2000000
#define PEGASOS2_SRAM_SIZE		0x40000

#endif /*_MACHINE_PEGASOSREG_H_*/
