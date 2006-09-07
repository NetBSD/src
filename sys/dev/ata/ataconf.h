/*	$NetBSD: ataconf.h,v 1.1 2006/09/07 12:34:42 itohy Exp $	*/

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
