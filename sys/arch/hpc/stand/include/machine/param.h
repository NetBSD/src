/*	$NetBSD: param.h,v 1.1 2001/02/09 18:35:30 uch Exp $	*/

/* Windows CE architecture */

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */
