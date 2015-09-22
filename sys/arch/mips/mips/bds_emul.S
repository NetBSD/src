/*	$NetBSD: bds_emul.S,v 1.6.24.1 2015/09/22 12:05:47 skrll Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
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
 *	@(#)fp.s	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>

#include <mips/asm.h>
#include <mips/trap.h>			/* for T_RES_INST */
#include <mips/cpuregs.h>

#include "assym.h"

/* insns are reordered in the way as MIPS architecture imposes */
	.set	reorder

#define	REG_REGMASK	(0x1f << REG_SCALESHIFT)

/*
 * Emulate CPU instruction in branch delay slot.
 * These instructions are not implemented and causes SIGILL.
 *  jump/branch
 *  COP0
 *  trap/syscall/break
 *
 * Args are same as mips_emul_fp, eg.
 * void mips_emul_delayslot(uint32_t insn, struct trapframe *tf, uint32_t cause);
 * It should be used to emulate an instruction in a branch delay slot.
 */
LEAF(mips_emul_branchdelayslot)
	REG_PROLOGUE
	REG_S	zero, TF_REG_ZERO(a1)		# ensure zero has value 0
	REG_EPILOGUE
	move	t3, ra				# need to save this locally

	srl	t0, a0, 26-PTR_SCALESHIFT
	andi	t0, 0x3F << PTR_SCALESHIFT
	PTR_L	t9, bcemul_optbl(t0)
	jr	t9

	.rdata
bcemul_optbl:
	PTR_WORD bcemul_special			# 000 SPECIAL
	PTR_WORD bcemul_sigill			# 001 REGIMM
	PTR_WORD bcemul_sigill			# 002 J
	PTR_WORD bcemul_sigill			# 003 JAL
	PTR_WORD bcemul_sigill			# 004 BEQ
	PTR_WORD bcemul_sigill			# 005 BNE
	PTR_WORD bcemul_sigill			# 006 BLEZ
	PTR_WORD bcemul_sigill			# 007 BGTZ
	PTR_WORD bcemul_addi			# 010 ADDI
	PTR_WORD bcemul_addiu			# 011 ADDIU
	PTR_WORD bcemul_slti			# 012 SLTI
	PTR_WORD bcemul_sltiu			# 013 SLTIU
	PTR_WORD bcemul_andi			# 014 ANDI
	PTR_WORD bcemul_ori			# 015 ORI
	PTR_WORD bcemul_xori			# 016 XORI
	PTR_WORD bcemul_lui			# 017 LUI
	PTR_WORD bcemul_sigill			# 020 COP0
	PTR_WORD _C_LABEL(mips_emul_fp)		# 021 COP1
	PTR_WORD bcemul_sigill			# 022 COP2
	PTR_WORD bcemul_sigill			# 023 COP1X
	PTR_WORD bcemul_sigill			# 024 BEQL
	PTR_WORD bcemul_sigill			# 025 BNEL
	PTR_WORD bcemul_sigill			# 026 BNEZL
	PTR_WORD bcemul_sigill			# 027 BGTZL
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 030 DADDI (*)
	PTR_WORD bcemul_sigill			# 031 DADDIU (*)
	PTR_WORD bcemul_sigill			# 030 LDL (*)
	PTR_WORD bcemul_sigill			# 031 LDR (*)
#else
	PTR_WORD _C_LABEL(mips_emul_daddi)	# 030 DADDI (*)
	PTR_WORD _C_LABEL(mips_emul_daddiu)	# 031 DADDIU (*)
	PTR_WORD _C_LABEL(mips_emul_ldl)	# 032 LDL (*)
	PTR_WORD _C_LABEL(mips_emul_ldr)	# 033 LDR (*)
#endif
	PTR_WORD bcemul_sigill			# 034 SPECIAL2
	PTR_WORD bcemul_sigill			# 035 JALX
	PTR_WORD bcemul_sigill			# 036 MDMX
	PTR_WORD bcemul_sigill			# 037 SPECIAL3
	PTR_WORD _C_LABEL(mips_emul_lb)		# 040 LB
	PTR_WORD _C_LABEL(mips_emul_lh)		# 041 LH
	PTR_WORD _C_LABEL(mips_emul_lwl)	# 042 LWL
	PTR_WORD _C_LABEL(mips_emul_lw)		# 043 LW
	PTR_WORD _C_LABEL(mips_emul_lbu)	# 044 LBU
	PTR_WORD _C_LABEL(mips_emul_lhu)	# 045 LHU
	PTR_WORD _C_LABEL(mips_emul_lwr)	# 046 LWR
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 047 LWU (*)
#else
	PTR_WORD _C_LABEL(mips_emul_lwu)	# 047 LWU (*)
#endif
	PTR_WORD _C_LABEL(mips_emul_sb)		# 050 SB
	PTR_WORD _C_LABEL(mips_emul_sh)		# 051 SH
	PTR_WORD _C_LABEL(mips_emul_swl)	# 052 SWL
	PTR_WORD _C_LABEL(mips_emul_sw)		# 053 SW
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 054 SDL (*)
	PTR_WORD bcemul_sigill			# 055 SDR (*)
#else
	PTR_WORD _C_LABEL(mips_emul_sdl)	# 054 SDL (*)
	PTR_WORD _C_LABEL(mips_emul_sdr)	# 055 SDR (*)
#endif
	PTR_WORD _C_LABEL(mips_emul_swr)	# 056 SWR
	PTR_WORD bcemul_sigill			# 057 CACHE
	PTR_WORD bcemul_sigill			# 060 LL
	PTR_WORD _C_LABEL(mips_emul_lwc1)	# 061 LWC1
	PTR_WORD bcemul_sigill			# 062 LWC2
	PTR_WORD bcemul_sigill			# 063 PREF
	PTR_WORD bcemul_sigill			# 064 LLD (*)
	PTR_WORD _C_LABEL(mips_emul_ldc1)	# 065 LDC
	PTR_WORD bcemul_sigill			# 066 LDC2
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 067 LD (*)
#else
	PTR_WORD _C_LABEL(mips_emul_ld)		# 067 LD (*)
#endif
	PTR_WORD bcemul_sigill			# 070 SC
	PTR_WORD _C_LABEL(mips_emul_swc1)	# 071 SWC1
	PTR_WORD bcemul_sigill			# 072 SWC2
	PTR_WORD bcemul_sigill			# 073
	PTR_WORD bcemul_sigill			# 074 SCD (*)
	PTR_WORD _C_LABEL(mips_emul_sdc1)	# 075 SDC1
	PTR_WORD bcemul_sigill			# 076 SDC2
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 077 SD (*)
#else
	PTR_WORD _C_LABEL(mips_emul_sd)		# 077 SD (*)
#endif

	.text
bcemul_immed_prologue:
	srl	t1, a0, 21-REG_SCALESHIFT	# rs (source)
	srl	t2, a0, 16-REG_SCALESHIFT	# rt (dest)
	andi	t1, REG_REGMASK
	andi	t2, REG_REGMASK
	PTR_ADDU t1, a1
	PTR_ADDU t2, a1
	sll	v1, a0, 16
	sra	v1, v1, 16			# signed immed
	REG_PROLOGUE
	REG_L	v0, TF_REG_ZERO(t1)		# load source
	REG_EPILOGUE
	jr	ra				# execute the instruction

bcemul_uimmed_prologue:
	srl	t1, a0, 21-REG_SCALESHIFT	# rs (source)
	srl	t2, a0, 16-REG_SCALESHIFT	# rt (dest)
	andi	t1, REG_REGMASK
	andi	t2, REG_REGMASK
	PTR_ADDU t1, a1
	PTR_ADDU t2, a1
	andi	v1, a0, 0xffff			# unsigned immed
	REG_PROLOGUE
	REG_L	v0, TF_REG_ZERO(t1)		# load source
	REG_EPILOGUE
	jr	ra

#ifndef __mips_o32
bcemul_daddi:
	bal	bcemul_immed_prologue
	daddiu	t0, v0, v1
	b	bcemul_check_add_overflow
#endif

bcemul_addi:
	bal	bcemul_immed_prologue
	addu	t0, v0, v1

bcemul_check_add_overflow:
	move	ta0, t0		# result
	move	ta1, v0		# addend 1
	move	ta2, v1		# addend 2

bcemul_check_overflow:
	/* Overflow check */
	xor	ta2, ta1	/* negative + positive can't overflow */
	bltz	ta2, bcemul_store_and_done

	/* must have added two positive or two negative numbers */
	xor	ta1, ta0	/* result have same signedness as source? */
	bgez	ta1, bcemul_store_and_done /* yes, then we didn't overflow */

	b	bcemul_sigill

bcemul_addiu:
	bal	bcemul_immed_prologue
	addu	t0, v0, v1
	b	bcemul_store_and_done

#ifndef __mips_o32
bcemul_daddiu:
	bal	bcemul_immed_prologue
	daddu	t0, v0, v1
	b	bcemul_store_and_done
#endif

bcemul_slti:
	bal	bcemul_immed_prologue
	slt	t0, v0, v1
	b	bcemul_store_and_done

bcemul_sltiu:
	bal	bcemul_immed_prologue
	sltu	t0, v0, v1
	b	bcemul_store_and_done

bcemul_andi:
	bal	bcemul_uimmed_prologue
	and	t0, v0, v1
	b	bcemul_store_and_done

bcemul_ori:
	bal	bcemul_uimmed_prologue
	or	t0, v0, v1
	b	bcemul_store_and_done

bcemul_xori:
	bal	bcemul_uimmed_prologue
	xor	t0, v0, v1
	b	bcemul_store_and_done

bcemul_lui:
	srl	t2, a0, 16-REG_SCALESHIFT	# rt
	andi	t2, REG_REGMASK
	PTR_ADDU t2, a1
	sll	t0, a0, 16
	b	bcemul_store_and_done

	.rdata
bcemul_specialtbl:
	PTR_WORD bcemul_special_genshift	# 000 SLL
	PTR_WORD bcemul_sigill			# 001 MOVC1
	PTR_WORD bcemul_special_genshift	# 002 SRL
	PTR_WORD bcemul_special_genshift	# 003 SRA
	PTR_WORD bcemul_special_genshiftv	# 004 SLLV
	PTR_WORD bcemul_sigill			# 005
	PTR_WORD bcemul_special_genshiftv	# 006 SRLV
	PTR_WORD bcemul_special_genshiftv	# 007 SRAV
	PTR_WORD bcemul_sigill			# 010 JR
	PTR_WORD bcemul_sigill			# 011 JALR
	PTR_WORD bcemul_sigill			# 012 MOVZ
	PTR_WORD bcemul_sigill			# 013 MOVN
	PTR_WORD bcemul_sigill			# 014 SYSCALL
	PTR_WORD bcemul_sigill			# 015 BREAK
	PTR_WORD bcemul_sigill			# 016
	PTR_WORD bcemul_special_sync		# 017 SYNC
	PTR_WORD bcemul_special_mfhilo		# 020 MFHI
	PTR_WORD bcemul_special_mthilo		# 021 MTHI
	PTR_WORD bcemul_special_mfhilo		# 022 MFLO
	PTR_WORD bcemul_special_mthilo		# 023 MTLO
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 024 DSLLV (*)
	PTR_WORD bcemul_sigill			# 025
	PTR_WORD bcemul_sigill			# 026 DSRLV (*)
	PTR_WORD bcemul_sigill			# 027 DSRAV (*)
#else
	PTR_WORD bcemul_special_genshiftv	# 024 DSLLV (*)
	PTR_WORD bcemul_sigill			# 025
	PTR_WORD bcemul_special_genshiftv	# 026 DSRLV (*)
	PTR_WORD bcemul_special_genshiftv	# 027 DSRAV (*)
#endif
	PTR_WORD bcemul_special_genmultdiv	# 030 MULT
	PTR_WORD bcemul_special_genmultdiv	# 031 MULTU
	PTR_WORD bcemul_special_genmultdiv	# 032 DIV
	PTR_WORD bcemul_special_genmultdiv	# 033 DIVU
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 034 DMULT (*)
	PTR_WORD bcemul_sigill			# 035 DMULTU (*)
	PTR_WORD bcemul_sigill			# 036 DDIV (*)
	PTR_WORD bcemul_sigill			# 037 DDIVU (*)
#else
	PTR_WORD bcemul_special_genmultdiv	# 034 DMULT (*)
	PTR_WORD bcemul_special_genmultdiv	# 035 DMULTU (*)
	PTR_WORD bcemul_special_genmultdiv	# 036 DDIV (*)
	PTR_WORD bcemul_special_genmultdiv	# 037 DDIVU (*)
#endif
	PTR_WORD bcemul_special_genadd		# 040 ADD
	PTR_WORD bcemul_special_gen3op		# 041 ADDU
	PTR_WORD bcemul_special_gensub		# 042 SUB
	PTR_WORD bcemul_special_gen3op		# 043 SUBU
	PTR_WORD bcemul_special_gen3op		# 044 AND
	PTR_WORD bcemul_special_gen3op		# 045 OR
	PTR_WORD bcemul_special_gen3op		# 046 XOR
	PTR_WORD bcemul_special_gen3op		# 047 NOR
	PTR_WORD bcemul_sigill			# 050
	PTR_WORD bcemul_sigill			# 051
	PTR_WORD bcemul_special_gen3op		# 052 SLT
	PTR_WORD bcemul_special_gen3op		# 053 SLTU
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 054 DADD (*)
	PTR_WORD bcemul_sigill			# 055 DADDU (*)
	PTR_WORD bcemul_sigill			# 056 DSUB (*)
	PTR_WORD bcemul_sigill			# 057 DSUBU (*)
#else
	PTR_WORD bcemul_special_genadd		# 054 DADD (*)
	PTR_WORD bcemul_special_gen3op		# 055 DADDU (*)
	PTR_WORD bcemul_special_gensub		# 056 DSUB (*)
	PTR_WORD bcemul_special_gen3op		# 057 DSUBU (*)
#endif
	PTR_WORD bcemul_sigill			# 060 TGE
	PTR_WORD bcemul_sigill			# 061 TGEU
	PTR_WORD bcemul_sigill			# 062 TLT
	PTR_WORD bcemul_sigill			# 063 TLTU
	PTR_WORD bcemul_sigill			# 064 TEQ
	PTR_WORD bcemul_sigill			# 065
	PTR_WORD bcemul_sigill			# 066 TNE
	PTR_WORD bcemul_sigill			# 067
#ifdef __mips_o32
	PTR_WORD bcemul_sigill			# 070 DSLL (*)
	PTR_WORD bcemul_sigill			# 071
	PTR_WORD bcemul_sigill			# 072 DSRL (*)
	PTR_WORD bcemul_sigill			# 073 DSRA (*)
	PTR_WORD bcemul_sigill			# 074 DSLL32 (*)
	PTR_WORD bcemul_sigill			# 075
	PTR_WORD bcemul_sigill			# 076 DSRL32 (*)
	PTR_WORD bcemul_sigill			# 077 DSRA32 (*)
#else
	PTR_WORD bcemul_special_genshift	# 070 DSLL (*)
	PTR_WORD bcemul_sigill			# 071
	PTR_WORD bcemul_special_genshift	# 072 DSRL (*)
	PTR_WORD bcemul_special_genshift	# 073 DSRA (*)
	PTR_WORD bcemul_special_genshift	# 074 DSLL32 (*)
	PTR_WORD bcemul_sigill			# 075
	PTR_WORD bcemul_special_genshift	# 076 DSRL32 (*)
	PTR_WORD bcemul_special_genshift	# 077 DSRA32 (*)
#endif

	.text
	.set push
	.set noreorder
	.set nomacro
	.set mips64
bcemul_special_op:
	jr	ra;	sllv	t0, v0, v1	# 000 SLL
	jr	ra;	nop			# 001 MOVC1
	jr	ra;	srlv	t0, v0, v1	# 002 SRL
	jr	ra;	srav	t0, v0, v1	# 003 SRA
	jr	ra;	sllv	t0, v0, v1	# 004 SLLV
	jr	ra;	nop			# 005 *
	jr	ra;	srlv	t0, v0, v1	# 006 SRLV
	jr	ra;	srav	t0, v0, v1	# 007 SRAV
 
	jr	ra;	nop			# 010 JR
	jr	ra;	nop			# 011 JALR
	jr	ra;	movz	t0, v0, v1	# 012 MOVZ
	jr	ra;	movn	t0, v0, v1	# 013 MOVN
	jr	ra;	nop			# 014 SYSCALL
	jr	ra;	nop			# 015 BREAK
	jr	ra;	nop			# 016 *
	jr	ra;	nop			# 017 SYNC

	jr	ra;	mfhi	t0		# 020 MFHI
	jr	ra;	mthi	v0		# 021 MTHI
	jr	ra;	mflo	t0		# 022 MFLO
	jr	ra;	mtlo	v0		# 023 MTLO
	jr	ra;	dsllv	t0, v0, v1	# 024 DSLLV
	jr	ra;	nop			# 025 *
	jr	ra;	dsrlv	t0, v0, v1	# 026 DSRLV
	jr	ra;	dsrav	t0, v0, v1	# 027 DSRAV

	jr	ra;	mult	v0, v1		# 030 MULT
	jr	ra;	multu	v0, v1		# 031 MULTU
	jr	ra;	div	$0, v0, v1	# 032 DIV
	jr	ra;	divu	$0, v0, v1	# 033 DIVU
	jr	ra;	dmult	v0, v1		# 034 DMULT
	jr	ra;	dmultu	v0, v1		# 035 DMULTU
	jr	ra;	ddiv	$0, v0, v1	# 036 DDIV
	jr	ra;	ddivu	$0, v0, v1	# 037 DDIVU

	jr	ra;	add	t0, v0, v1	# 040 ADD
	jr	ra;	addu	t0, v0, v1	# 041 ADDU
	jr	ra;	sub	t0, v0, v1	# 042 SUB
	jr	ra;	subu	t0, v0, v1	# 043 SUBU
	jr	ra;	and	t0, v0, v1	# 044 AND
	jr	ra;	or	t0, v0, v1	# 045 OR
	jr	ra;	xor	t0, v0, v1	# 046 XOR
	jr	ra;	nor	t0, v0, v1	# 047 NOR

	jr	ra;	nop			# 050 *
	jr	ra;	nop			# 051 *
	jr	ra;	slt	t0, v0, v1	# 052 SLT
	jr	ra;	sltu	t0, v0, v1	# 053 SLTU
	jr	ra;	dadd	t0, v0, v1	# 054 DADD
	jr	ra;	daddu	t0, v0, v1	# 055 DADDU
	jr	ra;	dsub	t0, v0, v1	# 056 DSUB
	jr	ra;	dsubu	t0, v0, v1	# 057 DSUBU

	jr	ra;	tge	v0, v1		# 060 TGE
	jr	ra;	tgeu	v0, v1		# 061 TGEU
	jr	ra;	tlt	v0, v1		# 062 TLT
	jr	ra;	tltu	v0, v1		# 063 TLTU
	jr	ra;	teq	v0, v1		# 064 TEQ
	jr	ra;	nop			# 065 *
	jr	ra;	tne	v0, v1		# 066 TNE
	jr	ra;	nop			# 067 *

	jr	ra;	dsllv	t0, v0, v1	# 070 DSLL
	jr	ra;	nop			# 071 *
	jr	ra;	dsrlv	t0, v0, v1	# 072 DSRL
	jr	ra;	dsrav	t0, v0, v1	# 073 DSRA
	jr	ra;	dsllv	t0, v0, v1	# 074 DSLL32
	jr	ra;	nop			# 075 *
	jr	ra;	dsrlv	t0, v0, v1	# 076 DSRL32
	jr	ra;	dsrav	t0, v0, v1	# 077 DSRA32
	.set pop	

bcemul_special:
	andi	t0, a0, 0x3f			# get special code
	sll	t1, t0, 3			# calculate index in specialop
	sll	t0, PTR_SCALESHIFT
	PTR_L	t9, bcemul_specialtbl(t0)
	PTR_LA	t0, bcemul_special_op(t1)
	jr	t9

bcemul_special_3op_prologue:
	move	t9, t0
	srl	t0, a0, 21-REG_SCALESHIFT	# rs (source1)
	srl	t1, a0, 16-REG_SCALESHIFT	# rt (source2)
	srl	t2, a0, 11-REG_SCALESHIFT	# rd (dest)
	andi	t0, REG_REGMASK
	andi	t1, REG_REGMASK
	andi	t2, REG_REGMASK
	PTR_ADDU t0, a1
	PTR_ADDU t1, a1
	PTR_ADDU t2, a1
	REG_PROLOGUE
	REG_L	v0, TF_REG_ZERO(t0)		# load source1
	REG_L	v1, TF_REG_ZERO(t1)		# load source2
	REG_EPILOGUE
	jr	t9

bcemul_special_2src_prologue:
	move	t9, t0
	srl	t0, a0, 21-REG_SCALESHIFT	# rs (source1)
	srl	t1, a0, 16-REG_SCALESHIFT	# rt (source2)
	andi	t0, REG_REGMASK
	andi	t1, REG_REGMASK
	PTR_ADDU t0, a1
	PTR_ADDU t1, a1
	REG_PROLOGUE
	REG_L	v0, TF_REG_ZERO(t0)		# load source1
	REG_L	v1, TF_REG_ZERO(t1)		# load source2
	REG_EPILOGUE
	jr	t9

bcemul_special_genshift:
	move	t9, t0
	srl	t1, a0, 16-REG_SCALESHIFT	# rt (source)
	srl	t2, a0, 11-REG_SCALESHIFT	# rd (dest)
	srl	v1, a0, 6			# sa
	andi	t1, REG_REGMASK
	andi	t2, REG_REGMASK
	beqz	t2, bcemul_done			# fast escape for nop variants
	andi	v1, 0x001F
#ifndef __mips_o32
	/*
	 * Deal with DSLL32, DSRA32, DSRL32 who need 32 added to their
	 * shift count.
	 */
	andi	v0, a0, 077			# focus on special op
	sltiu	v0, v0, 074			# less than DSLL32?
	xori	v0, 1				# invert result
	sll	v0, 5				# shift by 5 (now 0 or 32)
	addu	v1, v0				# add to shift count
#endif
	PTR_ADDU t1, a1
	PTR_ADDU t2, a1
	REG_PROLOGUE
	REG_L	v0, TF_REG_ZERO(t1)		# load source
	REG_EPILOGUE
	jal	t9
	j	bcemul_store_and_done

bcemul_special_sync:
	b	bcemul_done

bcemul_special_mfhilo:
	srl	t2, a0, 11-REG_SCALESHIFT	# rd (dest)
	andi	t2, REG_REGMASK
	PTR_ADDU t1, a1, TF_REG_MULHI
	PTR_ADDU t2, a1
	and	v0, a0, 2			# bit 1: 0=HI 1=LO
	sll	v0, REG_SCALESHIFT - 1
#if TF_REG_MULLO < TF_REG_MULHI
	PTR_SUBU t1, v0
#else
	PTR_ADDU t1, v0
#endif
	REG_PROLOGUE
	REG_L	t0, TF_REG_ZERO(t1)		# load source (mullo or mulhi)
	REG_EPILOGUE
	b	bcemul_store_and_done

bcemul_special_mthilo:
	srl	t1, a0, 21-REG_SCALESHIFT	# rs (source)
	andi	t1, REG_REGMASK
	PTR_ADDU t1, a1
	PTR_ADDU t2, a1, TF_REG_MULHI
	and	v0, a0, 2			# bit 1: 0=HI l=LO
	sll	v0, REG_SCALESHIFT - 1
#if TF_REG_MULLO < TF_REG_MULHI
	PTR_SUBU t2, v0	
#else
	PTR_ADDU t2, v0
#endif
	REG_PROLOGUE
	REG_L	t0, TF_REG_ZERO(t1)
	REG_EPILOGUE
	b	bcemul_store_and_done

bcemul_special_genmultdiv:
	bal	bcemul_special_2src_prologue	# grab operands and do it.
	mflo	v0
	mfhi	v1
	REG_PROLOGUE
	REG_S	v0, TF_REG_MULLO(a1)
	REG_S	v1, TF_REG_MULHI(a1)
	REG_EPILOGUE
	b	bcemul_done

bcemul_special_genadd:
	bal	bcemul_special_3op_prologue
	b	bcemul_check_add_overflow

bcemul_special_gensub:
	bal	bcemul_special_3op_prologue

	/*
	 * t0 = v0 - v1 which means v0 = t0 + v1
	 * Now we can the same logic for overflow from addition.
	 */
	move	ta0, v0
	move	ta1, t0
	move	ta2, v1

	b	bcemul_check_overflow

bcemul_special_genshiftv:
bcemul_special_gen3op:
	bal	bcemul_special_3op_prologue
	#b	bcemul_store_and_done	# fallthrough to bcemul_store_and_done

bcemul_store_and_done:
	beq	a1, t2, bcemul_done	# don't store to zero
	REG_PROLOGUE
	REG_S	t0, TF_REG_ZERO(t2)
	REG_EPILOGUE
	#b	bcemul_done		# fallthrough to bcemul_done

bcemul_done:
/*
 * Succeeded to emulate instruction with no error
 * so compute the next PC.
 */
	PTR_SUBU sp, CALLFRAME_SIZ
	REG_S	t3, CALLFRAME_RA(sp)
	REG_S	s0, CALLFRAME_S0(sp)
	move	s0, a1			# save trapfame

	/* Fetch previous branch instruction */
	REG_PROLOGUE
	REG_L	a0, TF_REG_EPC(s0)
	REG_EPILOGUE
	jal	_C_LABEL(ufetch_uint32)

	/* Calculate branch destination */
	sll	t0, v0, 16
	sra	t0, t0, 16-2
	REG_PROLOGUE
	REG_L	t1, TF_REG_EPC(s0)
	PTR_ADDU t0, 4
	PTR_ADDU t1, t0
	REG_S	t1, TF_REG_EPC(s0)
	REG_EPILOGUE

	REG_L	ra, CALLFRAME_RA(sp)
	REG_L	s0, CALLFRAME_S0(sp)
	PTR_ADDU sp, CALLFRAME_SIZ
	jr	ra

/*
 * Send SIGILL, SIGFPE.
 * Args are same as mips_emul_fp.
 */
bcemul_sigill:
	li	t0, 0xFFFFFF00
	and	a2, a2, t0
	ori	a2, a2, T_RES_INST << MIPS_CR_EXC_CODE_SHIFT
	REG_PROLOGUE
	REG_S	a2, TF_REG_CAUSE(a1)
	REG_EPILOGUE

	move	ra, t3				# restore ra
	move	a1, a0				# code = instruction
	move	a0, MIPS_CURLWP			# get current process
	j	_C_LABEL(mips_fpuillinst)

END(mips_emul_branchdelayslot)
