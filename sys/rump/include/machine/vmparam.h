/*	$NetBSD: vmparam.h,v 1.6 2026/05/04 19:31:46 thorpej Exp $	*/

/*
 * Dummy <machine/vmparam.h> for rump.
 *
 * Written by Jason R. Thorpe, May 1, 2026.
 * Public domain.
 */

#ifndef _SYS_RUMP_VMPARAM_H_
#define	_SYS_RUMP_VMPARAM_H_

/*
 * We have VM page sizes ranging from 2K to 16K.
 *
 * Note that mbufs use MIN_PAGE_SHIFT to calculate the number of
 * external pages that can be associated with an M_EXT mbuf.  On
 * LP64 platforms we can easily get into trouble and overflow the
 * required space (within MSIZE) because pointers are 8 bytes.
 * If a platform has a 256-byte MSIZE, then MIN_PAGE_SIZE must be
 * at least 8K.  And we know this is true because the compile-time
 * asserts pass when building the kernel on those platforms.  If
 * the platform's MSIZE is 512 (as it is on amd64, for example),
 * then a 4K page size is OK.
 */
#ifdef _LP64
#define	MIN_PAGE_SHIFT		(12 + (MSIZE == 256))
#else
#define	MIN_PAGE_SHIFT		11
#endif
#define	MAX_PAGE_SHIFT		14
#define	MIN_PAGE_SIZE		(1 << MIN_PAGE_SHIFT)
#define	MAX_PAGE_SIZE		(1 << MAX_PAGE_SHIFT)

/*
 * 16MB ought to be enough for anybody, really.  Especially
 * considering that user-space doesn't really exist in RUMP.
 */
#define	VM_MIN_ADDRESS		((vaddr_t)0)
#define	VM_MAX_ADDRESS		((vaddr_t)0x01000000)
#define	VM_MAXUSER_ADDRESS	VM_MAX_ADDRESS

/*
 * Stack starts at the top of the user's address space.
 */
#define	USRSTACK		VM_MAX_ADDRESS

/*
 * Resource limits that are incredibly generous for a
 * non-existent user-space.
 */
#define	MAXTSIZ			(5*1024*1024)
#define	DFLDSIZ			(4*1024*1024)
#define	MAXDSIZ			(6*1024*1024)
#define	DFLSSIZ			(512*1024)
#define	MAXSSIZ			(4*1024*1024)

/*
 * The RUMP virtual machine has one physical memory segment and
 * one page free list.
 */
#define	VM_PHYSSEG_MAX		1
#define	VM_PHYSSEG_STRAT	VM_PSTRAT_BSEARCH

#define	VM_NFREELIST		1
#define	VM_FREELIST_DEFAULT	0

#endif /* _SYS_RUMP_VMPARAM_H_ */
