/*	$NetBSD: fpemu.c,v 1.7.4.4 2002/06/24 22:05:47 nathanw Exp $ */

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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/user.h>

#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <machine/cpu.h>
#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/vmparam.h>			/* for VM_MAX_ADDRESS */
#include <mips/trap.h>

static __inline void	send_sigsegv(u_int32_t, u_int32_t, struct frame *,
			    u_int32_t);
static __inline void	update_pc(struct frame *, u_int32_t);

void	MachEmulateLWC1(u_int32_t inst, struct frame *, u_int32_t);
void	MachEmulateLDC1(u_int32_t inst, struct frame *, u_int32_t);
void	MachEmulateSWC1(u_int32_t inst, struct frame *, u_int32_t);
void	MachEmulateSDC1(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lb(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lbu(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lh(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lhu(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lw(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lwl(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_lwr(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_sb(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_sh(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_sw(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_swl(u_int32_t inst, struct frame *, u_int32_t);
void	bcemul_swr(u_int32_t inst, struct frame *f, u_int32_t);

vaddr_t MachEmulateBranch(struct frame *, vaddr_t, unsigned, int);

static __inline void
send_sigsegv(u_int32_t vaddr, u_int32_t exccode, struct frame *frame,
    u_int32_t cause)
{

	cause = (cause & 0xFFFFFF00) | (exccode << MIPS_CR_EXC_CODE_SHIFT);

	frame->f_regs[CAUSE] = cause;
	frame->f_regs[BADVADDR] = vaddr;
	trapsignal(curlwp, SIGSEGV, vaddr);
}

static __inline void
update_pc(struct frame *frame, u_int32_t cause)
{

	if (cause & MIPS_CR_BR_DELAY)
		frame->f_regs[PC] = MachEmulateBranch(frame, frame->f_regs[PC],
		    PCB_FSR(curpcb), 0);
	else
		frame->f_regs[PC] += 4;
}

#define LWSWC1_MAXLOOP	12

void
MachEmulateLWC1(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	void		*t;
	mips_reg_t	pc;
	int		i;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

	if (copyin((void *)vaddr, t, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	pc = frame->f_regs[PC];
	update_pc(frame, cause);

	if (cause & MIPS_CR_BR_DELAY)
		return;

	for (i = 1; i < LWSWC1_MAXLOOP; i++) {
		if (mips_btop(frame->f_regs[PC]) != mips_btop(pc))
			return;

		vaddr = frame->f_regs[PC];	/* XXX truncates to 32 bits */
		inst = fuiword((u_int32_t *)vaddr);
		if (((InstFmt)inst).FRType.op != OP_LWC1)
			return;

		offset = inst & 0xFFFF;
		vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

		/* segment and alignment check */
		if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
			send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
			return;
		}

		t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

		if (copyin((void *)vaddr, t, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
			return;
		}

		pc = frame->f_regs[PC];
		update_pc(frame, cause);
	}
}

void
MachEmulateLDC1(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000007) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1E]);

	if (copyin((void *)vaddr, t, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
MachEmulateSWC1(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	void		*t;
	mips_reg_t	pc;
	int		i;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

	if (copyout(t, (void *)vaddr, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	pc = frame->f_regs[PC];
	update_pc(frame, cause);

	if (cause & MIPS_CR_BR_DELAY)
		return;

	for (i = 1; i < LWSWC1_MAXLOOP; i++) {
		if (mips_btop(frame->f_regs[PC]) != mips_btop(pc))
			return;

		vaddr = frame->f_regs[PC];	/* XXX truncates to 32 bits */
		inst = fuiword((u_int32_t *)vaddr);
		if (((InstFmt)inst).FRType.op != OP_SWC1)
			return;

		offset = inst & 0xFFFF;
		vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

		/* segment and alignment check */
		if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
			send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
			return;
		}

		t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

		if (copyout(t, (void *)vaddr, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
			return;
		}

		pc = frame->f_regs[PC];
		update_pc(frame, cause);
	}
}

void
MachEmulateSDC1(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000007) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1E]);

	if (copyout(t, (void *)vaddr, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_lb(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	int8_t		x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 1) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = (int32_t)x;

	update_pc(frame, cause);
}

void
bcemul_lbu(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	u_int8_t	x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 1) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = (u_int32_t)x;

	update_pc(frame, cause);
}

void
bcemul_lh(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	int16_t		x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000001) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 2) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = (int32_t)x;

	update_pc(frame, cause);
}

void
bcemul_lhu(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;
	u_int16_t	x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000001) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 2) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = (u_int32_t)x;

	update_pc(frame, cause);
}

void
bcemul_lw(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &(frame->f_regs[(inst>>16)&0x1F]), 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_lwl(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr, a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 0x00000003)) * 8;
	a <<= shift;
	x &= ~(0xFFFFFFFFUL << shift);
	x |= a;

	frame->f_regs[(inst>>16)&0x1F] = x;

	update_pc(frame, cause);
}

void
bcemul_lwr(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr, a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (vaddr & 0x00000003) * 8;
	a >>= shift;
	x &= ~(0xFFFFFFFFUL >> shift);
	x |= a;

	frame->f_regs[(inst>>16)&0x1F] = x;

	update_pc(frame, cause);
}

void
bcemul_sb(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (subyte((void *)vaddr, frame->f_regs[(inst>>16)&0x1F]) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_sh(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr & 0x80000001) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (susword((void *)vaddr, frame->f_regs[(inst>>16)&0x1F]) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_sw(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (suword((void *)vaddr, frame->f_regs[(inst>>16)&0x1F]) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_swl(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr, a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 0x00000003)) * 8;
	x >>= shift;
	a &= ~(0xFFFFFFFFUL >> shift);
	a |= x;

	if (suword((void *)vaddr, a) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_swr(u_int32_t inst, struct frame *frame, u_int32_t cause)
{
	u_int32_t	vaddr, a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (vaddr & 0x00000003) * 8;
	x <<= shift;
	a &= ~(0xFFFFFFFFUL << shift);
	a |= x;

	if (suword((void *)vaddr, a) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}
