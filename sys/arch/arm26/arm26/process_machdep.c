/* $NetBSD: process_machdep.c,v 1.3 2001/02/10 19:09:48 bjh21 Exp $ */
/*-
 * Copyright (c) 2000 Ben Harris
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* This file is part of NetBSD/arm26 -- a port of NetBSD to ARM2/3 machines. */
/*
 * process_machdep.c - machine-dependent ptrace requests
 */

#include <sys/param.h>

__RCSID("$NetBSD: process_machdep.c,v 1.3 2001/02/10 19:09:48 bjh21 Exp $");

#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#include <arm/armreg.h>
#include <machine/frame.h>
#include <machine/pcb.h>
#include <machine/reg.h>

int
process_set_pc(struct proc *p, caddr_t addr)
{
	struct trapframe *tf = p->p_addr->u_pcb.pcb_tf;

	if (tf == NULL)
		return EINVAL;
	/* Only set the PC, not the PSR */
	if (((register_t)addr & R15_PC) != (register_t)addr)
		return EINVAL;
	tf->tf_r15 = (tf->tf_r15 & ~R15_PC) | (register_t)addr;
	return 0;
}

int
process_read_regs(struct proc *p, struct reg *regs)
{
	struct trapframe *tf = p->p_addr->u_pcb.pcb_tf;

	if (tf == NULL)
		return EINVAL;
	regs->r[0]  = tf->tf_r0;
	regs->r[1]  = tf->tf_r1;
	regs->r[2]  = tf->tf_r2;
	regs->r[3]  = tf->tf_r3;
	regs->r[4]  = tf->tf_r4;
	regs->r[5]  = tf->tf_r5;
	regs->r[6]  = tf->tf_r6;
	regs->r[7]  = tf->tf_r7;
	regs->r[8]  = tf->tf_r8;
	regs->r[9]  = tf->tf_r9;
	regs->r[10] = tf->tf_r10;
	regs->r[11] = tf->tf_r11;
	regs->r[12] = tf->tf_r12;
	regs->r_sp  = tf->tf_r13; /* XXX */
	regs->r_lr  = tf->tf_r14;
	regs->r_pc  = tf->tf_r15; /* XXX name? */
	return 0;
}

int
process_write_regs(struct proc *p, struct reg *regs)
{
	struct trapframe *tf = p->p_addr->u_pcb.pcb_tf;

	if (tf == NULL)
		return EINVAL;
	if ((regs->r_pc & (R15_MODE | R15_IRQ_DISABLE | R15_FIQ_DISABLE)) != 0)
		return EPERM;
	tf->tf_r0  = regs->r[0];
	tf->tf_r1  = regs->r[1];
	tf->tf_r2  = regs->r[2];
	tf->tf_r3  = regs->r[3];
	tf->tf_r4  = regs->r[4];
	tf->tf_r5  = regs->r[5];
	tf->tf_r6  = regs->r[6];
	tf->tf_r7  = regs->r[7];
	tf->tf_r8  = regs->r[8];
	tf->tf_r9  = regs->r[9];
	tf->tf_r10 = regs->r[10];
	tf->tf_r11 = regs->r[11];
	tf->tf_r12 = regs->r[12];
	tf->tf_r13 = regs->r_sp;
	tf->tf_r14 = regs->r_lr;
	tf->tf_r15 = regs->r_pc;
	return 0;
}

