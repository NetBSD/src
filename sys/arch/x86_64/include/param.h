/*	$NetBSD: param.h,v 1.1.2.1 2002/03/16 16:00:25 jdolecek Exp $	*/

#ifdef _KERNEL
#ifdef _LOCORE
#include <machine/psl.h>
#else
#include <machine/cpu.h>
#endif
#endif

#define	_MACHINE	x86_64
#define	MACHINE		"x86_64"
#define	_MACHINE_ARCH	x86_64
#define	MACHINE_ARCH	"x86_64"
#define MID_MACHINE	MID_X86_64

/*
 * Round p (pointer or byte index) up to a correctly-aligned value
 * for all data types (int, long, ...).   The result is u_int and
 * must be cast to any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits). 
 *
 */
#define ALIGNBYTES		(sizeof(long) - 1)
#define ALIGN(p)		(((u_long)(p) + ALIGNBYTES) &~ALIGNBYTES)
#define ALIGNED_POINTER(p,t)	1

#define ALIGNBYTES32		(sizeof(int) - 1)
#define ALIGN32(p)		(((u_long)(p) + ALIGNBYTES32) &~ALIGNBYTES32)

#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	NPTEPG		(NBPG/(sizeof (pt_entry_t)))

/*
 * XXXfvdl change this (after bootstrap) to take # of bits from
 * config info into account.
 */
#define	KERNBASE	0xffff800000000000 /* start of kernel virtual space */
#define	KERNTEXTOFF	0xffff800000100000 /* start of kernel text */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define KERNTEXTOFF_HI	0xffff8000
#define KERNTEXTOFF_LO	0x00100000

#define KERNBASE_HI	0xffff8000
#define KERNBASE_LO	0x00000000

#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1 << DEV_BSHIFT)
#define	BLKDEV_IOSIZE	2048
#ifndef	MAXPHYS
#define	MAXPHYS		(64 * 1024)	/* max raw I/O transfer size */
#endif

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */
#define	UPAGES		4		/* pages of u-area */
#define	USPACE		(UPAGES * NBPG)	/* total size of u-area */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	4*NBPG		/* default message buffer size */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		256		/* size of an mbuf */

#ifndef MCLSHIFT
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
					/* 2K cluster can hold Ether frame */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#ifndef NMBCLUSTERS
#if defined(_KERNEL_OPT)
#include "opt_gateway.h"
#endif

#ifdef GATEWAY
#define	NMBCLUSTERS	4096		/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS	2048		/* map size, max cluster allocation */
#endif
#endif

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((8 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_DEFAULT	((128 * 1024 * 1024) >> PAGE_SHIFT)

/* pages ("clicks") to disk blocks */
#define	ctod(x)		((x) << (PGSHIFT - DEV_BSHIFT))
#define	dtoc(x)		((x) >> (PGSHIFT - DEV_BSHIFT))

/* bytes to pages */
#define	ctob(x)		((x) << PGSHIFT)
#define	btoc(x)		(((x) + PGOFSET) >> PGSHIFT)

/* bytes to disk blocks */
#define	dbtob(x)	((x) << DEV_BSHIFT)
#define	btodb(x)	((x) >> DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and should use the bsize
 * field from the disk label.
 * For now though just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE / DEV_BSIZE))

/*
 * XXXfvdl the PD* stuff is different from i386.
 */
/*
 * Mach derived conversion macros
 */
#define	x86_64_round_pdr(x) \
	((((unsigned long)(x)) + (NBPD_L2 - 1)) & ~(NBPD_L2 - 1))
#define	x86_64_trunc_pdr(x)	((unsigned long)(x) & ~(NBPD_L2 - 1))
#define	x86_64_btod(x)		((unsigned long)(x) >> L2_SHIFT)
#define	x86_64_dtob(x)		((unsigned long)(x) << L2_SHIFT)
#define	x86_64_round_page(x)	((((unsigned long)(x)) + PGOFSET) & ~PGOFSET)
#define	x86_64_trunc_page(x)	((unsigned long)(x) & ~PGOFSET)
#define	x86_64_btop(x)		((unsigned long)(x) >> PGSHIFT)
#define	x86_64_ptob(x)		((unsigned long)(x) << PGSHIFT)

#define btop(x)				x86_64_btop(x)
#define ptob(x)				x86_64_ptob(x)
#define x86_trunc_page(x)		x86_64_trunc_page(x)
#define round_pdr(x)			x86_64_round_pdr(x)
