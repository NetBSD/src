/*	$NetBSD: mips_param.h,v 1.23.78.1 2009/08/20 07:47:52 matt Exp $	*/

#ifdef _KERNEL
#include <machine/cpu.h>
#endif

/*
 * On mips, UPAGES is fixed by sys/arch/mips/mips/locore code
 * to be the number of per-process-wired kernel-stack pages/PTES.
 */

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */

#define	UPAGES		2		/* pages of u-area */
#define	USPACE		(UPAGES*NBPG)	/* size of u-area in bytes */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

/*
 * Round p (pointer or byte index) up to a correctly-aligned value for all
 * data types (int, long, ...).   The result is u_int and must be cast to
 * any desired pointer type.
 *
 * ALIGNED_POINTER is a boolean macro that checks whether an address
 * is valid to fetch data elements of type t from on this architecture.
 * This does not reflect the optimal alignment, just the possibility
 * (within reasonable limits).
 *
 */
#define	ALIGNBYTES	7
#define	ALIGN(p)	(((uintptr_t)(p) + ALIGNBYTES) & ~ALIGNBYTES)
#define ALIGNED_POINTER(p,t)	((((uintptr_t)(p)) & (sizeof(t)-1)) == 0)

#define	NBPG		4096		/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		12		/* LOG2(NBPG) */
#define	NPTEPG		(NBPG/4)

#define NBSEG		0x400000	/* bytes/segment */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */
#define	SEGSHIFT	22		/* LOG2(NBSEG) */

/*
 * Minimum and maximum sizes of the kernel malloc arena in PAGE_SIZE-sized
 * logical pages.
 */
#define	NKMEMPAGES_MIN_DEFAULT	((8 * 1024 * 1024) >> PAGE_SHIFT)
#define	NKMEMPAGES_MAX_DEFAULT	((128 * 1024 * 1024) >> PAGE_SHIFT)

/*
 * Mach derived conversion macros
 */
#define mips_round_page(x)	((((uintptr_t)(x)) + NBPG - 1) & ~(NBPG-1))
#define mips_trunc_page(x)	((uintptr_t)(x) & ~(NBPG-1))
#define mips_btop(x)		((paddr_t)(x) >> PGSHIFT)
#define mips_ptob(x)		((paddr_t)(x) << PGSHIFT)

/*
 * Constants related to network buffer management.
 * MCLBYTES must be no larger than NBPG (the software page size), and,
 * on machines that exchange pages of input or output buffers with mbuf
 * clusters (MAPPED_MBUFS), MCLBYTES must also be an integral multiple
 * of the hardware page size.
 */
#ifndef MSIZE
#ifdef _LP64
#define	MSIZE		512		/* size of an mbuf */
#else
#define	MSIZE		256		/* size of an mbuf */
#endif

#ifndef MCLSHIFT
# define MCLSHIFT	11		/* convert bytes to m_buf clusters */
#endif	/* MCLSHIFT */

#define	MCLBYTES	(1 << MCLSHIFT)	/* size of a m_buf cluster */

#ifndef NMBCLUSTERS
#if defined(_KERNEL_OPT)
#include "opt_gateway.h"
#endif

#ifdef GATEWAY
#define	NMBCLUSTERS	2048		/* map size, max cluster allocation */
#else
#define	NMBCLUSTERS	1024		/* map size, max cluster allocation */
#endif
#endif
#endif
