|	$NetBSD: x_operr.sa,v 1.5 2001/09/16 16:34:32 wiz Exp $

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
|	x_operr.sa 3.5 7/1/91
|
|	fpsp_operr --- FPSP handler for operand error exception
|
|	See 68040 User's Manual pp. 9-44f
|
| Note 1: For trap disabled 040 does the following:
| If the dest is a fp reg, then an extended precision non_signaling
| NAN is stored in the dest reg.  If the dest format is b, w, or l and
| the source op is a NAN, then garbage is stored as the result (actually
| the upper 32 bits of the mantissa are sent to the integer unit). If
| the dest format is integer (b, w, l) and the operr is caused by
| integer overflow, or the source op is inf, then the result stored is
| garbage.
| There are three cases in which operr is incorrectly signaled on the 
| 040.  This occurs for move_out of format b, w, or l for the largest 
| negative integer (-2^7 for b, -2^15 for w, -2^31 for l).
|
|	  On opclass = 011 fmove.(b,w,l) that causes a conversion
|	  overflow -> OPERR, the exponent in wbte (and fpte) is:
|		byte    56 - (62 - exp)
|		word    48 - (62 - exp)
|		long    32 - (62 - exp)
|
|			where exp = (true exp) - 1
|
|  So, wbtemp and fptemp will contain the following on erroneoulsy
|	  signalled operr:
|			fpts = 1
|			fpte = 0x4000  (15 bit externally)
|		byte	fptm = 0xffffffff ffffff80
|		word	fptm = 0xffffffff ffff8000
|		long	fptm = 0xffffffff 80000000
|
| Note 2: For trap enabled 040 does the following:
| If the inst is move_out, then same as Note 1.
| If the inst is not move_out, the dest is not modified.
| The exceptional operand is not defined for integer overflow 
| during a move_out.
|

|X_OPERR	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	mem_write
|	xref	real_operr
|	xref	real_inex
|	xref	get_fline
|	xref	fpsp_done
|	xref	reg_dest

	.global	fpsp_operr
fpsp_operr:
|
	link	%a6,#-LOCAL_SIZE
	fsave	%a7@-
	moveml	%d0-%d1/%a0-%a1,%a6@(USER_DA)
	fmovemx	%fp0-%fp3,%a6@(USER_FP0)
	fmoveml	%fpcr/%fpsr/%fpi,%a6@(USER_FPCR)

|
| Check if this is an opclass 3 instruction.
|  If so, fall through, else branch to operr_end
|
	btst	#TFLAG,%a6@(T_BYTE)
	beqs	operr_end

|
| If the destination size is B,W,or L, the operr must be 
| handled here.
|
	movel	%a6@(CMDREG1B),%d0
	bfextu	%d0{#3:#3},%d0		|0=long, 4=word, 6=byte
	tstb	%d0		|determine size; check long
	beq	operr_long
	cmpib	#4,%d0		|check word
	beq	operr_word
	cmpib	#6,%d0		|check byte
	beq	operr_byte

|
| The size is not B,W,or L, so the operr is handled by the 
| kernel handler.  Set the operr bits and clean up, leaving
| only the integer exception frame on the stack, and the 
| fpu in the original exceptional state.
|
operr_end:
	bset	#operr_bit,%a6@(FPSR_EXCEPT)
	bset	#aiop_bit,%a6@(FPSR_AEXCEPT)

	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_operr

operr_long:
	moveql	#4,%d1		|write size to %d1
	moveb	%a6@(STAG),%d0		|test stag for nan
	andib	#0xe0,%d0		|clr all but tag
	cmpib	#0x60,%d0		|check for nan
	beq	operr_nan		|
	cmpil	#0x80000000,%a6@(FPTEMP_LO)		|test if ls lword is special
	bnes	chklerr		|if not equal, check for incorrect operr
	bsr	check_upper		|check if exp and ms mant are special
	tstl	%d0
	bnes	chklerr		|if %d0 is true, check for incorrect operr
	movel	#0x80000000,%d0		|store special case result
	bsr	operr_store
	bra	not_enabled		|clean and exit
|
|	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
|
chklerr:
	movew	%a6@(FPTEMP_EX),%d0
	andw	#0x7FFF,%d0		|ignore sign bit
	cmpw	#0x3FFE,%d0		|this is the only possible exponent value
	bnes	chklerr2
fixlong:
	movel	%a6@(FPTEMP_LO),%d0
	bsr	operr_store
	bra	not_enabled
chklerr2:
	movew	%a6@(FPTEMP_EX),%d0
	andw	#0x7FFF,%d0		|ignore sign bit
	cmpw	#0x4000,%d0
	bcc	store_max		|exponent out of range

	movel	%a6@(FPTEMP_LO),%d0
	andl	#0x7FFF0000,%d0		|look for all 1's on bits 30-16
	cmpl	#0x7FFF0000,%d0
	beqs	fixlong

	tstl	%a6@(FPTEMP_LO)
	bpls	chklepos
	cmpl	#0xFFFFFFFF,%a6@(FPTEMP_HI)
	beqs	fixlong
	bra	store_max
chklepos:
	tstl	%a6@(FPTEMP_HI)
	beqs	fixlong
	bra	store_max

operr_word:
	moveql	#2,%d1		|write size to %d1
	moveb	%a6@(STAG),%d0		|test stag for nan
	andib	#0xe0,%d0		|clr all but tag
	cmpib	#0x60,%d0		|check for nan
	beq	operr_nan		|
	cmpil	#0xffff8000,%a6@(FPTEMP_LO)		|test if ls lword is special
	bnes	chkwerr		|if not equal, check for incorrect operr
	bsr	check_upper		|check if exp and ms mant are special
	tstl	%d0
	bnes	chkwerr		|if %d0 is true, check for incorrect operr
	movel	#0x80000000,%d0		|store special case result
	bsr	operr_store
	bra	not_enabled		|clean and exit
|
|	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
|
chkwerr:
	movew	%a6@(FPTEMP_EX),%d0
	andw	#0x7FFF,%d0		|ignore sign bit
	cmpw	#0x3FFE,%d0		|this is the only possible exponent value
	bnes	store_max
	movel	%a6@(FPTEMP_LO),%d0
	swap	%d0
	bsr	operr_store
	bra	not_enabled

operr_byte:
	moveql	#1,%d1		|write size to %d1
	moveb	%a6@(STAG),%d0		|test stag for nan
	andib	#0xe0,%d0		|clr all but tag
	cmpib	#0x60,%d0		|check for nan
	beqs	operr_nan		|
	cmpil	#0xffffff80,%a6@(FPTEMP_LO)		|test if ls lword is special
	bnes	chkberr		|if not equal, check for incorrect operr
	bsr	check_upper		|check if exp and ms mant are special
	tstl	%d0
	bnes	chkberr		|if %d0 is true, check for incorrect operr
	movel	#0x80000000,%d0		|store special case result
	bsr	operr_store
	bra	not_enabled		|clean and exit
|
|	CHECK FOR INCORRECTLY GENERATED OPERR EXCEPTION HERE
|
chkberr:
	movew	%a6@(FPTEMP_EX),%d0
	andw	#0x7FFF,%d0		|ignore sign bit
	cmpw	#0x3FFE,%d0		|this is the only possible exponent value
	bnes	store_max
	movel	%a6@(FPTEMP_LO),%d0
	asll	#8,%d0
	swap	%d0
	bsr	operr_store
	bra	not_enabled

|
| This operr condition is not of the special case.  Set operr
| and aiop and write the portion of the nan to memory for the
| given size.
|
operr_nan:
	orl	#opaop_mask,%a6@(USER_FPSR)		|set operr & aiop

	movel	%a6@(ETEMP_HI),%d0		|output will be from upper 32 bits
	bsr	operr_store
	bra	end_operr
|
| Store_max loads the max pos or negative for the size, sets
| the operr and aiop bits, and clears inex and ainex, incorrectly
| set by the 040.
|
store_max:
	orl	#opaop_mask,%a6@(USER_FPSR)		|set operr & aiop
	bclr	#inex2_bit,%a6@(FPSR_EXCEPT)
	bclr	#ainex_bit,%a6@(FPSR_AEXCEPT)
	fmovel	#0,%FPSR
	
	tstw	%a6@(FPTEMP_EX)		|check sign
	blts	load_neg
	movel	#0x7fffffff,%d0
	bsr	operr_store
	bra	end_operr
load_neg:
	movel	#0x80000000,%d0
	bsr	operr_store
	bra	end_operr

|
| This routine stores the data in %d0, for the given size in %d1,
| to memory or data register as required.  A read of the fline
| is required to determine the destination.
|
operr_store:
	movel	%d0,%a6@(L_SCR1)		|move write data to L_SCR1
	movel	%d1,%a7@-		|save register size
	bsrl	get_fline		|fline returned in %d0
	movel	%a7@+,%d1
	bftst	%d0{#26:#3}		|if mode is zero, dest is Dn
	bnes	dest_mem
|
| Destination is Dn.  Get register number from %d0. Data is on
| the stack at %a7@. %D1 has size: 1=byte,2=word,4=long/single
|
	andil	#7,%d0		|isolate register number
	cmpil	#4,%d1
	beqs	op_long		|the most frequent case
	cmpil	#2,%d1
	bnes	op_con
	orl	#8,%d0
	bras	op_con
op_long:
	orl	#0x10,%d0
op_con:
	movel	%d0,%d1		|format size:reg for reg_dest
	bral	reg_dest		|call to reg_dest returns to caller
|				;of operr_store
|
| Destination is memory.  Get <ea> from integer exception frame
| and call mem_write.
|
dest_mem:
	lea	%a6@(L_SCR1),%a0		|put ptr to write data in %a0
	movel	%a6@(EXC_EA),%a1		|put user destination address in %a1
	movel	%d1,%d0		|put size in %d0
	bsrl	mem_write
	rts
|
| Check the exponent for 0xc000 and the upper 32 bits of the 
| mantissa for 0xffffffff.  If both are true, return %d0 clr
| and store the lower n bits of the least lword of FPTEMP
| to %d0 for write out.  If not, it is a real operr, and set %d0.
|
check_upper:
	cmpil	#0xffffffff,%a6@(FPTEMP_HI)		|check if first byte is all 1's
	bnes	true_operr		|if not all 1's then was true operr
	cmpiw	#0xc000,%a6@(FPTEMP_EX)		|check if incorrectly signalled
	beqs	not_true_operr		|branch if not true operr
	cmpiw	#0xbfff,%a6@(FPTEMP_EX)		|check if incorrectly signalled
	beqs	not_true_operr		|branch if not true operr
true_operr:
	movel	#1,%d0		|signal real operr
	rts
not_true_operr:
	clrl	%d0		|signal no real operr
	rts

|
| End_operr tests for operr enabled.  If not, it cleans up the stack
| and does an rte.  If enabled, it cleans up the stack and branches
| to the kernel operr handler with only the integer exception
| frame on the stack and the fpu in the original exceptional state
| with correct data written to the destination.
|
end_operr:
	btst	#operr_bit,%a6@(FPCR_ENABLE)
	beqs	not_enabled
enabled:
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_operr

not_enabled:
|
| It is possible to have either inex2 or inex1 exceptions with the
| operr.  If the inex enable bit is set in the %FPCR, and either
| inex2 or inex1 occurred, we must clean up and branch to the
| real inex handler.
|
ck_inex:
	moveb	%a6@(FPCR_ENABLE),%d0
	andb	%a6@(FPSR_EXCEPT),%d0
	andib	#0x3,%d0
	beq	operr_exit
|
| Inexact enabled and reported, and we must take an inexact exception.
|
take_inex:
	moveb	#INEX_VEC,%a6@(EXC_VEC+1)
	movel	%a6@(USER_FPSR),%a6@(FPSR_SHADOW)
	orl	#sx_mask,%a6@(E_BYTE)
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	frestore	%a7@+
	unlk	%a6
	bral	real_inex
|
| Since operr is only an E1 exception, there is no need to frestore
| any state back to the fpu.
|
operr_exit:
	moveml	%a6@(USER_DA),%d0-%d1/%a0-%a1
	fmovemx	%a6@(USER_FP0),%fp0-%fp3
	fmoveml	%a6@(USER_FPCR),%fpcr/%fpsr/%fpi
	unlk	%a6
	bral	fpsp_done

|	end
