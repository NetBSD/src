/*	$NetBSD: sysarch.h,v 1.8.96.1 2010/05/15 20:27:48 matt Exp $ */

#ifndef _MIPS_SYSARCH_H_
#define _MIPS_SYSARCH_H_

/*
 * Architecture specific syscalls (mips)
 */
#define MIPS_CACHEFLUSH	0
#define MIPS_CACHECTL	1
#define MIPS_TINFOSET	2

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
