/*	$NetBSD: sysarch.h,v 1.2.4.1 1998/11/19 06:44:50 nisimura Exp $ */

#ifndef _MIPS_SYSARCH_H_
#define _MIPS_SYSARCH_H_

/*
 * Architecture specific syscalls (mips)
 */
#define MIPS_CACHEFLUSH	0
#define MIPS_CACHECTL	1


struct mips_cacheflush_args {
	vaddr_t va;
	int nbytes;
	int whichcache;
};

struct mips_cachectl_args {
	vaddr_t va;
	int nbytes;
	int ctl;
};

#ifndef _KERNEL
int sysarch __P((int, void *));
#endif

#endif /* !_MIPS_SYSARCH_H_ */
