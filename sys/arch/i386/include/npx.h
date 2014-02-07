/*	$NetBSD: npx.h,v 1.35 2014/02/07 22:40:22 dsl Exp $	*/

#ifndef	_I386_NPX_H_
#define	_I386_NPX_H_

#include <x86/cpu_extended_state.h>

#ifdef _KERNEL

int	npx586bug1(int, int);
void 	fpuinit(struct cpu_info *);
struct lwp;
int	npxtrap(struct lwp *);

#endif

#endif /* !_I386_NPX_H_ */
