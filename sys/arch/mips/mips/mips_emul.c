/*	$NetBSD: mips_emul.c,v 1.14.78.5 2009/11/14 21:52:08 matt Exp $ */

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mips_emul.c,v 1.14.78.5 2009/11/14 21:52:08 matt Exp $");

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

void MachEmulateFP(uint32_t, struct frame *, uint32_t);

static inline void	send_sigsegv(intptr_t, uint32_t, struct frame *,
			    uint32_t);
static inline void	update_pc(struct frame *, uint32_t);

vaddr_t MachEmulateBranch(struct frame *, vaddr_t, unsigned, int);
void	MachEmulateInst(uint32_t, uint32_t, vaddr_t, struct frame *);

void	MachEmulateLWC0(uint32_t, struct frame *, uint32_t);
void	MachEmulateSWC0(uint32_t, struct frame *, uint32_t);
void	MachEmulateSpecial(uint32_t, struct frame *, uint32_t);
void	MachEmulateLWC1(uint32_t, struct frame *, uint32_t);
void	MachEmulateLDC1(uint32_t, struct frame *, uint32_t);
void	MachEmulateSWC1(uint32_t, struct frame *, uint32_t);
void	MachEmulateSDC1(uint32_t, struct frame *, uint32_t);

void	bcemul_lb(uint32_t, struct frame *, uint32_t);
void	bcemul_lbu(uint32_t, struct frame *, uint32_t);
void	bcemul_lh(uint32_t, struct frame *, uint32_t);
void	bcemul_lhu(uint32_t, struct frame *, uint32_t);
void	bcemul_lw(uint32_t, struct frame *, uint32_t);
void	bcemul_lwl(uint32_t, struct frame *, uint32_t);
void	bcemul_lwr(uint32_t, struct frame *, uint32_t);
#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void	bcemul_ld(uint32_t, struct frame *, uint32_t);
void	bcemul_ldl(uint32_t, struct frame *, uint32_t);
void	bcemul_ldr(uint32_t, struct frame *, uint32_t);
#endif
void	bcemul_sb(uint32_t, struct frame *, uint32_t);
void	bcemul_sh(uint32_t, struct frame *, uint32_t);
void	bcemul_sw(uint32_t, struct frame *, uint32_t);
void	bcemul_swl(uint32_t, struct frame *, uint32_t);
void	bcemul_swr(uint32_t, struct frame *f, uint32_t);
#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void	bcemul_sd(uint32_t, struct frame *, uint32_t);
void	bcemul_sdl(uint32_t, struct frame *, uint32_t);
void	bcemul_sdr(uint32_t, struct frame *f, uint32_t);
#endif

/*
 * MIPS2 LL instruction emulation state
 */
struct {
	struct lwp *lwp;
	vaddr_t addr;
	uint32_t value;
} llstate;

/*
 * Analyse 'next' PC address taking account of branch/jump instructions
 */
vaddr_t
MachEmulateBranch(struct frame *f, vaddr_t instpc, unsigned fpuCSR,
    int allowNonBranch)
{
#define	BRANCHTARGET(i) (4 + ((i).word) + ((short)(i).IType.imm << 2))
	InstFmt inst;
	vaddr_t nextpc;

	if (instpc < MIPS_KSEG0_START)
		inst.word = fuiword((void *)instpc);
	else
		inst.word = *(unsigned *)instpc;

	switch ((int)inst.JType.op) {
	case OP_SPECIAL:
		if (inst.RType.func == OP_JR || inst.RType.func == OP_JALR)
			nextpc = f->f_regs[inst.RType.rs];
		else if (allowNonBranch)
			nextpc = instpc + 4;
		else
			panic("MachEmulateBranch: Non-branch instruction at "
			    "pc 0x%lx", (long)instpc);
		break;

	case OP_BCOND:
		switch ((int)inst.IType.rt) {
		case OP_BLTZ:
		case OP_BLTZAL:
		case OP_BLTZL:		/* squashed */
		case OP_BLTZALL:	/* squashed */
			if ((int)(f->f_regs[inst.RType.rs]) < 0)
				nextpc = BRANCHTARGET(inst);
			else
				nextpc = instpc + 8;
			break;

		case OP_BGEZ:
		case OP_BGEZAL:
		case OP_BGEZL:		/* squashed */
		case OP_BGEZALL:	/* squashed */
			if ((int)(f->f_regs[inst.RType.rs]) >= 0)
				nextpc = BRANCHTARGET(inst);
			else
				nextpc = instpc + 8;
			break;

		default:
			panic("MachEmulateBranch: Bad branch cond");
		}
		break;

	case OP_J:
	case OP_JAL:
		nextpc = (inst.JType.target << 2) |
			((unsigned)instpc & 0xF0000000);
		break;

	case OP_BEQ:
	case OP_BEQL:	/* squashed */
		if (f->f_regs[inst.RType.rs] == f->f_regs[inst.RType.rt])
			nextpc = BRANCHTARGET(inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_BNE:
	case OP_BNEL:	/* squashed */
		if (f->f_regs[inst.RType.rs] != f->f_regs[inst.RType.rt])
			nextpc = BRANCHTARGET(inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_BLEZ:
	case OP_BLEZL:	/* squashed */
		if ((int)(f->f_regs[inst.RType.rs]) <= 0)
			nextpc = BRANCHTARGET(inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_BGTZ:
	case OP_BGTZL:	/* squashed */
		if ((int)(f->f_regs[inst.RType.rs]) > 0)
			nextpc = BRANCHTARGET(inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_COP1:
		if (inst.RType.rs == OP_BCx || inst.RType.rs == OP_BCy) {
			int condition = (fpuCSR & MIPS_FPU_COND_BIT) != 0;
			if ((inst.RType.rt & COPz_BC_TF_MASK) != COPz_BC_TRUE)
				condition = !condition;
			if (condition)
				nextpc = BRANCHTARGET(inst);
			else
				nextpc = instpc + 8;
		}
		else if (allowNonBranch)
			nextpc = instpc + 4;
		else
			panic("MachEmulateBranch: Bad COP1 branch instruction");
		break;

	default:
		if (!allowNonBranch)
			panic("MachEmulateBranch: Non-branch instruction at "
			    "pc 0x%lx", (long)instpc);
		nextpc = instpc + 4;
	}
	return nextpc;
#undef	BRANCHTARGET
}

/*
 * Emulate instructions (including floating-point instructions)
 */
void
MachEmulateInst(uint32_t status, uint32_t cause, vaddr_t opc,
    struct frame *frame)
{
	uint32_t inst;
	ksiginfo_t ksi;

	/*
	 *  Fetch the instruction.
	 */
	if (cause & MIPS_CR_BR_DELAY)
		inst = ufetch_uint32((uint32_t *)opc+1);
	else
		inst = ufetch_uint32((uint32_t *)opc);

	switch (((InstFmt)inst).FRType.op) {
	case OP_LWC0:
		MachEmulateLWC0(inst, frame, cause);
		break;
	case OP_SWC0:
		MachEmulateSWC0(inst, frame, cause);
		break;
	case OP_SPECIAL:
		MachEmulateSpecial(inst, frame, cause);
		break;
	case OP_COP1:
		MachEmulateFP(inst, frame, cause);
		break;
#if defined(SOFTFLOAT)
	case OP_LWC1:
		MachEmulateLWC1(inst, frame, cause);
		break;
	case OP_LDC1:
		MachEmulateLDC1(inst, frame, cause);
		break;
	case OP_SWC1:
		MachEmulateSWC1(inst, frame, cause);
		break;
	case OP_SDC1:
		MachEmulateSDC1(inst, frame, cause);
		break;
#endif
	default:
#ifdef DEBUG
		printf("pid %d(%s): trap: bad vaddr %#"PRIxVADDR" cause %#x insn %#x\n", curproc->p_pid, curproc->p_comm, opc, cause, inst);
#endif
		frame->f_regs[_R_CAUSE] = cause;
		frame->f_regs[_R_BADVADDR] = opc;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = cause; /* XXX */
		ksi.ksi_code = SEGV_MAPERR;
		ksi.ksi_addr = (void *)opc;
		(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
		break;
	}
}

static inline void
send_sigsegv(intptr_t vaddr, uint32_t exccode, struct frame *frame,
    uint32_t cause)
{
	ksiginfo_t ksi;
	cause = (cause & 0xFFFFFF00) | (exccode << MIPS_CR_EXC_CODE_SHIFT);
	frame->f_regs[_R_CAUSE] = cause;
	frame->f_regs[_R_BADVADDR] = vaddr;
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGSEGV;
	ksi.ksi_trap = cause;
	ksi.ksi_code = SEGV_MAPERR;
	ksi.ksi_addr = (void *)vaddr;
	(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
}

static inline void
update_pc(struct frame *frame, uint32_t cause)
{

	if (cause & MIPS_CR_BR_DELAY)
		frame->f_regs[_R_PC] = 
		    MachEmulateBranch(frame, frame->f_regs[_R_PC],
			PCB_FSR(curpcb), 0);
	else
		frame->f_regs[_R_PC] += 4;
}

/*
 * MIPS2 LL instruction
 */
void
MachEmulateLWC0(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	t = &(frame->f_regs[(inst>>16)&0x1F]);

	if (copyin((void *)vaddr, t, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	llstate.lwp = curlwp;
	llstate.addr = vaddr;
	llstate.value = *((uint32_t *)t);

	update_pc(frame, cause);
}

/*
 * MIPS2 SC instruction
 */
void
MachEmulateSWC0(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	value;
	int16_t		offset;
	mips_reg_t	*t;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	t = (mips_reg_t *)&(frame->f_regs[(inst>>16)&0x1F]);

	/*
	 * Check that the process and address match the last
	 * LL instruction.
	 */
	if (curlwp == llstate.lwp && vaddr == llstate.addr) {
		llstate.lwp = NULL;
		/*
		 * Check that the data at the address hasn't changed
		 * since the LL instruction.
		 */
		if (copyin((void *)vaddr, &value, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
			return;
		}
		if (value == llstate.value) {
			/* SC successful */
			if (copyout(t, (void *)vaddr, 4) != 0) {
				send_sigsegv(vaddr, T_TLB_ST_MISS,
				    frame, cause);
				return;
			}
			*t = 1;
			update_pc(frame, cause);
			return;
		}
	}

	/* SC failed */
	*t = 0;
	update_pc(frame, cause);
}

void
MachEmulateSpecial(uint32_t inst, struct frame *frame, uint32_t cause)
{
	ksiginfo_t ksi;
	switch (((InstFmt)inst).RType.func) {
	case OP_SYNC:
		/* nothing */
		break;
	default:
		frame->f_regs[_R_CAUSE] = cause;
		frame->f_regs[_R_BADVADDR] = frame->f_regs[_R_PC];
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGSEGV;
		ksi.ksi_trap = cause;
		ksi.ksi_code = SEGV_MAPERR;
		ksi.ksi_addr = (void *)(intptr_t)frame->f_regs[_R_PC];
		(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
		break;
	}

	update_pc(frame, cause);
}

#if defined(SOFTFLOAT)

#define LWSWC1_MAXLOOP	12

void
MachEmulateLWC1(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;
	mips_reg_t	pc;
	int		i;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

	if (copyin((void *)vaddr, t, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	pc = frame->f_regs[_R_PC];
	update_pc(frame, cause);

	if (cause & MIPS_CR_BR_DELAY)
		return;

	for (i = 1; i < LWSWC1_MAXLOOP; i++) {
		if (mips_btop(frame->f_regs[_R_PC]) != mips_btop(pc))
			return;

		vaddr = frame->f_regs[_R_PC];	/* XXX truncates to 32 bits */
		inst = fuiword((uint32_t *)vaddr);
		if (((InstFmt)inst).FRType.op != OP_LWC1)
			return;

		offset = inst & 0xFFFF;
		vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

		/* segment and alignment check */
		if (vaddr < 0 || (vaddr & 3)) {
			send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
			return;
		}

		/* NewABI FIXME */
		t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

		if (copyin((void *)vaddr, t, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
			return;
		}

		pc = frame->f_regs[_R_PC];
		update_pc(frame, cause);
	}
}

void
MachEmulateLDC1(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0  || (vaddr & 7)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1E]);

	if (copyin((void *)vaddr, t, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
MachEmulateSWC1(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;
	mips_reg_t	pc;
	int		i;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

	if (copyout(t, (void *)vaddr, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	pc = frame->f_regs[_R_PC];
	update_pc(frame, cause);

	if (cause & MIPS_CR_BR_DELAY)
		return;

	for (i = 1; i < LWSWC1_MAXLOOP; i++) {
		if (mips_btop(frame->f_regs[_R_PC]) != mips_btop(pc))
			return;

		vaddr = frame->f_regs[_R_PC];	/* XXX truncates to 32 bits */
		inst = fuiword((uint32_t *)vaddr);
		if (((InstFmt)inst).FRType.op != OP_SWC1)
			return;

		offset = inst & 0xFFFF;
		vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

		/* segment and alignment check */
		if (vaddr < 0 || (vaddr & 3)) {
			send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
			return;
		}

		/* NewABI FIXME */
		t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

		if (copyout(t, (void *)vaddr, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
			return;
		}

		pc = frame->f_regs[_R_PC];
		update_pc(frame, cause);
	}
}

void
MachEmulateSDC1(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 7)) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1E]);

	if (copyout(t, (void *)vaddr, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_lb(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	int8_t		x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
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
bcemul_lbu(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	uint8_t	x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 1) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = (mips_ureg_t)x;

	update_pc(frame, cause);
}

void
bcemul_lh(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	int16_t		x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 1)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 2) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = x;

	update_pc(frame, cause);
}

void
bcemul_lhu(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	u_int16_t	x;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 1)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 2) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	frame->f_regs[(inst>>16)&0x1F] = (mips_ureg_t)x;

	update_pc(frame, cause);
}

void
bcemul_lw(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
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
bcemul_lwl(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
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
bcemul_lwr(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t		vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
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

#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void
bcemul_ld(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x7) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)vaddr, &(frame->f_regs[(inst>>16)&0x1F]), 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_ldl(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (7 - (vaddr & 0x7)) * 8;
	a <<= shift;
	x &= ~(~(uint64_t)0UL << shift);
	x |= a;

	frame->f_regs[(inst>>16)&0x1F] = x;

	update_pc(frame, cause);
}

void
bcemul_ldr(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (vaddr & 0x7) * 8;
	a >>= shift;
	x &= ~(~(uint64_t)0UL >> shift);
	x |= a;

	frame->f_regs[(inst>>16)&0x1F] = x;

	update_pc(frame, cause);
}
#endif /* defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64) */

void
bcemul_sb(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
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
bcemul_sh(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || vaddr & 1) {
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
bcemul_sw(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (ustore_uint32((void *)vaddr, frame->f_regs[(inst>>16)&0x1F]) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_swl(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 3)) * 8;
	x >>= shift;
	a &= ~(0xFFFFFFFFUL >> shift);
	a |= x;

	if (ustore_uint32((void *)vaddr, a) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_swr(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (vaddr & 3) * 8;
	x <<= shift;
	a &= ~(0xFFFFFFFFUL << shift);
	a |= x;

	if (ustore_uint32((void *)vaddr, a) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void
bcemul_sd(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || vaddr & 0x7) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyout((void *)vaddr, &frame->f_regs[(inst>>16)&0x1F], 8) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_sdl(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (7 - (vaddr & 7)) * 8;
	x >>= shift;
	a &= ~(~(uint64_t)0U >> shift);
	a |= x;

	if (copyout((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}

void
bcemul_sdr(uint32_t inst, struct frame *frame, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = frame->f_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, frame, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	x = frame->f_regs[(inst>>16)&0x1F];

	shift = (vaddr & 7) * 8;
	x <<= shift;
	a &= ~(~(uint64_t)0U << shift);
	a |= x;

	if (copyout((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, frame, cause);
		return;
	}

	update_pc(frame, cause);
}
#endif /* defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64) */
#endif /* defined(SOFTFLOAT) */
