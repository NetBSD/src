|	$NetBSD: x_unimp.sa,v 1.2 1994/10/26 07:50:32 cgd Exp $

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
|	x_unimp.sa 3.3 7/1/91
|
|	fpsp_unimp --- FPSP handler for unimplemented instruction	
|	exception.
|
| Invoked when the user program encounters a floating-point
| op-code that hardware does not support.  Trap vector# 11
| (See table 8-1 MC68030 User's Manual).
|
| 
| Note: An fsave for an unimplemented inst. will create a short
| fsave stack.
|
|  Input: 1. Six word stack frame for unimplemented inst, four word
|            for illegal
|            (See table 8-7 MC68030 User's Manual).
|         2. Unimp (short) fsave state frame created here by fsave
|            instruction.
|

|X_UNIMP	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	get_op
|	xref	do_func
|	xref	sto_res
|	xref	gen_except
|	xref	fpsp_fmt_error

	.global	fpsp_unimp
	.global	uni_2
fpsp_unimp:
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
uni_2:
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	fmovemx	%fp0-%fp3,%a6@(USER_FP0)
	fmoveml	%fpcr/%fpsr/%fpi,%a6@(USER_FPCR)
	moveb	%a7@,%d0		|test for valid version num
	andib	#0xf0,%d0		|test for 0x4x
	cmpib	#VER_4,%d0		|must be 0x4x or exit
	bnel	fpsp_fmt_error
|
|	Temporary D25B Fix
|	The following lines are used to ensure that the %FPSR
|	exception byte and condition codes are clear before proceeding
|
	movel	%a6@(USER_FPSR),%d0
	andl	#0xFF00FF,%d0		|clear all but accrued exceptions
	movel	%d0,%a6@(USER_FPSR)
	fmovel	#0,%FPSR		|clear all user bits
	fmovel	#0,%FPCR		|clear all user exceptions for FPSP

	clrb	%a6@(UFLG_TMP)		|clr flag for unsupp data

	bsrl	get_op		|go get operand(s)
	clrb	%a6@(STORE_FLG)
	bsrl	do_func		|do the function
	fsave	%a7@-		|capture possible exc state
	tstb	%a6@(STORE_FLG)
	bnes	no_store		|if STORE_FLG is set, no store
	bsrl	sto_res		|store the result in user space
no_store:
	bral	gen_except		|post any exceptions and return

|	end
