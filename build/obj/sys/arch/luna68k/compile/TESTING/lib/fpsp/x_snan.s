|	$NetBSD: x_snan.sa,v 1.4 2001/09/16 16:34:32 wiz Exp $

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
|	x_snan.sa 3.3 7/1/91
|
| fpsp_snan --- FPSP handler for signalling NAN exception
|
| SNAN for float -> integer conversions (integer conversion of
| an SNAN) is a non-maskable run-time exception.
|
| For trap disabled the 040 does the following:
| If the dest data format is s, d, or x, then the SNAN bit in the NAN
| is set to one and the resulting non-signaling NAN (truncated if
| necessary) is transferred to the dest.  If the dest format is b, w,
| or l, then garbage is written to the dest (actually the upper 32 bits
| of the mantissa are sent to the integer unit).
|
| For trap enabled the 040 does the following:
| If the inst is move_out, then the results are the same as for trap 
| disabled with the exception posted.  If the instruction is not move_
| out, the dest. is not modified, and the exception is posted.
|

|X_SNAN	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	get_fline
|	xref	mem_write
|	xref	real_snan
|	xref	real_inex
|	xref	fpsp_done
|	xref	reg_dest

	.global	fpsp_snan
fpsp_snan:
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	fmovemx	%fp0-%fp3,%a6@(USER_FP0)
	fmoveml	%fpcr/%fpsr/%fpi,%a6@(USER_FPCR)

|
| Check if trap enabled
|
	btst	#snan_bit,%a6@(FPCR_ENABLE)
	bnes	ena		|If enabled, then branch

	bsrl	move_out		|else SNAN disabled
|
| It is possible to have an inex1 exception with the
| snan.  If the inex enable bit is set in the %FPCR, and either
| inex2 or inex1 occurred, we must clean up and branch to the
| real inex handler.
|
ck_inex:
	moveb	%a6@(FPCR_ENABLE),%d0
	andb	%a6@(FPSR_EXCEPT),%d0
	andib	#0x3,%d0
	beq	end_snan
|
| Inexact enabled and reported, and we must take an inexact exception.
|
take_inex:
	moveb	#INEX_VEC,%a6@(EXC_VEC+1)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_inex
|
| SNAN is enabled.  Check if inst is move_out.
| Make any corrections to the 040 output as necessary.
|
ena:
	btst	#5,%a6@(CMDREG1B)		|if set, inst is move out
	beq	not_out

	bsrl	move_out

report_snan:
	moveb	%a7@,%a6@(VER_TMP)
	cmpib	#VER_40,%a7@		|test for orig unimp frame
	bnes	ck_rev
	moveql	#13,%d0		|need to zero 14 lwords
	bras	rep_con
ck_rev:
	moveql	#11,%d0		|need to zero 12 lwords
rep_con:
	clrl	%a7@
loop1:
	clrl	%a7@-		|clear and dec %a7
	dbra	%d0,loop1
	moveb	%a6@(VER_TMP),%a7@		|format a busy frame
	moveb	#BUSY_SIZE-4,%a7@(1)
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_snan
|
| Exit snan handler by expanding the unimp frame into a busy frame
|
end_snan:
	bclr	#E1,%a6@(E_BYTE)

	moveb	%a7@,%a6@(VER_TMP)
	cmpib	#VER_40,%a7@		|test for orig unimp frame
	bnes	ck_rev2
	moveql	#13,%d0		|need to zero 14 lwords
	bras	rep_con2
ck_rev2:
	moveql	#11,%d0		|need to zero 12 lwords
rep_con2:
	clrl	%a7@
loop2:
	clrl	%a7@-		|clear and dec %a7
	dbra	%d0,loop2
	moveb	%a6@(VER_TMP),%a7@		|format a busy frame
	moveb	#BUSY_SIZE-4,%a7@(1)		|write busy size
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	fpsp_done

|
| Move_out 
|
move_out:
	movel	%a6@(EXC_EA),%a0		|get <ea> from exc frame

	bfextu	%a6@(CMDREG1B){#3:#3},%d0		|move rx field to %d0{#2:#0}
	tstl	%d0		|check for long
	beqs	sto_long		|branch if move_out long
	
	cmpil	#4,%d0		|check for word
	beqs	sto_word		|branch if move_out word
	
	cmpil	#6,%d0		|check for byte
	beqs	sto_byte		|branch if move_out byte
	
|
| Not byte, word or long
|
	rts
|	
| Get the 32 most significant bits of etemp mantissa
|
sto_long:
	movel	%a6@(ETEMP_HI),%d1
	movel	#4,%d0		|load byte count
|
| Set signalling nan bit
|
	bset	#30,%d1		|
|
| Store to the users destination address
|
	tstl	%a0		|check if <ea> is 0
	beqs	wrt_dn		|destination is a data register
	
	movel	%d1,%a7@-		|move the snan onto the stack
	movel	%a0,%a1		|load dest addr into %a1
	movel	%a7,%a0		|load src addr of snan into %a0
	bsrl	mem_write		|write snan to user memory
	movel	%a7@+,%d1		|clear off stack
	rts
|
| Get the 16 most significant bits of etemp mantissa
|
sto_word:
	movel	%a6@(ETEMP_HI),%d1
	movel	#2,%d0		|load byte count
|
| Set signalling nan bit
|
	bset	#30,%d1		|
|
| Store to the users destination address
|
	tstl	%a0		|check if <ea> is 0
	beqs	wrt_dn		|destination is a data register

	movel	%d1,%a7@-		|move the snan onto the stack
	movel	%a0,%a1		|load dest addr into %a1
	movel	%a7,%a0		|point to low word
	bsrl	mem_write		|write snan to user memory
	movel	%a7@+,%d1		|clear off stack
	rts
|
| Get the 8 most significant bits of etemp mantissa
|
sto_byte:
	movel	%a6@(ETEMP_HI),%d1
	movel	#1,%d0		|load byte count
|
| Set signalling nan bit
|
	bset	#30,%d1		|
|
| Store to the users destination address
|
	tstl	%a0		|check if <ea> is 0
	beqs	wrt_dn		|destination is a data register
	movel	%d1,%a7@-		|move the snan onto the stack
	movel	%a0,%a1		|load dest addr into %a1
	movel	%a7,%a0		|point to source byte
	bsrl	mem_write		|write snan to user memory
	movel	%a7@+,%d1		|clear off stack
	rts

|
|	wrt_dn --- write to a data register
|
|	We get here with %D1 containing the data to write and %D0 the
|	number of bytes to write: 1=byte,2=word,4=long.
|
wrt_dn:
	movel	%d1,%a6@(L_SCR1)		|data
	movel	%d0,%a7@-		|size
	bsrl	get_fline		|returns fline word in %d0
	movel	%d0,%d1
	andil	#0x7,%d1		|%d1 now holds register number
	movel	%sp@+,%d0		|get original size
	cmpil	#4,%d0
	beqs	wrt_long
	cmpil	#2,%d0
	bnes	wrt_byte
wrt_word:
	orl	#0x8,%d1
	bral	reg_dest
wrt_long:
	orl	#0x10,%d1
	bral	reg_dest
wrt_byte:
	bral	reg_dest
|
| Check if it is a src nan or dst nan
|
not_out:
	movel	%a6@(DTAG),%d0		|
	bfextu	%d0{#0:#3},%d0		|isolate dtag in lsbs

	cmpib	#3,%d0		|check for nan in destination
	bnes	issrc		|destination nan has priority
dst_nan:
	btst	#6,%a6@(FPTEMP_HI)		|check if dest nan is an snan
	bnes	issrc		|no, so check source for snan
	movew	%a6@(FPTEMP_EX),%d0
	bras	cont
issrc:
	movew	%a6@(ETEMP_EX),%d0
cont:
	btst	#15,%d0		|test for sign of snan
	beqs	clr_neg
	bset	#neg_bit,%a6@(FPSR_CC)
	bra	report_snan
clr_neg:
	bclr	#neg_bit,%a6@(FPSR_CC)
	bra	report_snan

|	end
