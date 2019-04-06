/*	$NetBSD: mips_emul.c,v 1.27 2019/04/06 03:06:26 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mips_emul.c,v 1.27 2019/04/06 03:06:26 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/proc.h>

#include <mips/locore.h>
#include <mips/mips_opcode.h>

#include <mips/reg.h>
#include <mips/regnum.h>			/* symbolic register indices */
#include <mips/pcb.h>
#include <mips/vmparam.h>			/* for VM_MAX_ADDRESS */
#include <mips/trap.h>

static inline void	send_sigsegv(intptr_t, uint32_t, struct trapframe *,
			    uint32_t);
static inline void	update_pc(struct trapframe *, uint32_t);

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
mips_emul_branch(struct trapframe *tf, vaddr_t instpc, uint32_t fpuCSR,
    bool allowNonBranch)
{
#define	BRANCHTARGET(pc, i) (4 + (pc) + ((short)(i).IType.imm << 2))
	InstFmt inst;
	vaddr_t nextpc;

	if (instpc < MIPS_KSEG0_START) {
		inst.word = mips_ufetch32((void *)instpc);
	} else {
		inst.word = *(uint32_t *)instpc;
	}

	switch ((int)inst.JType.op) {
	case OP_SPECIAL:
		if (inst.RType.func == OP_JR || inst.RType.func == OP_JALR)
			nextpc = tf->tf_regs[inst.RType.rs];
		else if (allowNonBranch)
			nextpc = instpc + 4;
		else
			panic("%s: %s instruction %08x at pc 0x%"PRIxVADDR,
			    __func__, "non-branch", inst.word, instpc);
		break;

	case OP_REGIMM:
		switch ((int)inst.IType.rt) {
		case OP_BLTZ:
		case OP_BLTZAL:
		case OP_BLTZL:		/* squashed */
		case OP_BLTZALL:	/* squashed */
			if ((int)(tf->tf_regs[inst.RType.rs]) < 0)
				nextpc = BRANCHTARGET(instpc, inst);
			else
				nextpc = instpc + 8;
			break;

		case OP_BGEZ:
		case OP_BGEZAL:
		case OP_BGEZL:		/* squashed */
		case OP_BGEZALL:	/* squashed */
			if ((int)(tf->tf_regs[inst.RType.rs]) >= 0)
				nextpc = BRANCHTARGET(instpc, inst);
			else
				nextpc = instpc + 8;
			break;

		default:
			panic("%s: %s instruction 0x%08x at pc 0x%"PRIxVADDR,
			    __func__, "bad branch", inst.word, instpc);
		}
		break;

	case OP_J:
	case OP_JAL:
		nextpc = (inst.JType.target << 2) |
			((intptr_t)instpc & 0xF0000000);
		break;

	case OP_BEQ:
	case OP_BEQL:	/* squashed */
		if (tf->tf_regs[inst.RType.rs] == tf->tf_regs[inst.RType.rt])
			nextpc = BRANCHTARGET(instpc, inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_BNE:
	case OP_BNEL:	/* squashed */
		if (tf->tf_regs[inst.RType.rs] != tf->tf_regs[inst.RType.rt])
			nextpc = BRANCHTARGET(instpc, inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_BLEZ:
	case OP_BLEZL:	/* squashed */
		if ((int)(tf->tf_regs[inst.RType.rs]) <= 0)
			nextpc = BRANCHTARGET(instpc, inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_BGTZ:
	case OP_BGTZL:	/* squashed */
		if ((int)(tf->tf_regs[inst.RType.rs]) > 0)
			nextpc = BRANCHTARGET(instpc, inst);
		else
			nextpc = instpc + 8;
		break;

	case OP_COP1:
		if (inst.RType.rs == OP_BCx || inst.RType.rs == OP_BCy) {
			int condition = (fpuCSR & MIPS_FPU_COND_BIT) != 0;
			if ((inst.RType.rt & COPz_BC_TF_MASK) != COPz_BC_TRUE)
				condition = !condition;
			if (condition)
				nextpc = BRANCHTARGET(instpc, inst);
			else
				nextpc = instpc + 8;
		}
		else if (allowNonBranch)
			nextpc = instpc + 4;
		else
			panic("%s: %s instruction 0x%08x at pc 0x%"PRIxVADDR,
			    __func__, "bad COP1 branch", inst.word, instpc);
		break;

	default:
		if (!allowNonBranch)
			panic("%s: %s instruction 0x%08x at pc 0x%"PRIxVADDR,
			    __func__, "non-branch", inst.word, instpc);
		nextpc = instpc + 4;
	}
	KASSERT((nextpc & 0x3) == 0);
	return nextpc;
#undef	BRANCHTARGET
}

/*
 * Emulate instructions (including floating-point instructions)
 */
void
mips_emul_inst(uint32_t status, uint32_t cause, vaddr_t opc,
    struct trapframe *tf)
{
	uint32_t inst;
	ksiginfo_t ksi;
	int code = ILL_ILLOPC;

	/*
	 *  Fetch the instruction.
	 */
	if (cause & MIPS_CR_BR_DELAY)
		inst = mips_ufetch32((uint32_t *)opc+1);
	else
		inst = mips_ufetch32((uint32_t *)opc);

	switch (((InstFmt)inst).FRType.op) {
	case OP_LWC0:
		mips_emul_lwc0(inst, tf, cause);
		break;
	case OP_SWC0:
		mips_emul_swc0(inst, tf, cause);
		break;
	case OP_SPECIAL:
		mips_emul_special(inst, tf, cause);
		break;
	case OP_SPECIAL3:
		mips_emul_special3(inst, tf, cause);
		break;
	case OP_COP1:
#if defined(FPEMUL)
		mips_emul_fp(inst, tf, cause);
		break;
#endif
	case OP_LWC1:
#if defined(FPEMUL)
		mips_emul_lwc1(inst, tf, cause);
		break;
#endif
	case OP_LDC1:
#if defined(FPEMUL)
		mips_emul_ldc1(inst, tf, cause);
		break;
#endif
	case OP_SWC1:
#if defined(FPEMUL)
		mips_emul_swc1(inst, tf, cause);
		break;
#endif
	case OP_SDC1:
#if defined(FPEMUL)
		mips_emul_sdc1(inst, tf, cause);
		break;
#else
		code = ILL_COPROC;
		/* FALLTHROUGH */
#endif
	default:
#ifdef DEBUG
		printf("pid %d (%s): trap: bad insn @ %#"PRIxVADDR" cause %#x insn %#x code %d\n", curproc->p_pid, curproc->p_comm, opc, cause, inst, code);
#endif
		tf->tf_regs[_R_CAUSE] = cause;
		tf->tf_regs[_R_BADVADDR] = opc;
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = cause; /* XXX */
		ksi.ksi_code = code;
		ksi.ksi_addr = (void *)opc;
		(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
		break;
	}
}

static inline void
send_sigsegv(intptr_t vaddr, uint32_t exccode, struct trapframe *tf,
    uint32_t cause)
{
	ksiginfo_t ksi;
	cause = (cause & ~0xFF) | (exccode << MIPS_CR_EXC_CODE_SHIFT);
	tf->tf_regs[_R_CAUSE] = cause;
	tf->tf_regs[_R_BADVADDR] = vaddr;
	KSI_INIT_TRAP(&ksi);
	ksi.ksi_signo = SIGSEGV;
	ksi.ksi_trap = cause;
	ksi.ksi_code = SEGV_MAPERR;
	ksi.ksi_addr = (void *)vaddr;
	(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
}

static inline void
update_pc(struct trapframe *tf, uint32_t cause)
{

	if (cause & MIPS_CR_BR_DELAY)
		tf->tf_regs[_R_PC] = 
		    mips_emul_branch(tf, tf->tf_regs[_R_PC],
			PCB_FSR(curpcb), 0);
	else
		tf->tf_regs[_R_PC] += 4;
}

/*
 * MIPS2 LL instruction
 */
void
mips_emul_lwc0(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	t = &(tf->tf_regs[(inst>>16)&0x1F]);

	if (copyin((void *)vaddr, t, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	llstate.lwp = curlwp;
	llstate.addr = vaddr;
	llstate.value = *((uint32_t *)t);

	update_pc(tf, cause);
}

/*
 * MIPS2 SC instruction
 */
void
mips_emul_swc0(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	value;
	int16_t		offset;
	mips_reg_t	*t;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	t = (mips_reg_t *)&(tf->tf_regs[(inst>>16)&0x1F]);

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
			send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
			return;
		}
		if (value == llstate.value) {
			/* SC successful */
			if (copyout(t, (void *)vaddr, 4) != 0) {
				send_sigsegv(vaddr, T_TLB_ST_MISS,
				    tf, cause);
				return;
			}
			*t = 1;
			update_pc(tf, cause);
			return;
		}
	}

	/* SC failed */
	*t = 0;
	update_pc(tf, cause);
}

void
mips_emul_special(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	ksiginfo_t ksi;
	const InstFmt instfmt = { .word = inst };

	switch (instfmt.RType.func) {
	case OP_SYNC:
		/* nothing */
		break;
	default:
		tf->tf_regs[_R_CAUSE] = cause;
		tf->tf_regs[_R_BADVADDR] = tf->tf_regs[_R_PC];
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = cause;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (void *)(intptr_t)tf->tf_regs[_R_PC];
		(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
		break;
	}

	update_pc(tf, cause);
}

void
mips_emul_special3(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	ksiginfo_t ksi;
	const InstFmt instfmt = { .word = inst };
	switch (instfmt.RType.func) {
	case OP_LX: {
		const intptr_t vaddr = tf->tf_regs[instfmt.RType.rs] 
		    + tf->tf_regs[instfmt.RType.rt];
		mips_reg_t r;
		int error = EFAULT;
		if (vaddr < 0) {
		    addr_err:
			send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
			return;
		}
		switch (instfmt.RType.shamt) {
#if !defined(__mips_o32)
		case OP_LX_LDX: {
			uint64_t tmp64;
			if (vaddr & 7)
				goto addr_err;
			error = copyin((void *)vaddr, &tmp64, sizeof(tmp64));
			r = tmp64;
			break;
		}
#endif
		case OP_LX_LWX: {
			int32_t tmp32;
			if (vaddr & 3)
				goto addr_err;
			error = copyin((void *)vaddr, &tmp32, sizeof(tmp32));
			r = tmp32;
			break;
		}
		case OP_LX_LHX: {
			int16_t tmp16;
			if (vaddr & 1)
				goto addr_err;
			error = copyin((void *)vaddr, &tmp16, sizeof(tmp16));
			r = tmp16;
			break;
		}
		case OP_LX_LBUX: {
			uint8_t tmp8;
			error = copyin((void *)vaddr, &tmp8, sizeof(tmp8));
			r = tmp8;
			break;
		}
		default:
			goto illopc;
		}
		if (error) {
			send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
			return;
		}
		tf->tf_regs[instfmt.RType.rd] = r;
		break;
	}
	case OP_RDHWR:
		switch (instfmt.RType.rd) {
		case 29:
			tf->tf_regs[instfmt.RType.rt] =
			    (mips_reg_t)(intptr_t)curlwp->l_private;
			goto done;
		}
		/* FALLTHROUGH */
	illopc:
	default:
		tf->tf_regs[_R_CAUSE] = cause;
		tf->tf_regs[_R_BADVADDR] = tf->tf_regs[_R_PC];
		KSI_INIT_TRAP(&ksi);
		ksi.ksi_signo = SIGILL;
		ksi.ksi_trap = cause;
		ksi.ksi_code = ILL_ILLOPC;
		ksi.ksi_addr = (void *)(intptr_t)tf->tf_regs[_R_PC];
		(*curproc->p_emul->e_trapsignal)(curlwp, &ksi);
		return;
	}
done:
	update_pc(tf, cause);
}

#if defined(FPEMUL)

#define LWSWC1_MAXLOOP	12

void
mips_emul_lwc1(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;
	mips_reg_t	pc;
	int		i;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

	if (copyin((void *)vaddr, t, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	pc = tf->tf_regs[_R_PC];
	update_pc(tf, cause);

	if (cause & MIPS_CR_BR_DELAY)
		return;

	for (i = 1; i < LWSWC1_MAXLOOP; i++) {
		if (mips_btop(tf->tf_regs[_R_PC]) != mips_btop(pc))
			return;

		vaddr = tf->tf_regs[_R_PC];	/* XXX truncates to 32 bits */
		inst = mips_ufetch32((uint32_t *)vaddr);
		if (((InstFmt)inst).FRType.op != OP_LWC1)
			return;

		offset = inst & 0xFFFF;
		vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

		/* segment and alignment check */
		if (vaddr < 0 || (vaddr & 3)) {
			send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
			return;
		}

		/* NewABI FIXME */
		t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

		if (copyin((void *)vaddr, t, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
			return;
		}

		pc = tf->tf_regs[_R_PC];
		update_pc(tf, cause);
	}
}

void
mips_emul_ldc1(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0  || (vaddr & 7)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1E]);

	if (copyin((void *)vaddr, t, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_swc1(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;
	mips_reg_t	pc;
	int		i;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

	if (copyout(t, (void *)vaddr, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	pc = tf->tf_regs[_R_PC];
	update_pc(tf, cause);

	if (cause & MIPS_CR_BR_DELAY)
		return;

	for (i = 1; i < LWSWC1_MAXLOOP; i++) {
		if (mips_btop(tf->tf_regs[_R_PC]) != mips_btop(pc))
			return;

		vaddr = tf->tf_regs[_R_PC];	/* XXX truncates to 32 bits */
		inst = mips_ufetch32((uint32_t *)vaddr);
		if (((InstFmt)inst).FRType.op != OP_SWC1)
			return;

		offset = inst & 0xFFFF;
		vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

		/* segment and alignment check */
		if (vaddr < 0 || (vaddr & 3)) {
			send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
			return;
		}

		/* NewABI FIXME */
		t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1F]);

		if (copyout(t, (void *)vaddr, 4) != 0) {
			send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
			return;
		}

		pc = tf->tf_regs[_R_PC];
		update_pc(tf, cause);
	}
}

void
mips_emul_sdc1(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	void		*t;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 7)) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	/* NewABI FIXME */
	t = &(curpcb->pcb_fpregs.r_regs[(inst>>16)&0x1E]);

	if (copyout(t, (void *)vaddr, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_lb(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	int8_t		x;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 1) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_lbu(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	uint8_t		x;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 1) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_lh(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	int16_t		x;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 1)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 2) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_lhu(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	uint16_t	x;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 1)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 2) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	tf->tf_regs[(inst>>16)&0x1F] = (mips_ureg_t)x;

	update_pc(tf, cause);
}

void
mips_emul_lw(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	int32_t		x;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_lwl(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 0x00000003)) * 8;
	a <<= shift;
	x &= ~(0xFFFFFFFFUL << shift);
	x |= a;

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_lwr(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (vaddr & 0x00000003) * 8;
	a >>= shift;
	x &= ~(0xFFFFFFFFUL >> shift);
	x |= a;

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void
mips_emul_lwu(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;
	uint32_t	x;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x3) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &x, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_ld(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr > VM_MAX_ADDRESS || vaddr & 0x7) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)vaddr, &(tf->tf_regs[(inst>>16)&0x1F]), 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_ldl(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr & 0x80000000) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (7 - (vaddr & 0x7)) * 8;
	a <<= shift;
	x &= ~(~(uint64_t)0UL << shift);
	x |= a;

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}

void
mips_emul_ldr(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_LD, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_LD_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (vaddr & 0x7) * 8;
	a >>= shift;
	x &= ~(~(uint64_t)0UL >> shift);
	x |= a;

	tf->tf_regs[(inst>>16)&0x1F] = x;

	update_pc(tf, cause);
}
#endif /* defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64) */

void
mips_emul_sb(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (ustore_8((void *)vaddr, tf->tf_regs[(inst>>16)&0x1F]) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_sh(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || vaddr & 1) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (ustore_16((void *)vaddr, tf->tf_regs[(inst>>16)&0x1F]) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_sw(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || (vaddr & 3)) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (ustore_32((void *)vaddr, tf->tf_regs[(inst>>16)&0x1F]) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_swl(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (3 - (vaddr & 3)) * 8;
	x >>= shift;
	a &= ~(0xFFFFFFFFUL >> shift);
	a |= x;

	if (ustore_32((void *)vaddr, a) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_swr(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint32_t	a, x, shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~3), &a, 4) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (vaddr & 3) * 8;
	x <<= shift;
	a &= ~(0xFFFFFFFFUL << shift);
	a |= x;

	if (ustore_32((void *)vaddr, a) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

#if defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64)
void
mips_emul_sd(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment and alignment check */
	if (vaddr < 0 || vaddr & 0x7) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (copyout((void *)vaddr, &tf->tf_regs[(inst>>16)&0x1F], 8) < 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_sdl(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (7 - (vaddr & 7)) * 8;
	x >>= shift;
	a &= ~(~(uint64_t)0U >> shift);
	a |= x;

	if (copyout((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}

void
mips_emul_sdr(uint32_t inst, struct trapframe *tf, uint32_t cause)
{
	intptr_t	vaddr;
	uint64_t	a, x;
	uint32_t	shift;
	int16_t		offset;

	offset = inst & 0xFFFF;
	vaddr = tf->tf_regs[(inst>>21)&0x1F] + offset;

	/* segment check */
	if (vaddr < 0) {
		send_sigsegv(vaddr, T_ADDR_ERR_ST, tf, cause);
		return;
	}

	if (copyin((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	x = tf->tf_regs[(inst>>16)&0x1F];

	shift = (vaddr & 7) * 8;
	x <<= shift;
	a &= ~(~(uint64_t)0U << shift);
	a |= x;

	if (copyout((void *)(vaddr & ~0x7), &a, 8) != 0) {
		send_sigsegv(vaddr, T_TLB_ST_MISS, tf, cause);
		return;
	}

	update_pc(tf, cause);
}
#endif /* defined(__mips_n32) || defined(__mips_n64) || defined(__mips_o64) */
#endif /* defined(FPEMUL) */
