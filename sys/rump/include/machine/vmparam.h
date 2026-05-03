/*	$NetBSD: vmparam.h,v 1.5 2026/05/03 17:17:22 thorpej Exp $	*/

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
 */
#define	MIN_PAGE_SHIFT		11
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
