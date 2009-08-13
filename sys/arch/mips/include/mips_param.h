/*	$NetBSD: mips_param.h,v 1.25 2009/08/13 05:15:09 matt Exp $	*/

#ifdef _KERNEL
#include <machine/cpu.h>
#endif

/*
 * On mips, UPAGES is fixed by sys/arch/mips/mips/locore code
 * to be the number of per-process-wired kernel-stack pages/PTES.
 */

#define	SSIZE		1		/* initial stack size/NBPG */
#define	SINCR		1		/* increment of stack/NBPG */

#ifdef ENABLE_MIPS_16KB_PAGE
#define	UPAGES		1		/* pages of u-area */
#define	USPACE		(UPAGES*NBPG)	/* size of u-area in bytes */
#elif defined(ENABLE_MIPS_4KB_PAGE) || 1
#define	UPAGES		2		/* pages of u-area */
#define	USPACE		(UPAGES*NBPG)	/* size of u-area in bytes */
#else
#error ENABLE_MIPS_xKB_PAGE not defined
#endif

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

#ifdef ENABLE_MIPS_16KB_PAGE
#define	NBPG		16384		/* bytes/page */
#define	PGSHIFT		14		/* LOG2(NBPG) */
#else
#define	NBPG		4096		/* bytes/page */
#define	PGSHIFT		12		/* LOG2(NBPG) */
#endif
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	NPTEPG		(NBPG/4)

#define NBSEG		(NBPG*NPTEPG)	/* bytes/segment */
#define	SEGOFSET	(NBSEG-1)	/* byte offset into segment */
#define	SEGSHIFT	(2*PGSHIFT-2)	/* LOG2(NBSEG) */

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

#ifdef __MIPSEL__
#define	MID_MACHINE	135	/* MID_PMAX (little-endian) */
#endif
#ifdef __MIPSEB__
#define	MID_MACHINE	152	/* MID_MIPS (big-endian) */
#endif

