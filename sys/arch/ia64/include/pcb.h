/*	$NetBSD: pcb.h,v 1.2.68.1 2017/08/28 17:51:42 skrll Exp $	*/

/*-
 * Copyright (c) 2003,2004 Marcel Moolenaar
 * Copyright (c) 2000 Doug Rabson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$FreeBSD: src/sys/ia64/include/pcb.h,v 1.16 2004/08/16 19:05:08 marcel Exp $
 */

#ifndef _IA64_PCB_H_
#define _IA64_PCB_H_

#include <machine/_regset.h>
#include <sys/lock.h>

/*
 * PCB: process control block
 */
struct pmap;
struct pcb {
	struct _special		pcb_special;
	struct _callee_saved	pcb_preserved;
	struct _callee_saved_fp	pcb_preserved_fp;
	struct _high_fp		pcb_high_fp;
	struct pcpu		*pcb_fpcpu;
	kmutex_t		pcb_fpcpu_slock;


	/* IA32 specific registers. */
	uint64_t		pcb_ia32_cflg;
	uint64_t		pcb_ia32_eflag;
	uint64_t		pcb_ia32_fcr;
	uint64_t		pcb_ia32_fdr;
	uint64_t		pcb_ia32_fir;
	uint64_t		pcb_ia32_fsr;

	uint64_t		pcb_onfault;	/* for copy faults */
  //	struct pmap 		*pcb_current_pmap;
};

#ifdef _KERNEL

#define	savectx(p)	swapctx(p, NULL)

struct trapframe;

void makectx(struct trapframe *, struct pcb *);
/* XXX not sure about the attributes, for now use equivalent to freebsd */
void restorectx(struct pcb *) __attribute__ ((__noreturn__)); /* same as __dead2? */
int swapctx(struct pcb *old, struct pcb *new) __returns_twice;

void ia32_restorectx(struct pcb *);
void ia32_savectx(struct pcb *);

#endif

#endif /* _IA64_PCB_H_ */
