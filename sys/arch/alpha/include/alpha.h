/* $NetBSD: alpha.h,v 1.9 2000/06/01 17:12:42 thorpej Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
#ifdef _KERNEL

#include <machine/bus.h>

struct pcb;
struct proc;
struct reg;
struct rpb;
struct trapframe;

extern int bootdev_debug;

void	XentArith __P((u_int64_t, u_int64_t, u_int64_t));	/* MAGIC */
void	XentIF __P((u_int64_t, u_int64_t, u_int64_t));		/* MAGIC */
void	XentInt __P((u_int64_t, u_int64_t, u_int64_t));		/* MAGIC */
void	XentMM __P((u_int64_t, u_int64_t, u_int64_t));		/* MAGIC */
void	XentRestart __P((void));				/* MAGIC */
void	XentSys __P((u_int64_t, u_int64_t, u_int64_t));		/* MAGIC */
void	XentUna __P((u_int64_t, u_int64_t, u_int64_t));		/* MAGIC */
void	alpha_init __P((u_long, u_long, u_long, u_long, u_long));
int	alpha_pa_access __P((u_long));
void	ast __P((struct trapframe *));
int	badaddr	__P((void *, size_t));
int	badaddr_read __P((void *, size_t, void *));
void	child_return __P((void *));
u_int64_t console_restart __P((struct trapframe *));
void	do_sir __P((void));
void	dumpconf __P((void));
void	exception_return __P((void));				/* MAGIC */
void	frametoreg __P((struct trapframe *, struct reg *));
long	fswintrberr __P((void));				/* MAGIC */
void	init_bootstrap_console __P((void));
void	init_prom_interface __P((struct rpb *));
void	interrupt
	__P((unsigned long, unsigned long, unsigned long, struct trapframe *));
void	machine_check
	__P((unsigned long, struct trapframe *, unsigned long, unsigned long));
u_int64_t hwrpb_checksum __P((void));
void	hwrpb_restart_setup __P((void));
void	regdump __P((struct trapframe *));
void	regtoframe __P((struct reg *, struct trapframe *));
void	savectx __P((struct pcb *));
void    switch_exit __P((struct proc *));			/* MAGIC */
void	switch_trampoline __P((void));				/* MAGIC */
void	syscall __P((u_int64_t, struct trapframe *));
void	trap __P((unsigned long, unsigned long, unsigned long, unsigned long,
	    struct trapframe *));
void	trap_init __P((void));
void	enable_nsio_ide __P((bus_space_tag_t));
char *	dot_conv __P((unsigned long));

/* Multiprocessor glue; cpu.c */
struct cpu_info;
int	cpu_iccb_send __P((long, const char *));
void	cpu_iccb_receive __P((void));
void	cpu_hatch __P((struct cpu_info *));
void	cpu_halt_secondary __P((unsigned long));
void	cpu_spinup_trampoline __P((void));			/* MAGIC */
void	cpu_pause __P((unsigned long));
void	cpu_resume __P((unsigned long));

#endif /* _KERNEL */
#endif /* _ALPHA_H_ */
