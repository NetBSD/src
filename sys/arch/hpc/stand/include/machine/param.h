/*	$NetBSD: param.h,v 1.1.26.4 2004/09/21 13:15:59 skrll Exp $	*/

/* Windows CE architecture */

#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	BLKDEV_IOSIZE	2048
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */

/* bytes to disk blocks */
#define	btodb(x)	((x) >> DEV_BSHIFT)
