/*	$NetBSD: machdep.h,v 1.20.8.3 2001/03/12 13:29:42 bouyer Exp $	*/

/*
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
 * Internal definitions unique to sun3/68k cpu support.
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
struct mmu_rootptr;

extern label_t *nofault;

extern vm_offset_t vmmap;	/* XXX - See mem.c */

/* Cache flush functions. */
void	DCIA __P((void));
void	DCIU __P((void));
void	ICIA __P((void));

void	child_return __P((void *));

void	clock_init  __P((void));
void	cninit __P((void));

void	dumpconf __P((void));
void	dumpsys __P((void));

void	enable_fpu __P((int));
void	enable_init __P((void));
void	enable_video __P((int));

int 	fpu_emulate __P((struct trapframe *, struct fpframe *));

/* Backward compatibility... */
#define getsr	_getsr

void**	getvbr __P((void));
int	getcrp __P((struct mmu_rootptr *));

void	initfpu __P((void));
void	intreg_init __P((void));

void	isr_init __P((void));
void	isr_config __P((void));

void	m68881_save __P((struct fpframe *));
void	m68881_restore __P((struct fpframe *));

void	netintr __P((void));

caddr_t	obio_find_mapping __P((int pa, int size));
void	obio_init __P((void));

void	proc_trampoline __P((void));

void	savectx __P((struct pcb *));

void	setvbr __P((void **));

void	sunmon_abort __P((void));
void	sunmon_halt __P((void));
void	sunmon_init __P((void));
void	sunmon_reboot __P((char *));

void	swapconf __P((void));

void	switch_exit __P((struct proc *));

void	zs_init __P((void));

#ifdef	_SUN3_

struct sun3_kcore_hdr;

extern int cache_size;
void	cache_enable __P((void));

/* Kernel virtual address space available: */
extern vm_offset_t virtual_avail, virtual_end;
/* Physical address space available: */
extern vm_offset_t avail_start, avail_end;
/* The "hole" (used to skip the Sun3/50 video RAM) */
extern vm_offset_t hole_start, hole_size;

/* cache.c */
void	cache_enable __P((void));
void	cache_flush_page(vm_offset_t pgva);
void	cache_flush_segment(vm_offset_t sgva);
void	cache_flush_context(void);

/* pmap.c */
void	pmap_bootstrap __P((vm_offset_t nextva));
void	pmap_kcore_hdr __P((struct sun3_kcore_hdr *));
void	pmap_get_pagemap __P((int *pt, int off));

#endif	/* SUN3 */

#ifdef	_SUN3X_

struct mmu_rootptr;
struct sun3x_kcore_hdr;

extern int has_iocache;

/* This is set by locore.s with the monitor's root ptr. */
extern struct mmu_rootptr mon_crp;

/* Lowest "managed" kernel virtual address. */
extern vm_offset_t virtual_avail;

/* Cache flush ops, Sun3X specific. */
void	DCIAS __P((vm_offset_t));
void	DCIS __P((void));
void	ICPA __P((void));
void	PCIA __P((void));
/* ATC flush operations. */
void	TBIA __P((void));
void	TBIS __P((vm_offset_t));
void	TBIAS __P((void));
void	TBIAU __P((void));

void	loadcrp __P((struct mmu_rootptr *));

void	pmap_bootstrap __P((vm_offset_t nextva));
void	pmap_kcore_hdr __P((struct sun3x_kcore_hdr *));
int 	pmap_pa_exists __P((vm_offset_t pa));

#endif	/* SUN3X */

#endif	/* _KERNEL */
