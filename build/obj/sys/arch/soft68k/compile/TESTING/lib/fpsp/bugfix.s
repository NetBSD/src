|	$NetBSD: bugfix.sa,v 1.3 1994/10/26 07:48:55 cgd Exp $

|	MOTOROLA MICROPROCESSOR & MEMORY TECHNOLOGY GROUP
|	M68000 Hi-Performance Microprocessor Division
|	M68040 Software Package 
|
|	M68040 Software Package Copyright (c) 1993, 1994 Motorola Inc.
|	All rights reserved.
|
|	THE SOFTWARE is provided on an "AS IS" basis and without warranty.
|	To the maximum extent permitted by applicable law,
|	MOTOROLA DISCLAIMS ALL WARRANTIES WHETHER EXPRESS OR IMPLIED,
|	INCLUDING IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
|	PARTICULAR PURPOSE and any warranty against infringement with
|	regard to the SOFTWARE (INCLUDING ANY MODIFIED VERSIONS THEREOF)
|	and any accompanying written materials. 
|
|	To the maximum extent permitted by applicable law,
|	IN NO EVENT SHALL MOTOROLA BE LIABLE FOR ANY DAMAGES WHATSOEVER
|	(INCLUDING WITHOUT LIMITATION, DAMAGES FOR LOSS OF BUSINESS
|	PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR
|	OTHER PECUNIARY LOSS) ARISING OF THE USE OR INABILITY TO USE THE
|	SOFTWARE.  Motorola assumes no responsibility for the maintenance
|	and support of the SOFTWARE.  
|
|	You are hereby granted a copyright license to use, modify, and
|	distribute the SOFTWARE so long as this entire notice is retained
|	without alteration in any modified and/or redistributed versions,
|	and that such modified versions are clearly identified as such.
|	No licenses are granted by implication, estoppel or otherwise
|	under any patents or trademarks of Motorola, Inc.

|
|	bugfix.sa 3.2 1/31/91
|
|
|	This file contains workarounds for bugs in the 040
|	relating to the Floating-Point Software Package (FPSP)
|
|	Fixes for bugs: 1238
|
|	Bug: 1238 
|
|
|    /* The following dirty_bit clear should be left in
|     * the handler permanently to improve throughput.
|     * The dirty_bits are located at bits [23:16] in
|     * longword 0x08 in the busy frame 0x4x60.  Bit 16
|     * corresponds to %FP0, bit 17 corresponds to %FP1,
|     * and so on.
|     */
|    if  (E3_exception_just_serviced)   {
|         dirty_bit[cmdreg3b[9:7]] = 0;
|         }
|
|    if  (fsave_format_version != 0x40)  {goto NOFIX}
|
|    if !(E3_exception_just_serviced)   {goto NOFIX}
|    if  (cupc == 0000000)              {goto NOFIX}
|    if  ((cmdreg1b[15:13] != 000) &&
|         (cmdreg1b[15:10] != 010001))  {goto NOFIX}
|    if (((cmdreg1b[15:13] != 000) || ((cmdreg1b[12:10] != cmdreg2b[9:7]) &&
|				      (cmdreg1b[12:10] != cmdreg3b[9:7]))  ) &&
|	 ((cmdreg1b[ 9: 7] != cmdreg2b[9:7]) &&
|	  (cmdreg1b[ 9: 7] != cmdreg3b[9:7])) )  {goto NOFIX}
|
|    /* Note: for 6d43b or 8d43b, you may want to add the following code
|     * to get better coverage.  (If you do not insert this code, the part
|     * won't lock up; it will simply get the wrong answer.)
|     * Do NOT insert this code for 10d43b or later parts.
|     *
|     *  if (fpiarcu == integer stack return address) {
|     *       cupc = 0000000;
|     *       goto NOFIX;
|     *       }
|     */
|
|    if (cmdreg1b[15:13] != 000)   {goto FIX_OPCLASS2}
|    FIX_OPCLASS0:
|    if (((cmdreg1b[12:10] == cmdreg2b[9:7]) ||
|	 (cmdreg1b[ 9: 7] == cmdreg2b[9:7])) &&
|	(cmdreg1b[12:10] != cmdreg3b[9:7]) &&
|	(cmdreg1b[ 9: 7] != cmdreg3b[9:7]))  {  /* xu conflict only */
|	/* We execute the following code if there is an
|	   xu conflict and NOT an nu conflict */
|
|	/* first save some values on the fsave frame */
|	stag_temp     = STAG[fsave_frame];
|	cmdreg1b_temp = CMDREG1B[fsave_frame];
|	dtag_temp     = DTAG[fsave_frame];
|	ete15_temp    = ETE15[fsave_frame];
|
|	CUPC[fsave_frame] = 0000000;
|	FRESTORE
|	FSAVE
|
|	/* If the xu instruction is exceptional, we punt.
|	 * Otherwise, we would have to include OVFL/UNFL handler
|	 * code here to get the correct answer.
|	 */
|	if (fsave_frame_format == 0x4060) {goto KILL_PROCESS}
|
|	fsave_frame = /* build a long frame of all zeros */
|	fsave_frame_format = 0x4060;  /* label it as long frame */
|
|	/* load it with the temps we saved */
|	STAG[fsave_frame]     =  stag_temp;
|	CMDREG1B[fsave_frame] =  cmdreg1b_temp;
|	DTAG[fsave_frame]     =  dtag_temp;
|	ETE15[fsave_frame]    =  ete15_temp;
|
|	/* Make sure that the cmdreg3b dest reg is not going to
|	 * be destroyed by a FMOVEM at the end of all this code.
|	 * If it is, you should move the current value of the reg
|	 * onto the stack so that the reg will loaded with that value.
|	 */
|
|	/* All done.  Proceed with the code below */
|    }
|
|    etemp  = FP_reg_[cmdreg1b[12:10]];
|    ete15  = ~ete14;
|    cmdreg1b[15:10] = 010010;
|    clear(bug_flag_procIDxxxx);
|    FRESTORE and return;
|
|
|    FIX_OPCLASS2:
|    if ((cmdreg1b[9:7] == cmdreg2b[9:7]) &&
|	(cmdreg1b[9:7] != cmdreg3b[9:7]))  {  /* xu conflict only */
|	/* We execute the following code if there is an
|	   xu conflict and NOT an nu conflict */
|
|	/* first save some values on the fsave frame */
|	stag_temp     = STAG[fsave_frame];
|	cmdreg1b_temp = CMDREG1B[fsave_frame];
|	dtag_temp     = DTAG[fsave_frame];
|	ete15_temp    = ETE15[fsave_frame];
|	etemp_temp    = ETEMP[fsave_frame];
|
|	CUPC[fsave_frame] = 0000000;
|	FRESTORE
|	FSAVE
|
|
|	/* If the xu instruction is exceptional, we punt.
|	 * Otherwise, we would have to include OVFL/UNFL handler
|	 * code here to get the correct answer.
|	 */
|	if (fsave_frame_format == 0x4060) {goto KILL_PROCESS}
|
|	fsave_frame = /* build a long frame of all zeros */
|	fsave_frame_format = 0x4060;  /* label it as long frame */
|
|	/* load it with the temps we saved */
|	STAG[fsave_frame]     =  stag_temp;
|	CMDREG1B[fsave_frame] =  cmdreg1b_temp;
|	DTAG[fsave_frame]     =  dtag_temp;
|	ETE15[fsave_frame]    =  ete15_temp;
|	ETEMP[fsave_frame]    =  etemp_temp;
|
|	/* Make sure that the cmdreg3b dest reg is not going to
|	 * be destroyed by a FMOVEM at the end of all this code.
|	 * If it is, you should move the current value of the reg
|	 * onto the stack so that the reg will loaded with that value.
|	 */
|
|	/* All done.  Proceed with the code below */
|    }
|
|    if (etemp_exponent == min_sgl)   etemp_exponent = min_dbl;
|    if (etemp_exponent == max_sgl)   etemp_exponent = max_dbl;
|    cmdreg1b[15:10] = 010101;
|    clear(bug_flag_procIDxxxx);
|    FRESTORE and return;
|
|
|    NOFIX:
|    clear(bug_flag_procIDxxxx);
|    FRESTORE and return;
|

|BUGFIX    IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	fpsp_fmt_error

	.global	b1238_fix
b1238_fix:
|
| This code is entered only on completion of the handling of an 
| nu-generated ovfl, unfl, or inex exception.  If the version 
| number of the fsave is not 0x40, this handler is not necessary.
| Simply branch to fix_done and exit normally.
|
	cmpib	#VER_40,%a7@(4)
	bne	fix_done
|
| Test for cu_savepc equal to zero.  If not, this is not a bug
| #1238 case.
|
	moveb	%a6@(CU_SAVEPC),%d0
	andib	#0xFE,%d0
	beq	fix_done		|if zero, this is not bug #1238

|
| Test the register conflict aspect.  If opclass0, check for
| cu src equal to xu dest or equal to nu dest.  If so, go to 
| op0.  Else, or if opclass2, check for cu dest equal to
| xu dest or equal to nu dest.  If so, go to tst_opcl.  Else,
| exit, it is not the bug case.
|
| Check for opclass 0.  If not, go and check for opclass 2 and sgl.
|
	movew	%a6@(CMDREG1B),%d0
	andiw	#0xE000,%d0		|strip all but opclass
	bne	op2sgl		|not opclass 0, check op2
|
| Check for cu and nu register conflict.  If one exists, this takes
| priority over a cu and xu conflict. 
|
	bfextu	%a6@(CMDREG1B){#3:#3},%d0		|get 1st src 
	bfextu	%a6@(CMDREG3B){#6:#3},%d1		|get 3rd dest
	cmpb	%d0,%d1
	beqs	op0		|if equal, continue bugfix
|
| Check for cu dest equal to nu dest.  If so, go and fix the 
| bug condition.  Otherwise, exit.
|
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|get 1st dest 
	cmpb	%d0,%d1		|cmp 1st dest with 3rd dest
	beqs	op0		|if equal, continue bugfix
|
| Check for cu and xu register conflict.
|
	bfextu	%a6@(CMDREG2B){#6:#3},%d1		|get 2nd dest
	cmpb	%d0,%d1		|cmp 1st dest with 2nd dest
	beqs	op0_xu		|if equal, continue bugfix
	bfextu	%a6@(CMDREG1B){#3:#3},%d0		|get 1st src 
	cmpb	%d0,%d1		|cmp 1st src with 2nd dest
	beq	op0_xu
	bne	fix_done		|if the reg checks fail, exit
|
| We have the opclass 0 situation.
|
op0:
	bfextu	%a6@(CMDREG1B){#3:#3},%d0		|get source register no
	movel	#7,%d1
	subl	%d0,%d1
	clrl	%d0
	bset	%d1,%d0
	fmovemx	%d0,%a6@(ETEMP)		|load source to ETEMP

	moveb	#0x12,%d0
	bfins	%d0,%a6@(CMDREG1B){#0:#6}		|opclass 2, extended
|
|	Set ETEMP exponent bit 15 as the opposite of ete14
|
	btst	#6,%a6@(ETEMP_EX)		|check etemp exponent bit 14
	beq	setete15
	bclr	#etemp15_bit,%a6@(STAG)
	bra	finish
setete15:
	bset	#etemp15_bit,%a6@(STAG)
	bra	finish

|
| We have the case in which a conflict exists between the cu src or
| dest and the dest of the xu.  We must clear the instruction in 
| the cu and restore the state, allowing the instruction in the
| xu to complete.  Remember, the instruction in the nu
| was exceptional, and was completed by the appropriate handler.
| If the result of the xu instruction is not exceptional, we can
| restore the instruction from the cu to the frame and continue
| processing the original exception.  If the result is also
| exceptional, we choose to kill the process.
|
|	Items saved from the stack:
|	
|		0x3c stag     - L_SCR1
|		0x40 cmdreg1b - L_SCR2
|		0x44 dtag     - L_SCR3
|
| The cu savepc is set to zero, and the frame is restored to the
| fpu.
|
op0_xu:
	movel	%a6@(STAG),%a6@(L_SCR1)		|
	movel	%a6@(CMDREG1B),%a6@(L_SCR2)		|
	movel	%a6@(DTAG),%a6@(L_SCR3)
	andil	#0xe0000000,%a6@(L_SCR3)
	clrb	%a6@(CU_SAVEPC)
	movel	%a7@+,%d1		|save return address from bsr
	frestore	%a7@+
	fsave	%a7@-
|
| Check if the instruction which just completed was exceptional.
| 
	cmpw	#0x4060,%a7@
	beq	op0_xb
| 
| It is necessary to isolate the result of the instruction in the
| xu if it is to %fp0 - %fp3 and write that value to the USER_FPn
| locations on the stack.  The correct destination register is in 
| cmdreg2b.
|
	bfextu	%a6@(CMDREG2B){#6:#3},%d0		|get dest register no
	cmpil	#3,%d0
	bgts	op0_xi
	beqs	op0_fp3
	cmpil	#1,%d0
	blts	op0_fp0
	beqs	op0_fp1
op0_fp2:
	fmovemx	%fp2-%fp2,%a6@(USER_FP2)
	bras	op0_xi
op0_fp1:
	fmovemx	%fp1-%fp1,%a6@(USER_FP1)
	bras	op0_xi
op0_fp0:
	fmovemx	%fp0-%fp0,%a6@(USER_FP0)
	bras	op0_xi
op0_fp3:
	fmovemx	%fp3-%fp3,%a6@(USER_FP3)
|
| The frame returned is idle.  We must build a busy frame to hold
| the cu state information and setup etemp.
|
op0_xi:
	movel	#22,%d0		|clear 23 lwords
	clrl	%a7@
op0_loop:
	clrl	%a7@-
	dbf	%d0,op0_loop
	movel	#0x40600000,%a7@-
	movel	%a6@(L_SCR1),%a6@(STAG)
	movel	%a6@(L_SCR2),%a6@(CMDREG1B)
	movel	%a6@(L_SCR3),%a6@(DTAG)
	moveb	#0x6,%a6@(CU_SAVEPC)
	movel	%d1,%a7@-		|return bsr return address
	bfextu	%a6@(CMDREG1B){#3:#3},%d0		|get source register no
	movel	#7,%d1
	subl	%d0,%d1
	clrl	%d0
	bset	%d1,%d0
	fmovemx	%d0,%a6@(ETEMP)		|load source to ETEMP

	moveb	#0x12,%d0
	bfins	%d0,%a6@(CMDREG1B){#0:#6}		|opclass 2, extended
|
|	Set ETEMP exponent bit 15 as the opposite of ete14
|
	btst	#6,%a6@(ETEMP_EX)		|check etemp exponent bit 14
	beq	op0_sete15
	bclr	#etemp15_bit,%a6@(STAG)
	bra	finish
op0_sete15:
	bset	#etemp15_bit,%a6@(STAG)
	bra	finish

|
| The frame returned is busy.  It is not possible to reconstruct
| the code sequence to allow completion.  We will jump to 
| fpsp_fmt_error and allow the kernel to kill the process.
|
op0_xb:
	jmp	fpsp_fmt_error

|
| Check for opclass 2 and single size.  If not both, exit.
|
op2sgl:
	movew	%a6@(CMDREG1B),%d0
	andiw	#0xFC00,%d0		|strip all but opclass and size
	cmpiw	#0x4400,%d0		|test for opclass 2 and size=sgl
	bne	fix_done		|if not, it is not bug 1238
|
| Check for cu dest equal to nu dest or equal to xu dest, with 
| a cu and nu conflict taking priority an nu conflict.  If either,
| go and fix the bug condition.  Otherwise, exit.
|
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|get 1st dest 
	bfextu	%a6@(CMDREG3B){#6:#3},%d1		|get 3rd dest
	cmpb	%d0,%d1		|cmp 1st dest with 3rd dest
	beq	op2_com		|if equal, continue bugfix
	bfextu	%a6@(CMDREG2B){#6:#3},%d1		|get 2nd dest 
	cmpb	%d0,%d1		|cmp 1st dest with 2nd dest
	bne	fix_done		|if the reg checks fail, exit
|
| We have the case in which a conflict exists between the cu src or
| dest and the dest of the xu.  We must clear the instruction in 
| the cu and restore the state, allowing the instruction in the
| xu to complete.  Remember, the instruction in the nu
| was exceptional, and was completed by the appropriate handler.
| If the result of the xu instruction is not exceptional, we can
| restore the instruction from the cu to the frame and continue
| processing the original exception.  If the result is also
| exceptional, we choose to kill the process.
|
|	Items saved from the stack:
|	
|		0x3c stag     - L_SCR1
|		0x40 cmdreg1b - L_SCR2
|		0x44 dtag     - L_SCR3
|		etemp        - FP_SCR2
|
| The cu savepc is set to zero, and the frame is restored to the
| fpu.
|
op2_xu:
	movel	%a6@(STAG),%a6@(L_SCR1)		|
	movel	%a6@(CMDREG1B),%a6@(L_SCR2)		|
	movel	%a6@(DTAG),%a6@(L_SCR3)		|
	andil	#0xe0000000,%a6@(L_SCR3)
	clrb	%a6@(CU_SAVEPC)
	movel	%a6@(ETEMP),%a6@(FP_SCR2)
	movel	%a6@(ETEMP_HI),%a6@(FP_SCR2+4)
	movel	%a6@(ETEMP_LO),%a6@(FP_SCR2+8)
	movel	%a7@+,%d1		|save return address from bsr
	frestore	%a7@+
	fsave	%a7@-
|
| Check if the instruction which just completed was exceptional.
| 
	cmpw	#0x4060,%a7@
	beq	op2_xb
| 
| It is necessary to isolate the result of the instruction in the
| xu if it is to %fp0 - %fp3 and write that value to the USER_FPn
| locations on the stack.  The correct destination register is in 
| cmdreg2b.
|
	bfextu	%a6@(CMDREG2B){#6:#3},%d0		|get dest register no
	cmpil	#3,%d0
	bgts	op2_xi
	beqs	op2_fp3
	cmpil	#1,%d0
	blts	op2_fp0
	beqs	op2_fp1
op2_fp2:
	fmovemx	%fp2-%fp2,%a6@(USER_FP2)
	bras	op2_xi
op2_fp1:
	fmovemx	%fp1-%fp1,%a6@(USER_FP1)
	bras	op2_xi
op2_fp0:
	fmovemx	%fp0-%fp0,%a6@(USER_FP0)
	bras	op2_xi
op2_fp3:
	fmovemx	%fp3-%fp3,%a6@(USER_FP3)
|
| The frame returned is idle.  We must build a busy frame to hold
| the cu state information and fix up etemp.
|
op2_xi:
	movel	#22,%d0		|clear 23 lwords
	clrl	%a7@
op2_loop:
	clrl	%a7@-
	dbf	%d0,op2_loop
	movel	#0x40600000,%a7@-
	movel	%a6@(L_SCR1),%a6@(STAG)
	movel	%a6@(L_SCR2),%a6@(CMDREG1B)
	movel	%a6@(L_SCR3),%a6@(DTAG)
	moveb	#0x6,%a6@(CU_SAVEPC)
	movel	%a6@(FP_SCR2),%a6@(ETEMP)
	movel	%a6@(FP_SCR2+4),%a6@(ETEMP_HI)
	movel	%a6@(FP_SCR2+8),%a6@(ETEMP_LO)
	movel	%d1,%a7@-
	bra	op2_com

|
| We have the opclass 2 single source situation.
|
op2_com:
	moveb	#0x15,%d0
	bfins	%d0,%a6@(CMDREG1B){#0:#6}		|opclass 2, double

	cmpw	#0x407F,%a6@(ETEMP_EX)		|single +max
	bnes	case2
	movew	#0x43FF,%a6@(ETEMP_EX)		|to double +max
	bra	finish
case2:
	cmpw	#0xC07F,%a6@(ETEMP_EX)		|single -max
	bnes	case3
	movew	#0xC3FF,%a6@(ETEMP_EX)		|to double -max
	bra	finish
case3:
	cmpw	#0x3F80,%a6@(ETEMP_EX)		|single +min
	bnes	case4
	movew	#0x3C00,%a6@(ETEMP_EX)		|to double +min
	bra	finish
case4:
	cmpw	#0xBF80,%a6@(ETEMP_EX)		|single -min
	bne	fix_done
	movew	#0xBC00,%a6@(ETEMP_EX)		|to double -min
	bra	finish
|
| The frame returned is busy.  It is not possible to reconstruct
| the code sequence to allow completion.  fpsp_fmt_error causes
| an fline illegal instruction to be executed.
|
| You should replace the jump to fpsp_fmt_error with a jump
| to the entry point used to kill a process. 
|
op2_xb:
	jmp	fpsp_fmt_error

|
| Enter here if the case is not of the situations affected by
| bug #1238, or if the fix is completed, and exit.
|
finish:
fix_done:
	rts

|	end
