/*	$NetBSD: mips_param.h,v 1.29.4.1 2012/04/17 00:06:39 yamt Exp $	*/

#ifdef _KERNEL
#include <machine/cpu.h>
#endif

/*
 * No reason this can't be common
 */
#if defined(__MIPSEB__)
# if defined(__mips_n32) || defined(__mips_n64)
#  define	_MACHINE_ARCH	mips64eb
#  define	MACHINE_ARCH	"mips64eb"
#  define	_MACHINE32_ARCH	mipseb
#  define	MACHINE32_ARCH	"mipseb"
# else
#  define	_MACHINE_ARCH	mipseb
#  define	MACHINE_ARCH	"mipseb"
# endif
#elif defined(__MIPSEL__)
# if defined(__mips_n32) || defined(__mips_n64)
#  define	_MACHINE_ARCH	mips64el
#  define	MACHINE_ARCH	"mips64el"
#  define	_MACHINE32_ARCH	mipsel
#  define	MACHINE32_ARCH	"mipsel"
# else
#  define	_MACHINE_ARCH	mipsel
#  define	MACHINE_ARCH	"mipsel"
#endif
#else
#error neither __MIPSEL__ nor __MIPSEB__ are defined.
#endif

/*
 * Userland code should be using uname/sysctl to get MACHINE so simply
 * export a generic MACHINE of "mips"
 */
#ifndef _KERNEL
#undef MACHINE
#define MACHINE "mips"
#endif

#define ALIGNBYTES32		(sizeof(double) - 1)
#define ALIGN32(p)		(((uintptr_t)(p) + ALIGNBYTES32) &~ALIGNBYTES32)

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
#define	USPACE_ALIGN	USPACE		/* make sure it starts on a even VA */

#ifndef MSGBUFSIZE
#define MSGBUFSIZE	NBPG		/* default message buffer size */
#endif

#ifndef COHERENCY_UNIT
#define COHERENCY_UNIT	32	/* MIPS cachelines are usually 32 bytes */
#endif

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
#define	SEGSHIFT	(PGSHIFT+(PGSHIFT-2))	/* LOG2(NBSEG) */

#ifdef _LP64
#define	NSEGPG		(NBPG/8)
#define NBXSEG		(NSEGPG*NBSEG)	/* bytes/xsegment */
#define	XSEGOFSET	(NBSEG-1)	/* byte offset into segment */
#define	XSEGSHIFT	(SEGSHIFT+(PGSHIFT-3))	/* LOG2(NBXSEG) */
#endif

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
#define	MID_MACHINE	MID_PMAX	/* MID_PMAX (little-endian) */
#endif
#ifdef __MIPSEB__
#define	MID_MACHINE	MID_MIPS	/* MID_MIPS (big-endian) */
#endif

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

#endif
