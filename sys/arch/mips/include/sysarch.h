/*	$NetBSD: sysarch.h,v 1.2 1998/02/25 21:27:47 perry Exp $ */

#ifndef _MIPS_SYSARCH_H_
#define _MIPS_SYSARCH_H_

/*
 * Architecture specific syscalls (mips)
 */
#define MIPS_CACHEFLUSH	0
#define MIPS_CACHECTL	1


struct mips_cacheflush_args {
	vm_offset_t va;
	int nbytes;
	int whichcache;
};

struct mips_cachectl_args {
	vm_offset_t va;
	int nbytes;
	int ctl;
};

#ifndef _KERNEL
int sysarch __P((int, void *));
#endif

#endif /* !_MIPS_SYSARCH_H_ */
