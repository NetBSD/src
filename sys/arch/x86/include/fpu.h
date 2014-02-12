/*	$NetBSD: fpu.h,v 1.2 2014/02/12 23:24:09 dsl Exp $	*/

#ifndef	_X86_FPU_H_
#define	_X86_FPU_H_

#include <x86/cpu_extended_state.h>

#ifdef _KERNEL

struct trapframe;
struct cpu_info;

void fpuinit(struct cpu_info *);
void fpusave_lwp(struct lwp *, bool);
void fpusave_cpu(bool);

void fputrap(struct trapframe *);
void fpudna(struct trapframe *);

void process_xmm_to_s87(const struct fxsave *, struct save87 *);
void process_s87_to_xmm(const struct save87 *, struct fxsave *);

#endif

#endif /* _X86_FPU_H_ */
