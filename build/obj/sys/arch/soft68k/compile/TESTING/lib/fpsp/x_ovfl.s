|	$NetBSD: x_ovfl.sa,v 1.3 2001/09/16 16:34:32 wiz Exp $

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
|	x_ovfl.sa 3.5 7/1/91
|
|	fpsp_ovfl --- FPSP handler for overflow exception
|
|	Overflow occurs when a floating-point intermediate result is
|	too large to be represented in a floating-point data register,
|	or when storing to memory, the contents of a floating-point
|	data register are too large to be represented in the
|	destination format.
|		
| Trap disabled results
|
| If the instruction is move_out, then garbage is stored in the
| destination.  If the instruction is not move_out, then the
| destination is not affected.  For 68881 compatibility, the
| following values should be stored at the destination, based
| on the current rounding mode:
|
|  RN	Infinity with the sign of the intermediate result.
|  RZ	Largest magnitude number, with the sign of the
|	intermediate result.
|  RM   For pos overflow, the largest pos number. For neg overflow,
|	-infinity
|  RP   For pos overflow, +infinity. For neg overflow, the largest
|	neg number
|
| Trap enabled results
| All trap disabled code applies.  In addition the exceptional
| operand needs to be made available to the users exception handler
| with a bias of 0x6000 subtracted from the exponent.
|

|X_OVFL	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	ovf_r_x2
|	xref	ovf_r_x3
|	xref	store
|	xref	real_ovfl
|	xref	real_inex
|	xref	fpsp_done
|	xref	g_opcls
|	xref	b1238_fix

	.global	fpsp_ovfl
fpsp_ovfl:
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	fmovemx	%fp0-%fp3,%a6@(USER_FP0)
	fmoveml	%fpcr/%fpsr/%fpi,%a6@(USER_FPCR)

|
|	The 040 doesn't set the AINEX bit in the %FPSR, the following
|	line temporarily rectifies this error.
|
	bset	#ainex_bit,%a6@(FPSR_AEXCEPT)
|
	bsrl	ovf_adj		|denormalize, round & store interm op
|
|	if overflow traps not enabled check for inexact exception
|
	btst	#ovfl_bit,%a6@(FPCR_ENABLE)
	beqs	ck_inex		|
|
	btst	#E3,%a6@(E_BYTE)
	beqs	no_e3_1
	bfextu	%a6@(CMDREG3B){#6:#3},%d0		|get dest reg no
	bclr	%d0,%a6@(FPR_DIRTY_BITS)		|clr dest dirty bit
	bsrl	b1238_fix
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
no_e3_1:
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_ovfl
|
| It is possible to have either inex2 or inex1 exceptions with the
| ovfl.  If the inex enable bit is set in the %FPCR, and either
| inex2 or inex1 occurred, we must clean up and branch to the
| real inex handler.
|
ck_inex:
|	move.b		%a6@(FPCR_ENABLE),%d0
|	and.b		%a6@(FPSR_EXCEPT),%d0
|	andi.b		#0x3,%d0
	btst	#inex2_bit,%a6@(FPCR_ENABLE)
	beqs	ovfl_exit
|
| Inexact enabled and reported, and we must take an inexact exception.
|
take_inex:
	btst	#E3,%a6@(E_BYTE)
	beqs	no_e3_2
	bfextu	%a6@(CMDREG3B){#6:#3},%d0		|get dest reg no
	bclr	%d0,%a6@(FPR_DIRTY_BITS)		|clr dest dirty bit
	bsrl	b1238_fix
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
	
ovfl_exit:
	bclr	#E3,%a6@(E_BYTE)		|test and clear E3 bit
	beqs	e1_set
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
|	ovf_adj
|
ovf_adj:
|
| Have %a0 point to the correct operand. 
|
	btst	#E3,%a6@(E_BYTE)		|test E3 bit
	beqs	ovf_e1

	lea	%a6@(WBTEMP),%a0
	bras	ovf_com
ovf_e1:
	lea	%a6@(ETEMP),%a0

ovf_com:
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)

	bsrl	g_opcls		|returns opclass in %d0
	cmpiw	#3,%d0		|check for opclass3
	bnes	not_opc011

|
| FPSR_CC is saved and restored because ovf_r_x3 affects it. The
| CCs are defined to be 'not affected' for the opclass3 instruction.
|
	moveb	%a6@(FPSR_CC),%a6@(L_SCR1)
	bsrl	ovf_r_x3		|returns %a0 pointing to result
	moveb	%a6@(L_SCR1),%a6@(FPSR_CC)
	bral	store		|stores to memory or register
	
not_opc011:
	bsrl	ovf_r_x2		|returns %a0 pointing to result
	bral	store		|stores to memory or register

|	end
