|	$NetBSD: x_fline.sa,v 1.2 1994/10/26 07:50:23 cgd Exp $

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
|	x_fline.sa 3.3 1/10/91
|
|	fpsp_fline --- FPSP handler for fline exception
|
|	First determine if the exception is one of the unimplemented
|	floating point instructions.  If so, let fpsp_unimp handle it.
|	Next, determine if the instruction is an fmovecr with a non-zero
|	<ea> field.  If so, handle here and return.  Otherwise, it
|	must be a real F-line exception.
|

|X_FLINE	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	real_fline
|	xref	fpsp_unimp
|	xref	uni_2
|	xref	mem_read
|	xref	fpsp_fmt_error

	.global	fpsp_fline
fpsp_fline:
|
|	check for unimplemented vector first.  Use EXC_VEC-4 because
|	the equate is valid only after a 'link %a6' has pushed one more
|	long onto the stack.
|
	cmpw	#UNIMP_VEC,%a7@(EXC_VEC-4)
	beql	fpsp_unimp

|
|	fmovecr with non-zero <ea> handling here
|
	subl	#4,%a7		|4 accounts for 2-word difference
|				;between six word frame (unimp) and
|				;four word frame
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	moveal	%a6@(EXC_PC+4),%a0		|get address of fline instruction
	lea	%a6@(L_SCR1),%a1		|use L_SCR1 as scratch
	movel	#4,%d0
	addl	#4,%a6		|to offset the sub.l #4,%a7 above so that
|				;%a6 can point correctly to the stack frame 
|				;before branching to mem_read
	bsrl	mem_read
	subl	#4,%a6
	movel	%a6@(L_SCR1),%d0		|%d0 contains the fline and command word
	bfextu	%d0{#4:#3},%d1		|extract coprocessor id
	cmpib	#1,%d1		|check if cpid=1
	bne	not_mvcr		|exit if not
	bfextu	%d0{#16:#6},%d1
	cmpib	#0x17,%d1		|check if it is an FMOVECR encoding
	bne	not_mvcr		|
|				;if an FMOVECR instruction, fix stack
|				;and go to FPSP_UNIMP
fix_stack:
	cmpib	#VER_40,%a7@		|test for orig unimp frame
	bnes	ck_rev
	subl	#UNIMP_40_SIZE-4,%a7		|emulate an orig fsave
	moveb	#VER_40,%a7@
	moveb	#UNIMP_40_SIZE-4,%a7@(1)
	clrw	%a7@(2)
	bras	fix_con
ck_rev:
	cmpib	#VER_41,%a7@		|test for rev unimp frame
	bnel	fpsp_fmt_error		|if not 0x40 or 0x41, exit with error
	subl	#UNIMP_41_SIZE-4,%a7		|emulate a rev fsave
	moveb	#VER_41,%a7@
	moveb	#UNIMP_41_SIZE-4,%a7@(1)
	clrw	%a7@(2)
fix_con:
	movew	%a6@(EXC_SR+4),%a6@(EXC_SR)		|move stacked sr to new position
	movel	%a6@(EXC_PC+4),%a6@(EXC_PC)		|move stacked %pc to new position
	fmovel	%a6@(EXC_PC),%FPI		|point %FPI to fline inst
	movel	#4,%d1
	addl	%d1,%a6@(EXC_PC)		|increment stacked %pc value to next inst
	movew	#0x202c,%a6@(EXC_VEC)		|reformat vector to unimp
	clrl	%a6@(EXC_EA)		|clear the EXC_EA field
	movew	%d0,%a6@(CMDREG1B)		|move the lower word into CMDREG1B
	clrl	%a6@(E_BYTE)
	bset	#UFLAG,%a6@(T_BYTE)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1		|restore data registers
	bral	uni_2

not_mvcr:
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1		|restore data registers
	frestore	%a7@+
	unlk	%a6
	addl	#4,%a7
	bral	real_fline

|	end
