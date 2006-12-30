/*	$NetBSD: ataconf.h,v 1.1.12.2 2006/12/30 20:47:54 yamt Exp $	*/

/*
 * Written in 2006 by ITOH Yasufumi.
 * Public domain.
 */

#ifndef _DEV_ATA_ATACONF_H_
#define _DEV_ATA_ATACONF_H_

#if !defined(_KERNEL_OPT) || defined(LKM)

/* compile-in everything for LKM or LKM-enabled kernel */
# define NATA_DMA	1
# define NATA_UDMA	1
# define NATA_PIOBM	1

#else

# include "ata_dma.h"
# if NATA_UDMA > 1 && NATA_DMA == 0
   #error ata_udma requires ata_dma
# endif

#endif

#endif /* _DEV_ATA_ATACONF_H_ */
