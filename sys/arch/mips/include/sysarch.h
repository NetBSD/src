/*	$NetBSD: sysarch.h,v 1.1 1997/06/09 11:46:17 jonathan Exp $ */

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
int sysarch __P((int, char *));
#endif

#endif /* !_MIPS_SYSARCH_H_ */
