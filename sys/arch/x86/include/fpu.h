/*	$NetBSD: fpu.h,v 1.1 2014/02/11 20:17:16 dsl Exp $	*/

#ifndef	_AMD64_FPU_H_
#define	_AMD64_FPU_H_

#include <x86/cpu_extended_state.h>

#ifdef _KERNEL
/*
 * XXX
 */
struct trapframe;
struct cpu_info;

void fpuinit(struct cpu_info *);
void fpudrop(void);
void fpusave(struct lwp *);
void fpudiscard(struct lwp *);
void fputrap(struct trapframe *);
void fpusave_lwp(struct lwp *, bool);
void fpusave_cpu(bool);

#endif

#endif /* _AMD64_FPU_H_ */
