/*	$NetBSD: machdep.h,v 1.4 2001/06/27 03:31:42 fredette Exp $	*/

/*
 * Copyright (c) 2001 Matthew Fredette
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	from: Utah Hdr: cpu.h 1.16 91/03/25
 *	from: @(#)cpu.h	7.7 (Berkeley) 6/27/91
 *	cpu.h,v 1.2 1993/05/22 07:58:17 cgd Exp
 */

/*
 * Internal definitions unique to sun2/68k cpu support.
 * These are the "private" declarations - those needed
 * only here in machine-independent code.  The "public"
 * definitions are in cpu.h (used by common code).
 */

#ifdef _KERNEL

/* Prototypes... */

struct frame;
struct fpframe;
struct pcb;
struct proc;
struct reg;
struct trapframe;
struct uio;

extern int cold;
extern int fputype;

extern label_t *nofault;

extern vm_offset_t vmmap;	/* XXX - See mem.c */

void	clock_init  __P((void));
void	cninit __P((void));

void	dumpconf __P((void));
void	dumpsys __P((void));

int 	fpu_emulate __P((struct trapframe *, struct fpframe *));

int 	getdfc __P((void));
int 	getsfc __P((void));

/* Backward compatibility... */
#define getsr	_getsr

void**	getvbr __P((void));

void	initfpu __P((void));

void	set_clk_mode __P((int, int));

void	proc_trampoline __P((void));

void	savectx __P((struct pcb *));

void	setvbr __P((void **));

void	g0_entry __P((void));
void	g4_entry __P((void));

void	swapconf __P((void));

void	switch_exit __P((struct proc *));

void	zs_init __P((void));

struct sun2_kcore_hdr;

/* Kernel virtual address space available: */
extern vm_offset_t virtual_avail, virtual_end;
/* Physical address space available: */
extern vm_offset_t avail_start, avail_end;

/* pmap.c */
void	pmap_bootstrap __P((vm_offset_t nextva));
void	pmap_kcore_hdr __P((struct sun2_kcore_hdr *));
void	pmap_get_pagemap __P((int *pt, int off));

#endif	/* _KERNEL */
