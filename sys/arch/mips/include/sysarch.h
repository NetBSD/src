/*	$NetBSD: sysarch.h,v 1.8.96.2 2011/04/29 08:26:22 matt Exp $ */

#ifndef _MIPS_SYSARCH_H_
#define _MIPS_SYSARCH_H_

/*
 * Architecture specific syscalls (mips)
 */
#define MIPS_CACHEFLUSH	0
#define MIPS_CACHECTL	1

struct mips_cacheflush_args {
	vaddr_t va;
	size_t nbytes;
	int whichcache;
};

struct mips_cachectl_args {
	vaddr_t va;
	size_t nbytes;
	int ctl;
};

#ifndef _KERNEL
#include <sys/cdefs.h>

__BEGIN_DECLS
int sysarch(int, void *);
__END_DECLS
#endif /* !_KERNEL */
#endif /* !_MIPS_SYSARCH_H_ */
