/*	$NetBSD: param.h,v 1.18.2.2 2017/12/03 11:35:47 jdolecek Exp $	*/

#ifdef __x86_64__

#ifndef XEN
/* Must be defined before cpu.h */
#define	MAXCPUS		256
#endif

#ifdef _KERNEL
#include <machine/cpu.h>
#endif

#define	_MACHINE	amd64
#define	MACHINE		"amd64"
#define	_MACHINE_ARCH	x86_64
#define	MACHINE_ARCH	"x86_64"
#define MID_MACHINE	MID_X86_64

#define ALIGNED_POINTER(p,t)	1

#define ALIGNBYTES32		(sizeof(int) - 1)
#define ALIGN32(p)		(((u_long)(p) + ALIGNBYTES32) &~ALIGNBYTES32)

#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	NBPG		(1 << PGSHIFT)	/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	NPTEPG		(NBPG/(sizeof (pt_entry_t)))

#define	MAXIOMEM	0xffffffffffff

/*
 * Maximum physical memory supported by the implementation.
 */
#define MAXPHYSMEM	0x100000000000ULL /* 16TB */

/*
 * XXXfvdl change this (after bootstrap) to take # of bits from
 * config info into account.
 */
#define	KERNBASE	0xffffffff80000000 /* start of kernel virtual space */
#define	KERNTEXTOFF	0xffffffff80200000 /* start of kernel text */
#define	BTOPKERNBASE	((u_long)KERNBASE >> PGSHIFT)

#define KERNTEXTOFF_HI	0xffffffff
#define KERNTEXTOFF_LO	0x80200000

#define KERNBASE_HI	0xffffffff
#define KERNBASE_LO	0x80000000

#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define	DEV_BSIZE	(1 << DEV_BSHIFT)
#define	BLKDEV_IOSIZE	2048
#ifndef	MAXPHYS
#define	MAXPHYS		(64 * 1024)	/* default I/O transfer size max */
#endif

#define	MACHINE_MAXPHYS	(1024 * 1024)	/* absolute I/O transfer size max */

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */
#ifdef DIAGNOSTIC
#define	UPAGES		4		/* pages of u-area (1 for redzone) */
#else
#define	UPAGES		3		/* pages of u-area */
#endif
#define	USPACE		(UPAGES * NBPG)	/* total size of u-area */
#define	INTRSTACKSIZE	4096

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	(8*NBPG)	/* default message buffer size */
#endif

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#define	MSIZE		512		/* size of an mbuf */

#ifndef MCLSHIFT
#define	MCLSHIFT	11		/* convert bytes to m_buf clusters */
					/* 2K cluster can hold Ether frame */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#ifndef NFS_RSIZE
#define NFS_RSIZE       32768
#endif
#ifndef NFS_WSIZE
#define NFS_WSIZE       32768
#endif

/*
 * Minimum size of the kernel kmem_arena in PAGE_SIZE-sized
 * logical pages.
 * No enforced maximum on amd64.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((8 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_UNLIMITED 1

/*
 * XXXfvdl the PD* stuff is different from i386.
 */
/*
 * Mach derived conversion macros
 */
#define	x86_round_pdr(x) \
	((((unsigned long)(x)) + (NBPD_L2 - 1)) & ~(NBPD_L2 - 1))
#define	x86_trunc_pdr(x)	((unsigned long)(x) & ~(NBPD_L2 - 1))
#define	x86_btod(x)		((unsigned long)(x) >> L2_SHIFT)
#define	x86_dtob(x)		((unsigned long)(x) << L2_SHIFT)
#define	x86_round_page(x)	((((unsigned long)(x)) + PGOFSET) & ~PGOFSET)
#define	x86_trunc_page(x)	((unsigned long)(x) & ~PGOFSET)
#define	x86_btop(x)		((unsigned long)(x) >> PGSHIFT)
#define	x86_ptob(x)		((unsigned long)(x) << PGSHIFT)

#define btop(x)				x86_btop(x)
#define ptob(x)				x86_ptob(x)

#define mstohz(ms) ((ms + 0UL) * hz / 1000)

#else	/*	__x86_64__	*/

#include <i386/param.h>

#endif	/*	__x86_64__	*/
