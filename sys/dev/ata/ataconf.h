/*	$NetBSD: ataconf.h,v 1.1.68.1 2008/12/13 01:14:13 haad Exp $	*/

/*
 * Written in 2006 by ITOH Yasufumi.
 * Public domain.
 */

#ifndef _DEV_ATA_ATACONF_H_
#define _DEV_ATA_ATACONF_H_

#if !defined(_KERNEL_OPT) || defined(MODULAR)

/* compile-in everything for module or module-enabled kernel */
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
