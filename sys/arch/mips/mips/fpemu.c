/*	$NetBSD: fpemu.c,v 1.2 1999/11/22 02:11:09 shin Exp $ */

/*
 * Copyright (c) 1999 Shuichiro URATA.  All rights reserved.
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

#ifdef SOFTFLOAT
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/signalvar.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/buf.h>
#include <sys/reboot.h>
#ifdef KTRACE
#include <sys/ktrace.h>
#endif

#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <machine/cpu.h>
#include <mips/trap.h>
#include <machine/psl.h>
#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
extern struct proc *fpcurproc;

int	MachEmulateLWC1 __P((unsigned inst, mips_reg_t *frame));
int	MachEmulateLDC1 __P((unsigned inst, mips_reg_t *frame));
int	MachEmulateSWC1 __P((unsigned inst, mips_reg_t *frame));
int	MachEmulateSDC1 __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lb __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lbu __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lh __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lhu __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lw __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lwl __P((unsigned inst, mips_reg_t *frame));
int	bcemul_lwr __P((unsigned inst, mips_reg_t *frame));
int	bcemul_sb __P((unsigned inst, mips_reg_t *frame));
int	bcemul_sh __P((unsigned inst, mips_reg_t *frame));
int	bcemul_sw __P((unsigned inst, mips_reg_t *frame));
int	bcemul_swl __P((unsigned inst, mips_reg_t *frame));
int	bcemul_swr __P((unsigned inst, mips_reg_t *frame));

int
MachEmulateLWC1(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, x;
	short		offset;

	offset = inst & 0xFFFF;

	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000003)
		return SIGSEGV;

	x = fuword((void *)vaddr);
	fpcurproc->p_addr->u_pcb.pcb_fpregs.r_regs[(inst>>16)&0x1F] = x;

	return 0;
}

int
MachEmulateLDC1(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, x;
	short		offset;

	offset = inst & 0xFFFF;

	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000007)
		return SIGSEGV;

	x = fuword((void *)vaddr);
	fpcurproc->p_addr->u_pcb.pcb_fpregs.r_regs[(inst>>16)&0x1F] = x;
	x = fuword((void *)(vaddr+4));
	fpcurproc->p_addr->u_pcb.pcb_fpregs.r_regs[((inst>>16)&0x1F)+1] = x;

	return 0;
}

int
MachEmulateSWC1(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, x;
	short		offset;

	offset = inst & 0xFFFF;

	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000003)
		return SIGSEGV;

	x = fpcurproc->p_addr->u_pcb.pcb_fpregs.r_regs[(inst>>16)&0x1F];
	suword((void *)vaddr, x);

	return 0;
}

int
MachEmulateSDC1(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, x;
	short		offset;

	offset = inst & 0xFFFF;

	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000007)
		return SIGSEGV;

	x = fpcurproc->p_addr->u_pcb.pcb_fpregs.r_regs[(inst>>16)&0x1F];
	suword((void *)vaddr, x);
	x = fpcurproc->p_addr->u_pcb.pcb_fpregs.r_regs[((inst>>16)&0x1F)+1];
	suword((void *)(vaddr+4), x);

	return 0;
}

int
bcemul_lb(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, x;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	x = fubyte((void *)vaddr);
	if (x & 0x80)
		x |= 0xFFFFFF00;
	frame[(inst>>16)&0x1F] = x;

	return 0;
}

int
bcemul_lbu(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	frame[(inst>>16)&0x1F] = fubyte((void *)vaddr);

	return 0;
}

int
bcemul_lh(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, x;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000001)
		return SIGSEGV;

	x = fusword((void *)vaddr);
	if (x & 0x8000)
		x |= 0xFFFF0000;
	frame[(inst>>16)&0x1F] = x;

	return 0;
}

int
bcemul_lhu(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000001)
		return SIGSEGV;

	frame[(inst>>16)&0x1F] = fusword((void *)vaddr);

	return 0;
}

int
bcemul_lw(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000003)
		return SIGSEGV;

	frame[(inst>>16)&0x1F] = fuword((void *)vaddr);

	return 0;
}

int
bcemul_lwl(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, a, x, shift;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	a = fuword((void *)(vaddr & ~0x3));
	x = frame[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 0x00000003)) * 8;
	a <<= shift;
	x &= ~(0xFFFFFFFFUL << shift);
	x |= a;

	frame[(inst>>16)&0x1F] = x;

	return 0;
}

int
bcemul_lwr(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, a, x, shift;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	a = fuword((void *)(vaddr & ~0x3));
	x = frame[(inst>>16)&0x1F];

	shift = (vaddr & 0x00000003) * 8;
	a >>= shift;
	x &= ~(0xFFFFFFFFUL >> shift);
	x |= a;

	frame[(inst>>16)&0x1F] = x;

	return 0;
}

int
bcemul_sb(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	subyte((void *)vaddr, frame[(inst>>16)&0x1F]);

	return 0;
}

int
bcemul_sh(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000001)
		return SIGSEGV;

	susword((void *)vaddr, frame[(inst>>16)&0x1F]);

	return 0;
}

int
bcemul_sw(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000003)
		return SIGSEGV;

	suword((void *)vaddr, frame[(inst>>16)&0x1F]);

	return 0;
}

int
bcemul_swl(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, a, x, shift;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	a = fuword((void *)(vaddr & ~0x3));
	x = frame[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 0x00000003)) * 8;
	x >>= shift;
	a &= ~(0xFFFFFFFFUL >> shift);
	a |= x;

	suword((void *)vaddr, a);

	return 0;
}

int
bcemul_swr(inst, frame)
	unsigned inst;
	mips_reg_t *frame;
{
	unsigned int	vaddr, a, x, shift;
	short		offset;

	offset = inst & 0xFFFF;
	vaddr = frame[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000)
		return SIGSEGV;

	a = fuword((void *)(vaddr & ~0x3));
	x = frame[(inst>>16)&0x1F];

	shift = (vaddr & 0x00000003) * 8;
	x <<= shift;
	a &= ~(0xFFFFFFFFUL << shift);
	a |= x;

	suword((void *)vaddr, a);

	return 0;
}
#endif /* SOFTFLOAT */
