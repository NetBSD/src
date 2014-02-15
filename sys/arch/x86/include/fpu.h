/*	$NetBSD: fpu.h,v 1.3 2014/02/15 10:11:15 dsl Exp $	*/

#ifndef	_X86_FPU_H_
#define	_X86_FPU_H_

#include <x86/cpu_extended_state.h>

#ifdef _KERNEL

struct cpu_info;
struct lwp;
struct trapframe;

void fpuinit(struct cpu_info *);
void fpusave_lwp(struct lwp *, bool);
void fpusave_cpu(bool);

void fputrap(struct trapframe *);
void fpudna(struct trapframe *);

void process_xmm_to_s87(const struct fxsave *, struct save87 *);
void process_s87_to_xmm(const struct save87 *, struct fxsave *);

/* Set all to defaults (eg during exec) */
void fpu_save_area_clear(struct lwp *, unsigned int);
/* Reset control words only - for signal handlers */
void fpu_save_area_reset(struct lwp *);

#endif

#endif /* _X86_FPU_H_ */
