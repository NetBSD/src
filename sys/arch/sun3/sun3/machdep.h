/*	$NetBSD: machdep.h,v 1.37.14.1 2014/08/20 00:03:26 tls Exp $	*/

/*
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
 *	from: Utah Hdr: cpu.h 1.16 91/03/25
 *	from: @(#)cpu.h	7.7 (Berkeley) 6/27/91
 *	cpu.h,v 1.2 1993/05/22 07:58:17 cgd Exp
 */

/*
 * Copyright (c) 1994 Gordon W. Ross
 * Copyright (c) 1993 Adam Glass
 * Copyright (c) 1988 University of Utah.
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
 * Internal definitions unique to sun3/68k CPU support.
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

extern vaddr_t vmmap;	/* XXX - See mem.c */

void	clock_init (void);
void	cninit(void);

void	dumpconf(void);
void	dumpsys(void);

void	enable_fpu(int);
void	enable_init(void);
void	enable_video(int);

int	fpu_emulate(struct trapframe *, struct fpframe *, ksiginfo_t *);

/* Backward compatibility... */
#define getsr	_getsr

void**	getvbr(void);
int	getcrp(struct mmu_rootptr *);

void	initfpu(void);
void	intreg_init(void);

void	isr_init(void);
void	isr_config(void);

void	netintr(void);

void	obio_init(void);

void	setvbr(void **);

void	sunmon_abort(void);
void	sunmon_halt(void);
void	sunmon_init(void);
void	sunmon_reboot(const char *);

void	swapconf(void);

void	zs_init(void);

#ifdef	_SUN3_

struct sun3_kcore_hdr;

extern int cache_size;
void	cache_enable(void);

/* Kernel virtual address space available: */
extern vaddr_t virtual_avail, virtual_end;
/* Physical address space available: */
extern paddr_t avail_start, avail_end;

/* cache.c */
void	cache_enable(void);
void	cache_flush_page(vaddr_t);
void	cache_flush_segment(vaddr_t);
void	cache_flush_context(void);

/* pmap.c */
void	pmap_bootstrap(vaddr_t);
void	pmap_kcore_hdr(struct sun3_kcore_hdr *);
void	pmap_get_pagemap(int *, int);

#endif	/* SUN3 */

#ifdef	_SUN3X_

struct mmu_rootptr;
struct sun3x_kcore_hdr;

extern int has_iocache;

/* This is set by locore.s with the monitor's root ptr. */
extern struct mmu_rootptr mon_crp;

/* Lowest "managed" kernel virtual address. */
extern vaddr_t virtual_avail;

void	loadcrp(struct mmu_rootptr *);

void	pmap_bootstrap(vaddr_t);
void	pmap_kcore_hdr(struct sun3x_kcore_hdr *);
int	pmap_pa_exists(paddr_t);

#endif	/* SUN3X */

#endif	/* _KERNEL */
