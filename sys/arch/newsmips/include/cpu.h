/*	$NetBSD: cpu.h,v 1.16.192.1 2018/10/20 06:58:29 pgoyette Exp $	*/

#ifndef _NEWSMIPS_CPU_H_
#define _NEWSMIPS_CPU_H_

#include <mips/cpu.h>

#if defined(_KERNEL) || defined(_STANDALONE)
#ifndef _LOCORE
extern int systype;

#define NEWS3400	1
#define NEWS5000	2
#define NEWS4000	3

/* System type dependent initializations. */
#ifdef news3400
void news3400_init(void);
int news3400_badaddr(void *, u_int);
#endif
#ifdef news5000
void news5000_init(void);
#endif
#ifdef news4000
void news4000_init(void);
#endif
#endif
#endif

#endif /* _NEWSMIPS_CPU_H_ */
