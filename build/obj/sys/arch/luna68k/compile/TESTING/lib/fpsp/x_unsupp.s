|	$NetBSD: x_unsupp.sa,v 1.2 1994/10/26 07:50:33 cgd Exp $

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
|	x_unsupp.sa 3.3 7/1/91
|
|	fpsp_unsupp --- FPSP handler for unsupported data type exception
|
| Trap vector #55	(See table 8-1 Mc68030 User's manual).	
| Invoked when the user program encounters a data format (packed) that
| hardware does not support or a data type (denormalized numbers or un-
| normalized numbers).
| Normalizes denorms and unnorms, unpacks packed numbers then stores 
| them back into the machine to let the 040 finish the operation.  
|
| Unsupp calls two routines:
| 	1. get_op -  gets the operand(s)
| 	2. res_func - restore the function back into the 040 or
| 			if fmove.p fpm,<ea> then pack source (fpm)
| 			and store in users memory <ea>.
|
|  Input: Long fsave stack frame
|

|X_UNSUPP	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	get_op
|	xref	res_func
|	xref	gen_except
|	xref	fpsp_fmt_error

	.global	fpsp_unsupp
fpsp_unsupp:
|
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	fmovemx	%fp0-%fp3,%a6@(USER_FP0)
	fmoveml	%fpcr/%fpsr/%fpi,%a6@(USER_FPCR)


	moveb	%a7@,%a6@(VER_TMP)		|save version number
	moveb	%a7@,%d0		|test for valid version num
	andib	#0xf0,%d0		|test for 0x4x
	cmpib	#VER_4,%d0		|must be 0x4x or exit
	bnel	fpsp_fmt_error

	fmovel	#0,%FPSR		|clear all user status bits
	fmovel	#0,%FPCR		|clear all user control bits
|
|	The following lines are used to ensure that the %FPSR
|	exception byte and condition codes are clear before proceeding,
|	except in the case of fmove, which leaves the cc's intact.
|
unsupp_con:
	movel	%a6@(USER_FPSR),%d1
	btst	#5,%a6@(CMDREG1B)		|looking for fmove out
	bne	fmove_con
	andl	#0xFF00FF,%d1		|clear all but aexcs and qbyte
	bras	end_fix
fmove_con:
	andl	#0x0FFF40FF,%d1		|clear all but cc's, snan bit, aexcs, and qbyte
end_fix:
	movel	%d1,%a6@(USER_FPSR)

	st	%a6@(UFLG_TMP)		|set flag for unsupp data

	bsrl	get_op		|everything okay, go get operand(s)
	bsrl	res_func		|fix up stack frame so can restore it
	clrl	%a7@-
	moveb	%a6@(VER_TMP),%a7@		|move idle fmt word to top of stack
	bral	gen_except
|
|	end
