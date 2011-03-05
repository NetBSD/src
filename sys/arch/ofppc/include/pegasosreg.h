/* $NetBSD: pegasosreg.h,v 1.1.52.2 2011/03/05 20:51:28 rmind Exp $ */

#ifndef _MACHINE_PEGASOSREG_H_
#define _MACHINE_PEGASOSREG_H_

/* Marvell Discovery System Controller */
#define PEGASOS2_GT_REGBASE		0xf1000000

/* GPIO to control AGP configuration space access */
#define PEGASOS2_AGP_CONF_ENABLE	(1 << 23)

#define PEGASOS2_SRAM_BASE		0xf2000000
#define PEGASOS2_SRAM_SIZE		0x40000

#endif /*_MACHINE_PEGASOSREG_H_*/
