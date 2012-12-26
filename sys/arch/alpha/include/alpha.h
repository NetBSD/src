/* $NetBSD: alpha.h,v 1.31 2012/12/26 19:13:19 matt Exp $ */

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: cpu.h 1.16 91/03/25$
 *
 *	@(#)cpu.h	8.4 (Berkeley) 1/5/94
 */

#ifndef _ALPHA_H_
#define _ALPHA_H_

typedef union alpha_s_float {
	uint32_t i;
	uint32_t frac: 23,
		  exp:   8,
		  sign:  1;
} s_float;

typedef union alpha_t_float {
	uint64_t i;
	uint64_t frac: 52,
		  exp:  11,
		  sign:  1;
} t_float;

#ifdef _KERNEL

#include <sys/bus.h>

#include <sys/pcu.h>

struct pcb;
struct lwp;
struct reg;
struct rpb;
struct trapframe;

extern u_long cpu_implver;		/* from IMPLVER instruction */
extern u_long cpu_amask;		/* from AMASK instruction */
extern int bootdev_debug;
extern int alpha_fp_sync_complete;
extern int alpha_unaligned_print, alpha_unaligned_fix, alpha_unaligned_sigbus;

void	XentArith(uint64_t, uint64_t, uint64_t);		/* MAGIC */
void	XentIF(uint64_t, uint64_t, uint64_t);		/* MAGIC */
void	XentInt(uint64_t, uint64_t, uint64_t);		/* MAGIC */
void	XentMM(uint64_t, uint64_t, uint64_t);		/* MAGIC */
void	XentRestart(void);					/* MAGIC */
void	XentSys(uint64_t, uint64_t, uint64_t);		/* MAGIC */
void	XentUna(uint64_t, uint64_t, uint64_t);		/* MAGIC */
void	alpha_init(u_long, u_long, u_long, u_long, u_long);
void	ast(struct trapframe *);
int	badaddr(void *, size_t);
int	badaddr_read(void *, size_t, void *);
uint64_t console_restart(struct trapframe *);
void	do_sir(void);
void	dumpconf(void);
void	exception_return(void);					/* MAGIC */
void	frametoreg(const struct trapframe *, struct reg *);
long	fswintrberr(void);					/* MAGIC */
void	init_bootstrap_console(void);
void	init_prom_interface(struct rpb *);
void	interrupt(unsigned long, unsigned long, unsigned long,
	    struct trapframe *);
void	machine_check(unsigned long, struct trapframe *, unsigned long,
	    unsigned long);
uint64_t hwrpb_checksum(void);
void	hwrpb_restart_setup(void);
void	regdump(struct trapframe *);
void	regtoframe(const struct reg *, struct trapframe *);
void	savectx(struct pcb *);
void	trap(unsigned long, unsigned long, unsigned long, unsigned long,
	    struct trapframe *);
void	trap_init(void);
void	enable_nsio_ide(bus_space_tag_t);
char *	dot_conv(unsigned long);

extern const pcu_ops_t fpu_ops;

void    fpu_state_load(struct lwp *, u_int);
void    fpu_state_save(struct lwp *, u_int);
void    fpu_state_release(struct lwp *, u_int);

static inline void
fpu_load(void)
{
	pcu_load(&fpu_ops);
}

static inline void
fpu_save(void)
{
	pcu_save(&fpu_ops);
}

static inline void
fpu_discard(void)
{
	pcu_discard(&fpu_ops);
}

void	alpha_patch(bool);

/* Multiprocessor glue; cpu.c */

struct cpu_info;
int	cpu_iccb_send(long, const char *);
void	cpu_iccb_receive(void);
void	cpu_hatch(struct cpu_info *);
void	cpu_halt(void) __attribute__((__noreturn__));
void	cpu_spinup_trampoline(void);				/* MAGIC */
void	cpu_pause(unsigned long);
void	cpu_resume(unsigned long);
#if defined(DDB)
void	cpu_debug_dump(void);
#endif

/* IEEE and VAX FP completion */

void alpha_sts(int, s_float *);					/* MAGIC */
void alpha_stt(int, t_float *);					/* MAGIC */
void alpha_lds(int, s_float *);					/* MAGIC */
void alpha_ldt(int, t_float *);					/* MAGIC */

uint64_t alpha_read_fpcr(void);					/* MAGIC */
void alpha_write_fpcr(uint64_t);				/* MAGIC */

uint64_t alpha_read_fp_c(struct lwp *);
void alpha_write_fp_c(struct lwp *, uint64_t);

int alpha_fp_complete(u_long, u_long, struct lwp *, uint64_t *);

/* Security sensitive rate limiting printf */

void rlprintf(struct timeval *t, const char *fmt, ...);

#endif /* _KERNEL */
#endif /* _ALPHA_H_ */
