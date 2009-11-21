/*	$NetBSD: fpu.c,v 1.26 2009/11/21 04:16:51 rmind Exp $ */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
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
 *	@(#)fpu.c	8.1 (Berkeley) 6/11/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fpu.c,v 1.26 2009/11/21 04:16:51 rmind Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/signalvar.h>

#include <machine/instr.h>
#include <machine/reg.h>

#include <sparc/fpu/fpu_emu.h>
#include <sparc/fpu/fpu_extern.h>

int fpe_debug = 0;

#ifdef DEBUG
/*
 * Dump a `fpn' structure.
 */
void
fpu_dumpfpn(struct fpn *fp)
{
	static const char *class[] = {
		"SNAN", "QNAN", "ZERO", "NUM", "INF"
	};

	printf("%s %c.%x %x %x %xE%d", class[fp->fp_class + 2],
		fp->fp_sign ? '-' : ' ',
		fp->fp_mant[0],	fp->fp_mant[1],
		fp->fp_mant[2], fp->fp_mant[3],
		fp->fp_exp);
}
#endif

/*
 * fpu_execute returns the following error numbers (0 = no error):
 */
#define	FPE		1	/* take a floating point exception */
#define	NOTFPU		2	/* not an FPU instruction */

/*
 * Translate current exceptions into `first' exception.  The
 * bits go the wrong way for ffs() (0x10 is most important, etc).
 * There are only 5, so do it the obvious way.
 */
#define	X1(x) x
#define	X2(x) x,x
#define	X4(x) x,x,x,x
#define	X8(x) X4(x),X4(x)
#define	X16(x) X8(x),X8(x)

static char cx_to_trapx[] = {
	X1(FSR_NX),
	X2(FSR_DZ),
	X4(FSR_UF),
	X8(FSR_OF),
	X16(FSR_NV)
};
static u_char fpu_codes_native[] = {
	X1(FPE_FLTRES),
	X2(FPE_FLTDIV),
	X4(FPE_FLTUND),
	X8(FPE_FLTOVF),
	X16(FPE_FLTINV)
};
#if defined(COMPAT_SUNOS)
static u_char fpu_codes_sunos[] = {
	X1(FPE_FLTINEX_TRAP),
	X2(FPE_FLTDIV_TRAP),
	X4(FPE_FLTUND_TRAP),
	X8(FPE_FLTOVF_TRAP),
	X16(FPE_FLTOPERR_TRAP)
};
extern struct emul emul_sunos;
#endif /* SUNOS_COMPAT */
/* Note: SVR4(Solaris) FPE_* codes happen to be compatible with ours */

/*
 * The FPU gave us an exception.  Clean up the mess.  Note that the
 * fp queue can only have FPops in it, never load/store FP registers
 * nor FBfcc instructions.  Experiments with `crashme' prove that
 * unknown FPops do enter the queue, however.
 */
int
fpu_cleanup(l, fs)
	struct lwp *l;
#ifndef SUN4U
	struct fpstate *fs;
#else /* SUN4U */
	struct fpstate64 *fs;
#endif /* SUN4U */
{
	int i, fsr = fs->fs_fsr, error;
	struct proc *p = l->l_proc;
	union instr instr;
	struct fpemu fe;
	u_char *fpu_codes;
	int code = 0;

	fpu_codes =
#ifdef COMPAT_SUNOS
		(p->p_emul == &emul_sunos) ? fpu_codes_sunos :
#endif
		fpu_codes_native;

	switch ((fsr >> FSR_FTT_SHIFT) & FSR_FTT_MASK) {

	case FSR_TT_NONE:
		panic("fpu_cleanup: No fault");	/* ??? */
		break;

	case FSR_TT_IEEE:
		DPRINTF(FPE_INSN, ("fpu_cleanup: FSR_TT_IEEE\n"));
		/* XXX missing trap address! */
		if ((i = fsr & FSR_CX) == 0)
			panic("fpu ieee trap, but no exception");
		code = fpu_codes[i - 1];
		break;		/* XXX should return, but queue remains */

	case FSR_TT_UNFIN:
		DPRINTF(FPE_INSN, ("fpu_cleanup: FSR_TT_UNFIN\n"));
#ifdef SUN4U
		if (fs->fs_qsize == 0) {
			printf("fpu_cleanup: unfinished fpop");
			/* The book sez reexecute or emulate. */
			return (0);
		}
		break;

#endif /* SUN4U */
	case FSR_TT_UNIMP:
		DPRINTF(FPE_INSN, ("fpu_cleanup: FSR_TT_UNIMP\n"));
		if (fs->fs_qsize == 0)
			panic("fpu_cleanup: unimplemented fpop");
		break;

	case FSR_TT_SEQ:
		panic("fpu sequence error");
		/* NOTREACHED */

	case FSR_TT_HWERR:
		DPRINTF(FPE_INSN, ("fpu_cleanup: FSR_TT_HWERR\n"));
		log(LOG_ERR, "fpu hardware error (%s[%d])\n",
		    p->p_comm, p->p_pid);
		uprintf("%s[%d]: fpu hardware error\n", p->p_comm, p->p_pid);
		code = SI_NOINFO;
		goto out;

	default:
		printf("fsr=0x%x\n", fsr);
		panic("fpu error");
	}

	/* emulate the instructions left in the queue */
	fe.fe_fpstate = fs;
	for (i = 0; i < fs->fs_qsize; i++) {
		instr.i_int = fs->fs_queue[i].fq_instr;
		if (instr.i_any.i_op != IOP_reg ||
		    (instr.i_op3.i_op3 != IOP3_FPop1 &&
		     instr.i_op3.i_op3 != IOP3_FPop2))
			panic("bogus fpu queue");
		error = fpu_execute(&fe, instr);
		if (error == 0)
			continue;

		switch (error) {
		case FPE:
			code = fpu_codes[(fs->fs_fsr & FSR_CX) - 1];
			break;

		case NOTFPU:
#ifdef SUN4U
#ifdef DEBUG
			printf("fpu_cleanup: not an FPU error -- sending SIGILL\n");
#endif
#endif /* SUN4U */
			code = SI_NOINFO;
			break;

		default:
			panic("fpu_cleanup 3");
			/* NOTREACHED */
		}
		/* XXX should stop here, but queue remains */
	}
out:
	fs->fs_qsize = 0;
	return (code);
}

#ifdef notyet
/*
 * If we have no FPU at all (are there any machines like this out
 * there!?) we have to emulate each instruction, and we need a pointer
 * to the trapframe so that we can step over them and do FBfcc's.
 * We know the `queue' is empty, though; we just want to emulate
 * the instruction at tf->tf_pc.
 */
fpu_emulate(l, tf, fs)
	struct lwp *l;
	struct trapframe *tf;
#ifndef SUN4U
	struct fpstate *fs;
#else /* SUN4U */
	struct fpstate64 *fs;
#endif /* SUN4U */
{

	do {
		fetch instr from pc
		decode
		if (integer instr) {
			struct pcb *pcb = lwp_getpcb(l);
			/*
			 * We do this here, rather than earlier, to avoid
			 * losing even more badly than usual.
			 */
			if (pcb->pcb_uw) {
				write_user_windows();
				if (rwindow_save(l))
					sigexit(l, SIGILL);
			}
			if (loadstore) {
				do_it;
				pc = npc, npc += 4
			} else if (fbfcc) {
				do_annul_stuff;
			} else
				return;
		} else if (fpu instr) {
			fe.fe_fsr = fs->fs_fsr &= ~FSR_CX;
			error = fpu_execute(&fe, fs, instr);
			switch (error) {
				etc;
			}
		} else
			return;
		if (want to reschedule)
			return;
	} while (error == 0);
}
#endif

/*
 * Execute an FPU instruction (one that runs entirely in the FPU; not
 * FBfcc or STF, for instance).  On return, fe->fe_fs->fs_fsr will be
 * modified to reflect the setting the hardware would have left.
 *
 * Note that we do not catch all illegal opcodes, so you can, for instance,
 * multiply two integers this way.
 */
int
fpu_execute(struct fpemu *fe, union instr instr)
{
	struct fpn *fp;
#ifndef SUN4U
	int opf, rs1, rs2, rd, type, mask, fsr, cx;
	struct fpstate *fs;
#else /* SUN4U */
	int opf, rs1, rs2, rd, type, mask, fsr, cx, i, cond;
	struct fpstate64 *fs;
#endif /* SUN4U */
	u_int space[4];

	/*
	 * `Decode' and execute instruction.  Start with no exceptions.
	 * The type of any i_opf opcode is in the bottom two bits, so we
	 * squish them out here.
	 */
	opf = instr.i_opf.i_opf;
	/*
	 * The low two bits of the opf field for floating point insns usually
	 * correspond to the operation width:
	 *
	 *	0:	Invalid
	 *	1:	Single precision float
	 *	2:	Double precision float
	 *	3:	Quad precision float
	 *
	 * The exceptions are the integer to float conversion instructions.
	 *
	 * For double and quad precision, the low bit if the rs or rd field
	 * is actually the high bit of the register number.
	 */

	type = opf & 3;
	mask = 0x3 >> (3 - type);

	rs1 = instr.i_opf.i_rs1;
	rs1 = (rs1 & ~mask) | ((rs1 & mask & 0x1) << 5);
	rs2 = instr.i_opf.i_rs2;
	rs2 = (rs2 & ~mask) | ((rs2 & mask & 0x1) << 5);
	rd = instr.i_opf.i_rd;
	rd = (rd & ~mask) | ((rd & mask & 0x1) << 5);
#ifdef DIAGNOSTIC
	if ((rs1 | rs2 | rd) & mask)
		/* This may be an FPU insn but it is illegal. */
		return (NOTFPU);
#endif
	fs = fe->fe_fpstate;
	fe->fe_fsr = fs->fs_fsr & ~FSR_CX;
	fe->fe_cx = 0;
#ifdef SUN4U
	/*
	 * Check to see if we're dealing with a fancy cmove and handle
	 * it first.
	 */
	if (instr.i_op3.i_op3 == IOP3_FPop2 && (opf&0xff0) != (FCMP&0xff0)) {
		switch (opf >>= 2) {
		case FMVFC0 >> 2:
			DPRINTF(FPE_INSN, ("fpu_execute: FMVFC0\n"));
			cond = (fs->fs_fsr>>FSR_FCC_SHIFT)&FSR_FCC_MASK;
			if (instr.i_fmovcc.i_cond != cond) return(0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVFC1 >> 2:
			DPRINTF(FPE_INSN, ("fpu_execute: FMVFC1\n"));
			cond = (fs->fs_fsr>>FSR_FCC1_SHIFT)&FSR_FCC_MASK;
			if (instr.i_fmovcc.i_cond != cond) return(0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVFC2 >> 2:
			DPRINTF(FPE_INSN, ("fpu_execute: FMVFC2\n"));
			cond = (fs->fs_fsr>>FSR_FCC2_SHIFT)&FSR_FCC_MASK;
			if (instr.i_fmovcc.i_cond != cond) return(0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVFC3 >> 2:
			DPRINTF(FPE_INSN, ("fpu_execute: FMVFC3\n"));
			cond = (fs->fs_fsr>>FSR_FCC3_SHIFT)&FSR_FCC_MASK;
			if (instr.i_fmovcc.i_cond != cond) return(0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVIC >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVIC\n"));
			cond = (curlwp->l_md.md_tf->tf_tstate>>TSTATE_CCR_SHIFT)&PSR_ICC;
			if (instr.i_fmovcc.i_cond != cond) return(0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVXC >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVXC\n"));
			cond = (curlwp->l_md.md_tf->tf_tstate>>(TSTATE_CCR_SHIFT+XCC_SHIFT))&PSR_ICC;
			if (instr.i_fmovcc.i_cond != cond) return(0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVRZ >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVRZ\n"));
			rs1 = instr.i_fmovr.i_rs1;
			if (rs1 != 0 && (int64_t)curlwp->l_md.md_tf->tf_global[rs1] != 0)
				return (0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVRLEZ >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVRLEZ\n"));
			rs1 = instr.i_fmovr.i_rs1;
			if (rs1 != 0 && (int64_t)curlwp->l_md.md_tf->tf_global[rs1] > 0)
				return (0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVRLZ >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVRLZ\n"));
			rs1 = instr.i_fmovr.i_rs1;
			if (rs1 == 0 || (int64_t)curlwp->l_md.md_tf->tf_global[rs1] >= 0)
				return (0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVRNZ >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVRNZ\n"));
			rs1 = instr.i_fmovr.i_rs1;
			if (rs1 == 0 || (int64_t)curlwp->l_md.md_tf->tf_global[rs1] == 0)
				return (0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVRGZ >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVRGZ\n"));
			rs1 = instr.i_fmovr.i_rs1;
			if (rs1 == 0 || (int64_t)curlwp->l_md.md_tf->tf_global[rs1] <= 0)
				return (0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		case FMVRGEZ >> 2:
			/* Presume we're curlwp */
			DPRINTF(FPE_INSN, ("fpu_execute: FMVRGEZ\n"));
			rs1 = instr.i_fmovr.i_rs1;
			if (rs1 != 0 && (int64_t)curlwp->l_md.md_tf->tf_global[rs1] < 0)
				return (0); /* success */
			rs1 = fs->fs_regs[rs2];
			goto mov;
		default:
			DPRINTF(FPE_INSN,
				("fpu_execute: unknown v9 FP inst %x opf %x\n",
					instr.i_int, opf));
			return (NOTFPU);
		}
	}
#endif /* SUN4U */
	switch (opf >>= 2) {

	default:
		DPRINTF(FPE_INSN,
			("fpu_execute: unknown basic FP inst %x opf %x\n",
				instr.i_int, opf));
		return (NOTFPU);

	case FMOV >> 2:		/* these should all be pretty obvious */
		DPRINTF(FPE_INSN, ("fpu_execute: FMOV\n"));
		rs1 = fs->fs_regs[rs2];
		goto mov;

	case FNEG >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FNEG\n"));
		rs1 = fs->fs_regs[rs2] ^ (1 << 31);
		goto mov;

	case FABS >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FABS\n"));
		rs1 = fs->fs_regs[rs2] & ~(1 << 31);
	mov:
#ifndef SUN4U
		fs->fs_regs[rd] = rs1;
#else /* SUN4U */
		i = 1<<(type-1);
		fs->fs_regs[rd++] = rs1;
		while (--i > 0)
			fs->fs_regs[rd++] = fs->fs_regs[++rs2];
#endif /* SUN4U */
		fs->fs_fsr = fe->fe_fsr;
		return (0);	/* success */

	case FSQRT >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FSQRT\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs2);
		fp = fpu_sqrt(fe);
		break;

	case FADD >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FADD\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		fp = fpu_add(fe);
		break;

	case FSUB >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FSUB\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		fp = fpu_sub(fe);
		break;

	case FMUL >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FMUL\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		fp = fpu_mul(fe);
		break;

	case FDIV >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FDIV\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		fp = fpu_div(fe);
		break;

	case FCMP >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FCMP\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		fpu_compare(fe, 0);
		goto cmpdone;

	case FCMPE >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FCMPE\n"));
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		fpu_compare(fe, 1);
	cmpdone:
		/*
		 * The only possible exception here is NV; catch it
		 * early and get out, as there is no result register.
		 */
		cx = fe->fe_cx;
		fsr = fe->fe_fsr | (cx << FSR_CX_SHIFT);
		if (cx != 0) {
			if (fsr & (FSR_NV << FSR_TEM_SHIFT)) {
				fs->fs_fsr = (fsr & ~FSR_FTT) |
				    (FSR_TT_IEEE << FSR_FTT_SHIFT);
				return (FPE);
			}
			fsr |= FSR_NV << FSR_AX_SHIFT;
		}
		fs->fs_fsr = fsr;
		return (0);

	case FSMULD >> 2:
	case FDMULX >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FSMULx\n"));
		if (type == FTYPE_EXT)
			return (NOTFPU);
		fpu_explode(fe, &fe->fe_f1, type, rs1);
		fpu_explode(fe, &fe->fe_f2, type, rs2);
		type++;	/* single to double, or double to quad */
		fp = fpu_mul(fe);
		break;

#ifdef SUN4U
	case FXTOS >> 2:
	case FXTOD >> 2:
	case FXTOQ >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FXTOx\n"));
		type = FTYPE_LNG;
		fpu_explode(fe, fp = &fe->fe_f1, type, rs2);
		type = opf & 3;	/* sneaky; depends on instruction encoding */
		break;

	case FTOX >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FTOX\n"));
		fpu_explode(fe, fp = &fe->fe_f1, type, rs2);
		type = FTYPE_LNG;
		/* Recalculate destination register */
		rd = instr.i_opf.i_rd;
		break;

#endif /* SUN4U */
	case FTOI >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FTOI\n"));
		fpu_explode(fe, fp = &fe->fe_f1, type, rs2);
		type = FTYPE_INT;
		/* Recalculate destination register */
		rd = instr.i_opf.i_rd;
		break;

	case FTOS >> 2:
	case FTOD >> 2:
	case FTOQ >> 2:
		DPRINTF(FPE_INSN, ("fpu_execute: FTOx\n"));
		fpu_explode(fe, fp = &fe->fe_f1, type, rs2);
		/* Recalculate rd with correct type info. */
		type = opf & 3;	/* sneaky; depends on instruction encoding */
		mask = 0x3 >> (3 - type);
		rd = instr.i_opf.i_rd;
		rd = (rd & ~mask) | ((rd & mask & 0x1) << 5);
		break;
	}

	/*
	 * ALU operation is complete.  Collapse the result and then check
	 * for exceptions.  If we got any, and they are enabled, do not
	 * alter the destination register, just stop with an exception.
	 * Otherwise set new current exceptions and accrue.
	 */
	fpu_implode(fe, fp, type, space);
	cx = fe->fe_cx;
	fsr = fe->fe_fsr;
	if (cx != 0) {
		mask = (fsr >> FSR_TEM_SHIFT) & FSR_TEM_MASK;
		if (cx & mask) {
			/* not accrued??? */
			fs->fs_fsr = (fsr & ~FSR_FTT) |
			    (FSR_TT_IEEE << FSR_FTT_SHIFT) |
			    (cx_to_trapx[(cx & mask) - 1] << FSR_CX_SHIFT);
			return (FPE);
		}
		fsr |= (cx << FSR_CX_SHIFT) | (cx << FSR_AX_SHIFT);
	}
	fs->fs_fsr = fsr;
	DPRINTF(FPE_REG, ("-> %c%d\n", (type == FTYPE_LNG) ? 'x' :
		((type == FTYPE_INT) ? 'i' :
			((type == FTYPE_SNG) ? 's' :
				((type == FTYPE_DBL) ? 'd' :
					((type == FTYPE_EXT) ? 'q' : '?')))),
		rd));
	fs->fs_regs[rd] = space[0];
	if (type >= FTYPE_DBL || type == FTYPE_LNG) {
		fs->fs_regs[rd + 1] = space[1];
		if (type > FTYPE_DBL) {
			fs->fs_regs[rd + 2] = space[2];
			fs->fs_regs[rd + 3] = space[3];
		}
	}
	return (0);	/* success */
}
