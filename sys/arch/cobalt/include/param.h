/*	$NetBSD: param.h,v 1.3 2000/03/31 14:51:53 soren Exp $	*/

#include <mips/mips_param.h>

#define	_MACHINE_ARCH	mipsel
#define	MACHINE_ARCH	"mipsel"
#define	_MACHINE	cobalt
#define	MACHINE		"cobalt"
#define	MID_MACHINE	MID_MIPS

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* Maximum raw I/O transfer size */

#define	CLSIZE		1
#define	CLSIZELOG2	0

#define	MSIZE		128		/* Size of an mbuf */

#ifndef MCLSHIFT
#define	MCLSHIFT	11		/* Convert bytes to m_buf clusters. */
#endif

#define	MCLBYTES	(1 << MCLSHIFT)	/* Size of a m_buf cluster */
#define	MCLOFSET	(MCLBYTES - 1)

#if defined(_KERNEL) && !defined(_LKM)
#include "opt_gateway.h"
#endif /* _KERNEL && ! _LKM */

#ifndef NMBCLUSTERS
#ifdef GATEWAY
#define	NMBCLUSTERS	2048		/* Map size, max cluster allocation */
#else
#define	NMBCLUSTERS	1024		/* Map size, max cluster allocation */
#endif
#endif

#ifdef _KERNEL
#ifndef _LOCORE

__inline extern void	delay(unsigned long);
#define DELAY(n)	delay(n)

#include <machine/intr.h>

#endif	/* _LOCORE */
#endif	/* _KERNEL */
