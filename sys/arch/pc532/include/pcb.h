/*	$NetBSD: pcb.h,v 1.12 2003/08/07 16:29:00 agc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)pcb.h	5.10 (Berkeley) 5/12/91
 */

#ifndef _MACHINE_PCB_H_
#define _MACHINE_PCB_H_

/*
 * PC 532 process control block
 *
 *  Phil Nelson, 12/8/92
 *
 */

struct pcb {
	/* Put in a pointer to the trap/interrupt frame. */
	struct reg *pcb_onstack;

	/* Floating point stuff */
	long   pcb_fsr;		/* fpu status reg */
	double pcb_freg[8];	/* F0 - F7 */

	/* Saved during a "swtch" */

	long pcb_ksp;		/* Kernel stack -- sp0. */
	long pcb_kfp;		/* Kernel fp. */
	long pcb_ptb;		/* ptb0, ptb1 */

/*
 * Software pcb (extension)
 */
	caddr_t	pcb_onfault;	/* copyin/out fault recovery */
	struct pmap *pcb_pmap;	/* back pointer to our pmap */
};

/*    
 * The pcb is augmented with machine-dependent additional data for 
 * core dumps. For the pc532, there is nothing to add.
 */     
struct md_coredump {
	long	md_pad[8];
};    


#ifdef _KERNEL
struct pcb *curpcb;		/* our current running pcb */
#endif

#endif
