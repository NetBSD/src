|	$NetBSD: netbsd.sa,v 1.5 2001/10/02 06:34:52 chs Exp $

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
|	skeleton.sa 3.2 4/26/91
|
|	This file contains code that is system dependent and will
|	need to be modified to install the FPSP.
|
|	Each entry point for exception 'xxxx' begins with a 'jmp fpsp_xxxx'.
|	Put any target system specific handling that must be done immediately
|	before the jump instruction.  If there no handling necessary, then
|	the 'fpsp_xxxx' handler entry point should be placed in the exception
|	table so that the 'jmp' can be eliminated. If the FPSP determines that the
|	exception is one that must be reported then there will be a
|	return from the package by a 'jmp real_xxxx'.  At that point
|	the machine state will be identical to the state before
|	the FPSP was entered.  In particular, whatever condition
|	that caused the exception will still be pending when the FPSP
|	package returns.  Thus, there will be system specific code
|	to handle the exception.
|
|	If the exception was completely handled by the package, then
|	the return will be via a 'jmp fpsp_done'.  Unless there is 
|	OS specific work to be done (such as handling a context switch or
|	interrupt) the user program can be resumed via 'rte'.
|
|	In the following skeleton code, some typical 'real_xxxx' handling
|	code is shown.  This code may need to be moved to an appropriate
|	place in the target system, or rewritten.
|	

|SKELETON	IDNT    2,1 Motorola 040 Floating Point Software Package

	.data
|
|	The following counters are used for standalone testing
|

	.text

	.include "fpsp.defs"

|
| XXX Note, this is NOT valid Motorola syntax, but what else can we do?
|
#include <machine/asm.h>

|	xref	b1238_fix
|	xref	_C_LABEL(mmutype)

|
|	Divide by Zero exception
|
|	All dz exceptions are 'real', hence no fpsp_dz entry point.
|
	.global	dz
	.global	real_dz
dz:
	cmpl	#-2,_C_LABEL(mmutype)
	bnel	_C_LABEL(fpfault)
real_dz:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E1,%a6@(E_BYTE)
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)

|
|	Inexact exception
|
|	All inexact exceptions are real, but the 'real' handler
|	will probably want to clear the pending exception.
|	The provided code will clear the E3 exception (if pending), 
|	otherwise clear the E1 exception.  The frestore is not really
|	necessary for E1 exceptions.
|
| Code following the 'inex' label is to handle bug #1232.  In this
| bug, if an E1 snan, ovfl, or unfl occurred, and the process was
| swapped out before taking the exception, the exception taken on
| return was inex, rather than the correct exception.  The snan, ovfl,
| and unfl exception to be taken must not have been enabled.  The
| fix is to check for E1, and the existence of one of snan, ovfl,
| or unfl bits set in the %fpsr.  If any of these are set, branch
| to the appropriate  handler for the exception in the %fpsr.  Note
| that this fix is only for d43b parts, and is skipped if the
| version number is not 0x40.
| 
|
	.global	real_inex
	.global	inex
inex:
	cmpl	#-2,_C_LABEL(mmutype)
	bnel	_C_LABEL(fpfault)
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	cmpib	#VER_40,%sp@		|test version number
	bnes	not_fmt40
	fmovel	%fpsr,%sp@-
	btst	#E1,%a6@(E_BYTE)		|test for E1 set
	beqs	not_b1232
	btst	#snan_bit,%sp@(2)		|test for snan
	beq	inex_ckofl
	addql	#4,%sp
	frestore	%sp@+
	unlk	%a6
	bra	snan
inex_ckofl:
	btst	#ovfl_bit,%sp@(2)		|test for ovfl
	beq	inex_ckufl		|
	addql	#4,%sp
	frestore	%sp@+
	unlk	%a6
	bra	ovfl
inex_ckufl:
	btst	#unfl_bit,%sp@(2)		|test for unfl
	beq	not_b1232
	addql	#4,%sp
	frestore	%sp@+
	unlk	%a6
	bra	unfl

|
| We do not have the bug 1232 case.  Clean up the stack and call
| real_inex.
|
not_b1232:
	addql	#4,%sp
	frestore	%sp@+
	unlk	%a6

real_inex:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
not_fmt40:
	bclr	#E3,%a6@(E_BYTE)		|clear and test E3 flag
	beqs	inex_cke1
|
| Clear dirty bit on dest resister in the frame before branching
| to b1238_fix.
|
	moveml	%d0/%d1,%a6@(USER_DA)
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|get dest reg no
	bclr	%d0,%a6@(FPR_DIRTY_BITS)		|clr dest dirty bit
	bsrl	b1238_fix		|test for bug1238 case
	moveml	%a6@(USER_DA),%d0/%d1
	bras	inex_done
inex_cke1:
	bclr	#E1,%a6@(E_BYTE)
inex_done:
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)
	
|
|	Overflow exception
|
|	xref	fpsp_ovfl
	.global	real_ovfl
	.global	ovfl
ovfl:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_ovfl
	jmp	_C_LABEL(fpfault)
real_ovfl:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E3,%a6@(E_BYTE)		|clear and test E3 flag
	bnes	ovfl_done
	bclr	#E1,%a6@(E_BYTE)
ovfl_done:
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)
	
|
|	Underflow exception
|
|	xref	fpsp_unfl
	.global	real_unfl
	.global	unfl
unfl:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_unfl
	jmp	_C_LABEL(fpfault)
real_unfl:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E3,%a6@(E_BYTE)		|clear and test E3 flag
	bnes	unfl_done
	bclr	#E1,%a6@(E_BYTE)
unfl_done:
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)
	
|
|	Signalling NAN exception
|
|	xref	fpsp_snan
	.global	real_snan
	.global	snan
snan:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_snan
	jmp	_C_LABEL(fpfault)
real_snan:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E1,%a6@(E_BYTE)		|snan is always an E1 exception
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)
	
|
|	Operand Error exception
|
|	xref	fpsp_operr
	.global	real_operr
	.global	operr
operr:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_operr
	jmp	_C_LABEL(fpfault)
real_operr:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E1,%a6@(E_BYTE)		|operr is always an E1 exception
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)
	
|
|	BSUN exception
|
|	This sample handler simply clears the nan bit in the %FPSR.
|
|	xref	fpsp_bsun
	.global	real_bsun
	.global	bsun
bsun:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_bsun
	jmp	_C_LABEL(fpfault)
real_bsun:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E1,%a6@(E_BYTE)		|bsun is always an E1 exception
	fmovel	%FPSR,%sp@-
	bclr	#nan_bit,%sp@
	fmovel	%sp@+,%FPSR
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)

|
|	F-line exception
|
|	A 'real' F-line exception is one that the FPSP isn't supposed to 
|	handle. E.g. an instruction with a co-processor ID that is not 1.
|
|
|	xref	fpsp_fline
	.global	real_fline
	.global	fline
fline:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_fline
	jmp	_C_LABEL(fpfault)
real_fline:
	jmp	_C_LABEL(fpfault)

|
|	Unsupported data type exception
|
|	xref	fpsp_unsupp
	.global	real_unsupp
	.global	unsupp
unsupp:
	cmpl	#-2,_C_LABEL(mmutype)
	beql	fpsp_unsupp
	jmp	_C_LABEL(fpfault)
real_unsupp:
	link	%a6,#-LOCAL_SIZE
	fsave	%sp@-
	bclr	#E1,%a6@(E_BYTE)		|unsupp is always an E1 exception
	frestore	%sp@+
	unlk	%a6
	jmp	_C_LABEL(fpfault)

|
|	Trace exception
|
	.global	real_trace
real_trace:
	rte

|
|	fpsp_fmt_error --- exit point for frame format error
|
|	The fpu stack frame does not match the frames existing
|	or planned at the time of this writing.  The fpsp is
|	unable to handle frame sizes not in the following
|	version:size pairs:
|
|	{4060, 4160} - busy frame
|	{4028, 4130} - unimp frame
|	{4000, 4100} - idle frame
|
|	This entry point simply holds an f-line illegal value.  
|	Replace this with a call to your kernel panic code or
|	code to handle future revisions of the fpu.
|
	.global	fpsp_fmt_error
fpsp_fmt_error:
	pea	1f
	jsr	_C_LABEL(panic)
	.long	0xf27f0000		|f-line illegal
1:
	.asciz	"bad		|floating point stack frame"
	.even

|
|	fpsp_done --- FPSP exit point
|
|	The exception has been handled by the package and we are ready
|	to return to user mode, but there may be OS specific code
|	to execute before we do.  If there is, do it now.
|
|
|	xref	_ASM_LABEL(rei)
	.global	fpsp_done
fpsp_done:
	jmp	_ASM_LABEL(rei)

|
|	mem_write --- write to user or supervisor address space
|
| Writes to memory while in supervisor mode.  copyout accomplishes
| this via a 'moves' instruction.  copyout is a UNIX SVR3 (and later) function.
| If you don't have copyout, use the local copy of the function below.
|
|	%a0 - supervisor source address
|	%a1 - user destination address
|	%d0 - number of bytes to write (maximum count is 12)
|
| The supervisor source address is guaranteed to point into the supervisor
| stack.  The result is that a UNIX
| process is allowed to sleep as a consequence of a page fault during
| copyout.  The probability of a page fault is exceedingly small because
| the 68040 always reads the destination address and thus the page
| faults should have already been handled.
|
| If the EXC_SR shows that the exception was from supervisor space,
| then just do a dumb (and slow) memory move.  In a UNIX environment
| there shouldn't be any supervisor mode floating point exceptions.
|
	.global	mem_write
mem_write:
	btst	#5,%a6@(EXC_SR)		|check for supervisor state
	beqs	user_write
super_write:
	moveb	%a0@+,%a1@+
	subql	#1,%d0
	bnes	super_write
	rts
user_write:
	movel	%d1,%sp@-		|preserve %d1 just in case
	movel	%d0,%sp@-
	movel	%a1,%sp@-
	movel	%a0,%sp@-
	jsr	_C_LABEL(copyout)
	addl	#12,%sp
	movel	%sp@+,%d1
	rts

|
|	mem_read --- read from user or supervisor address space
|
| Reads from memory while in supervisor mode.  copyin accomplishes
| this via a 'moves' instruction.  copyin is a UNIX SVR3 (and later) function.
| If you don't have copyin, use the local copy of the function below.
|
| The FPSP calls mem_read to read the original F-line instruction in order
| to extract the data register number when the 'Dn' addressing mode is
| used.
|
|Input:
|	%a0 - user source address
|	%a1 - supervisor destination address
|	%d0 - number of bytes to read (maximum count is 12)
|
| Like mem_write, mem_read always reads with a supervisor 
| destination address on the supervisor stack.  Also like mem_write,
| the EXC_SR is checked and a simple memory copy is done if reading
| from supervisor space is indicated.
|
	.global	mem_read
mem_read:
	btst	#5,%a6@(EXC_SR)		|check for supervisor state
	beqs	user_read
super_read:
	moveb	%a0@+,%a1@+
	subql	#1,%d0
	bnes	super_read
	rts
user_read:
	movel	%d1,%sp@-		|preserve %d1 just in case
	movel	%d0,%sp@-
	movel	%a1,%sp@-
	movel	%a0,%sp@-
	jsr	_C_LABEL(copyin)
	addl	#12,%sp
	movel	%sp@+,%d1
	rts

|	end
