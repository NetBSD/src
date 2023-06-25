|	$NetBSD: x_unfl.sa,v 1.5 2022/01/25 22:01:34 andvar Exp $

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
|	x_unfl.sa 3.4 7/1/91
|
|	fpsp_unfl --- FPSP handler for underflow exception
|
| Trap disabled results
|	For 881/2 compatibility, sw must denormalize the intermediate 
| result, then store the result.  Denormalization is accomplished 
| by taking the intermediate result (which is always normalized) and 
| shifting the mantissa right while incrementing the exponent until 
| it is equal to the denormalized exponent for the destination 
| format.  After denormalization, the result is rounded to the 
| destination format.
|		
| Trap enabled results
| 	All trap disabled code applies.	In addition the exceptional 
| operand needs to made available to the user with a bias of 0x6000 
| added to the exponent.
|

|X_UNFL	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	denorm
|	xref	round
|	xref	store
|	xref	g_rndpr
|	xref	g_opcls
|	xref	g_dfmtou
|	xref	real_unfl
|	xref	real_inex
|	xref	fpsp_done
|	xref	b1238_fix

	.global	fpsp_unfl
fpsp_unfl:
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	fmovemx	%fp0-%fp3,%a6@(USER_FP0)
	fmoveml	%fpcr/%fpsr/%fpi,%a6@(USER_FPCR)

|
	bsrl	unf_res		|denormalize, round & store interm op
|
| If underflow exceptions are not enabled, check for inexact
| exception
|
	btst	#unfl_bit,%a6@(FPCR_ENABLE)
	beqs	ck_inex

	btst	#E3,%a6@(E_BYTE)
	beqs	no_e3_1
|
| Clear dirty bit on dest resister in the frame before branching
| to b1238_fix.
|
	bfextu	%a6@(CMDREG3B){#6:#3},%d0		|get dest reg no
	bclr	%d0,%a6@(FPR_DIRTY_BITS)		|clr dest dirty bit
	bsrl	b1238_fix		|test for bug1238 case
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
no_e3_1:
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_unfl
|
| It is possible to have either inex2 or inex1 exceptions with the
| unfl.  If the inex enable bit is set in the %FPCR, and either
| inex2 or inex1 occurred, we must clean up and branch to the
| real inex handler.
|
ck_inex:
	moveb	%a6@(FPCR_ENABLE),%d0
	andb	%a6@(FPSR_EXCEPT),%d0
	andib	#0x3,%d0
	beqs	unfl_done

|
| Inexact enabled and reported, and we must take an inexact exception
|	
take_inex:
	btst	#E3,%a6@(E_BYTE)
	beqs	no_e3_2
|
| Clear dirty bit on dest resister in the frame before branching
| to b1238_fix.
|
	bfextu	%a6@(CMDREG3B){#6:#3},%d0		|get dest reg no
	bclr	%d0,%a6@(FPR_DIRTY_BITS)		|clr dest dirty bit
	bsrl	b1238_fix		|test for bug1238 case
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
no_e3_2:
	moveb	#INEX_VEC,%a6@(EXC_VEC+1)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_inex

unfl_done:
	bclr	#E3,%a6@(E_BYTE)
	beqs	e1_set		|if set then branch
|
| Clear dirty bit on dest resister in the frame before branching
| to b1238_fix.
|
	bfextu	%a6@(CMDREG3B){#6:#3},%d0		|get dest reg no
	bclr	%d0,%a6@(FPR_DIRTY_BITS)		|clr dest dirty bit
	bsrl	b1238_fix		|test for bug1238 case
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	fpsp_done
e1_set:
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	unlk	%a6
	bral	fpsp_done
|
|	unf_res --- underflow result calculation
|
unf_res:
	bsrl	g_rndpr		|returns RND_PREC in %d0 0=ext,
|					;1=sgl, 2=dbl
|					;we need the RND_PREC in the
|					;upper word for round
	clrw	%a7@-		|
	movew	%d0,%a7@-		|copy RND_PREC to stack
|
|
| If the exception bit set is E3, the exceptional operand from the
| fpu is in WBTEMP; else it is in FPTEMP.
|
	btst	#E3,%a6@(E_BYTE)
	beqs	unf_E1
unf_E3:
	lea	%a6@(WBTEMP),%a0		|%a0 now points to operand
|
| Test for fsgldiv and fsglmul.  If the inst was one of these, then
| force the precision to extended for the denorm routine.  Use
| the user's precision for the round routine.
|
	movew	%a6@(CMDREG3B),%d1		|check for fsgldiv or fsglmul
	andiw	#0x7f,%d1
	cmpiw	#0x30,%d1		|check for sgldiv
	beqs	unf_sgl
	cmpiw	#0x33,%d1		|check for sglmul
	bnes	unf_cont		|if not, use %fpcr prec in round
unf_sgl:
	clrl	%d0
	movew	#0x1,%a7@		|override g_rndpr precision
|					;force single
	bras	unf_cont
unf_E1:
	lea	%a6@(FPTEMP),%a0		|%a0 now points to operand
unf_cont:
	bclr	#sign_bit,%a0@(LOCAL_EX)		|clear sign bit
	sne	%a0@(LOCAL_SGN)		|store sign

	bsrl	denorm		|returns denorm, %a0 points to it
|
| WARNING:
|				;%d0 has guard,round sticky bit
|				;make sure that it is not corrupted
|				;before it reaches the round subroutine
|				;also ensure that %a0 isn't corrupted

|
| Set up %d1 for round subroutine %d1 contains the PREC/MODE
| information respectively on upper/lower register halves.
|
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		|get mode from %FPCR
|						;mode in lower %d1
	addl	%a7@+,%d1		|merge PREC/MODE
|
| WARNING: %a0 and %d0 are assumed to be intact between the denorm and
| round subroutines. All code between these two subroutines
| must not corrupt %a0 and %d0.
|
|
| Perform Round	
|	Input:		%a0 points to input operand
|			%d0{#31:#29} has guard, round, sticky
|			%d1{#01:#00} has rounding mode
|			%d1{#17:#16} has rounding precision
|	Output:		%a0 points to rounded operand
|

	bsrl	round		|returns rounded denorm at %a0@
|
| Differentiate between store to memory vs. store to register
|
unf_store:
	bsrl	g_opcls		|returns opclass in %d0{#2:#0}
	cmpib	#0x3,%d0
	bnes	not_opc011
|
| At this point, a store to memory is pending
|
opc011:
	bsrl	g_dfmtou
	tstb	%d0
	beqs	ext_opc011		|If extended, do not subtract
| 				;If destination format is sgl/dbl, 
	tstb	%a0@(LOCAL_HI)		|If rounded result is normal,don't
|					;subtract
	bmis	ext_opc011
	subqw	#1,%a0@(LOCAL_EX)		|account for denorm bias vs.
|				;normalized bias
|				;          normalized   denormalized
|				;single       0x7f           0x7e
|				;double       0x3ff          0x3fe
|
ext_opc011:
	bsrl	store		|stores to memory
	bras	unf_done		|finish up

|
| At this point, a store to a float register is pending
|
not_opc011:
	bsrl	store		|stores to float register
|				;%a0 is not corrupted on a store to a
|				;float register.
|
| Set the condition codes according to result
|
	tstl	%a0@(LOCAL_HI)		|check upper mantissa
	bnes	ck_sgn
	tstl	%a0@(LOCAL_LO)		|check lower mantissa
	bnes	ck_sgn
	bset	#z_bit,%a6@(FPSR_CC)		|set condition codes if zero
ck_sgn:
	btst	#sign_bit,%a0@(LOCAL_EX)		|check the sign bit
	beqs	unf_done
	bset	#neg_bit,%a6@(FPSR_CC)

|
| Finish.  
|
unf_done:
	btst	#inex2_bit,%a6@(FPSR_EXCEPT)
	beqs	no_aunfl
	bset	#aunfl_bit,%a6@(FPSR_AEXCEPT)
no_aunfl:
	rts

|	end
