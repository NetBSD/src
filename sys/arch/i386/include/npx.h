/*	$NetBSD: npx.h,v 1.34 2014/02/07 19:36:15 dsl Exp $	*/

#ifndef	_I386_NPX_H_
#define	_I386_NPX_H_

#include <x86/cpu_extended_state.h>

#ifdef _KERNEL

int	npx586bug1(int, int);
void 	fpuinit(struct cpu_info *);
void	process_xmm_to_s87(const struct fxsave *, struct save87 *);
void	process_s87_to_xmm(const struct save87 *, struct fxsave *);
struct lwp;
int	npxtrap(struct lwp *);

#endif

#endif /* !_I386_NPX_H_ */
