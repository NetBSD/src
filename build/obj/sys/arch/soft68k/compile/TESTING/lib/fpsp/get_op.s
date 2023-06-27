|	$NetBSD: get_op.sa,v 1.5 2022/11/02 20:38:22 andvar Exp $

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
|	get_op.sa 3.6 5/19/92
|
|	get_op.sa 3.5 4/26/91
|
|  Description: This routine is called by the unsupported format/data
| type exception handler ('unsupp' - vector 55) and the unimplemented
| instruction exception handler ('unimp' - vector 11).  'get_op'
| determines the opclass (0, 2, or 3) and branches to the
| opclass handler routine.  See 68881/2 User's Manual table 4-11
| for a description of the opclasses.
|
| For UNSUPPORTED data/format (exception vector 55) and for
| UNIMPLEMENTED instructions (exception vector 11) the following
| applies:
|
| - For unnormormalized numbers (opclass 0, 2, or 3) the
| number(s) is normalized and the operand type tag is updated.
|		
| - For a packed number (opclass 2) the number is unpacked and the
| operand type tag is updated.
|
| - For denormalized numbers (opclass 0 or 2) the number(s) is not
| changed but passed to the next module.  The next module for
| unimp is do_func, the next module for unsupp is res_func.
|
| For UNSUPPORTED data/format (exception vector 55) only the
| following applies:
|
| - If there is a move out with a packed number (opclass 3) the
| number is packed and written to user memory.  For the other
| opclasses the number(s) are written back to the fsave stack
| and the instruction is then restored back into the '040.  The
| '040 is then able to complete the instruction.
|
| For example:
| fadd.x fpm,fpn where the fpm contains an unnormalized number.
| The '040 takes an unsupported data trap and gets to this
| routine.  The number is normalized, put back on the stack and
| then an frestore is done to restore the instruction back into
| the '040.  The '040 then re-executes the fadd.x fpm,fpn with
| a normalized number in the source and the instruction is
| successful.
|		
| Next consider if in the process of normalizing the un-
| normalized number it becomes a denormalized number.  The
| routine which converts the unnorm to a norm (called mk_norm)
| detects this and tags the number as a denorm.  The routine
| res_func sees the denorm tag and converts the denorm to a
| norm.  The instruction is then restored back into the '040
| which re_executess the instruction.
|

|GET_OP    IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

	.global	PIRN,PIRZRM,PIRP
	.global	SMALRN,SMALRZRM,SMALRP
	.global	BIGRN,BIGRZRM,BIGRP

PIRN:
	.long	0x40000000,0xc90fdaa2,0x2168c235		|pi
PIRZRM:
	.long	0x40000000,0xc90fdaa2,0x2168c234		|pi
PIRP:
	.long	0x40000000,0xc90fdaa2,0x2168c235		|pi

|round to nearest
SMALRN:
	.long	0x3ffd0000,0x9a209a84,0xfbcff798		|log10(2)
	.long	0x40000000,0xadf85458,0xa2bb4a9a		|e
	.long	0x3fff0000,0xb8aa3b29,0x5c17f0bc		|log2(e)
	.long	0x3ffd0000,0xde5bd8a9,0x37287195		|log10(e)
	.long	0x00000000,0x00000000,0x00000000		|0.0
| round to zero;round to negative infinity
SMALRZRM:
	.long	0x3ffd0000,0x9a209a84,0xfbcff798		|log10(2)
	.long	0x40000000,0xadf85458,0xa2bb4a9a		|e
	.long	0x3fff0000,0xb8aa3b29,0x5c17f0bb		|log2(e)
	.long	0x3ffd0000,0xde5bd8a9,0x37287195		|log10(e)
	.long	0x00000000,0x00000000,0x00000000		|0.0
| round to positive infinity
SMALRP:
	.long	0x3ffd0000,0x9a209a84,0xfbcff799		|log10(2)
	.long	0x40000000,0xadf85458,0xa2bb4a9b		|e
	.long	0x3fff0000,0xb8aa3b29,0x5c17f0bc		|log2(e)
	.long	0x3ffd0000,0xde5bd8a9,0x37287195		|log10(e)
	.long	0x00000000,0x00000000,0x00000000		|0.0

|round to nearest
BIGRN:
	.long	0x3ffe0000,0xb17217f7,0xd1cf79ac		|ln(2)
	.long	0x40000000,0x935d8ddd,0xaaa8ac17		|ln(10)
	.long	0x3fff0000,0x80000000,0x00000000		|10 ^ 0

	.global	PTENRN
PTENRN:
	.long	0x40020000,0xA0000000,0x00000000		|10 ^ 1
	.long	0x40050000,0xC8000000,0x00000000		|10 ^ 2
	.long	0x400C0000,0x9C400000,0x00000000		|10 ^ 4
	.long	0x40190000,0xBEBC2000,0x00000000		|10 ^ 8
	.long	0x40340000,0x8E1BC9BF,0x04000000		|10 ^ 16
	.long	0x40690000,0x9DC5ADA8,0x2B70B59E		|10 ^ 32
	.long	0x40D30000,0xC2781F49,0xFFCFA6D5		|10 ^ 64
	.long	0x41A80000,0x93BA47C9,0x80E98CE0		|10 ^ 128
	.long	0x43510000,0xAA7EEBFB,0x9DF9DE8E		|10 ^ 256
	.long	0x46A30000,0xE319A0AE,0xA60E91C7		|10 ^ 512
	.long	0x4D480000,0xC9767586,0x81750C17		|10 ^ 1024
	.long	0x5A920000,0x9E8B3B5D,0xC53D5DE5		|10 ^ 2048
	.long	0x75250000,0xC4605202,0x8A20979B		|10 ^ 4096
|round to minus infinity
BIGRZRM:
	.long	0x3ffe0000,0xb17217f7,0xd1cf79ab		|ln(2)
	.long	0x40000000,0x935d8ddd,0xaaa8ac16		|ln(10)
	.long	0x3fff0000,0x80000000,0x00000000		|10 ^ 0

	.global	PTENRM
PTENRM:
	.long	0x40020000,0xA0000000,0x00000000		|10 ^ 1
	.long	0x40050000,0xC8000000,0x00000000		|10 ^ 2
	.long	0x400C0000,0x9C400000,0x00000000		|10 ^ 4
	.long	0x40190000,0xBEBC2000,0x00000000		|10 ^ 8
	.long	0x40340000,0x8E1BC9BF,0x04000000		|10 ^ 16
	.long	0x40690000,0x9DC5ADA8,0x2B70B59D		|10 ^ 32
	.long	0x40D30000,0xC2781F49,0xFFCFA6D5		|10 ^ 64
	.long	0x41A80000,0x93BA47C9,0x80E98CDF		|10 ^ 128
	.long	0x43510000,0xAA7EEBFB,0x9DF9DE8D		|10 ^ 256
	.long	0x46A30000,0xE319A0AE,0xA60E91C6		|10 ^ 512
	.long	0x4D480000,0xC9767586,0x81750C17		|10 ^ 1024
	.long	0x5A920000,0x9E8B3B5D,0xC53D5DE5		|10 ^ 2048
	.long	0x75250000,0xC4605202,0x8A20979A		|10 ^ 4096
|round to positive infinity
BIGRP:
	.long	0x3ffe0000,0xb17217f7,0xd1cf79ac		|ln(2)
	.long	0x40000000,0x935d8ddd,0xaaa8ac17		|ln(10)
	.long	0x3fff0000,0x80000000,0x00000000		|10 ^ 0

	.global	PTENRP
PTENRP:
	.long	0x40020000,0xA0000000,0x00000000		|10 ^ 1
	.long	0x40050000,0xC8000000,0x00000000		|10 ^ 2
	.long	0x400C0000,0x9C400000,0x00000000		|10 ^ 4
	.long	0x40190000,0xBEBC2000,0x00000000		|10 ^ 8
	.long	0x40340000,0x8E1BC9BF,0x04000000		|10 ^ 16
	.long	0x40690000,0x9DC5ADA8,0x2B70B59E		|10 ^ 32
	.long	0x40D30000,0xC2781F49,0xFFCFA6D6		|10 ^ 64
	.long	0x41A80000,0x93BA47C9,0x80E98CE0		|10 ^ 128
	.long	0x43510000,0xAA7EEBFB,0x9DF9DE8E		|10 ^ 256
	.long	0x46A30000,0xE319A0AE,0xA60E91C7		|10 ^ 512
	.long	0x4D480000,0xC9767586,0x81750C18		|10 ^ 1024
	.long	0x5A920000,0x9E8B3B5D,0xC53D5DE6		|10 ^ 2048
	.long	0x75250000,0xC4605202,0x8A20979B		|10 ^ 4096

|	xref	nrm_zero
|	xref	decbin
|	xref	round

	.global	get_op
	.global	uns_getop
	.global	uni_getop
get_op:
	clrb	%a6@(DY_MO_FLG)
	tstb	%a6@(UFLG_TMP)		|test flag for unsupp/unimp state
	beqs	short_uni_getop

uns_getop:
	btst	#direction_bit,%a6@(CMDREG1B)
	bne	opclass3		|branch if a fmove out (any kind)
	btst	#6,%a6@(CMDREG1B)
	beqs	uns_notpacked

	bfextu	%a6@(CMDREG1B){#3:#3},%d0
	cmpb	#3,%d0
	beq	pack_source		|check for a packed src op, branch if so
uns_notpacked:
	bsr	chk_dy_mo		|set the dyadic/monadic flag
	tstb	%a6@(DY_MO_FLG)
	beqs	src_op_ck		|if monadic, go check src op
|				;else, check dst op (fall through)

	btst	#7,%a6@(DTAG)
	beqs	src_op_ck		|if dst op is norm, check src op
	bras	dst_ex_dnrm		|else, handle destination unnorm/dnrm

uni_getop:
short_uni_getop:
	bfextu	%a6@(CMDREG1B){#0:#6},%d0		|get opclass and src fields
	cmpil	#0x17,%d0		|if op class and size fields are 0x17, 
|				;it is FMOVECR; if not, continue
|
| If the instruction is fmovecr, exit get_op.  It is handled
| in do_func and smovecr.sa.
|
	bne	not_fmovecr		|handle fmovecr as an unimplemented inst
	rts

not_fmovecr:
	btst	#E1,%a6@(E_BYTE)		|if set, there is a packed operand
	bne	pack_source		|check for packed src op, branch if so

| The following lines of are coded to optimize on normalized operands
	moveb	%a6@(STAG),%d0
	orb	%a6@(DTAG),%d0		|check if either of STAG/DTAG msb set
	bmis	dest_op_ck		|if so, some op needs to be fixed
	rts

dest_op_ck:
	btst	#7,%a6@(DTAG)		|check for unsupported data types in
	beqs	src_op_ck		|the destination, if not, check src op
	bsr	chk_dy_mo		|set dyadic/monadic flag
	tstb	%a6@(DY_MO_FLG)		|
	beqs	src_op_ck		|if monadic, check src op
|
| At this point, destination has an extended denorm or unnorm.
|
dst_ex_dnrm:
	movew	%a6@(FPTEMP_EX),%d0		|get destination exponent
	andiw	#0x7fff,%d0		|mask sign, check if exp = 0000
	beqs	src_op_ck		|if denorm then check source op.
|				;denorms are taken care of in res_func 
|				;(unsupp) or do_func (unimp)
|				;else unnorm fall through
	lea	%a6@(FPTEMP),%a0		|point %a0 to dop - used in mk_norm
	bsr	mk_norm		|go normalize - mk_norm returns:
|				;L_SCR1{#7:#5} = operand tag 
|				;	(000 = norm, 100 = denorm)
|				;L_SCR1{4} = fpte15 or ete15 
|				;	0 = exp >  0x3fff
|				;	1 = exp <= 0x3fff
|				;and puts the normalized num back 
|				;on the fsave stack
|
	moveb	%a6@(L_SCR1),%a6@(DTAG)		|write the new tag & fpte15 
|				;to the fsave stack and fall 
|				;through to check source operand
|
src_op_ck:
	btst	#7,%a6@(STAG)
	beq	end_getop		|check for unsupported data types on the
|				;source operand
	btst	#5,%a6@(STAG)
	bnes	src_sd_dnrm		|if bit 5 set, handle sgl/dbl denorms
|
| At this point only unnorms or extended denorms are possible.
|
src_ex_dnrm:
	movew	%a6@(ETEMP_EX),%d0		|get source exponent
	andiw	#0x7fff,%d0		|mask sign, check if exp = 0000
	beq	end_getop		|if denorm then exit, denorms are 
|				;handled in do_func
	lea	%a6@(ETEMP),%a0		|point %a0 to sop - used in mk_norm
	bsr	mk_norm		|go normalize - mk_norm returns:
|				;L_SCR1{#7:#5} = operand tag 
|				;	(000 = norm, 100 = denorm)
|				;L_SCR1{4} = fpte15 or ete15 
|				;	0 = exp >  0x3fff
|				;	1 = exp <= 0x3fff
|				;and puts the normalized num back 
|				;on the fsave stack
|
	moveb	%a6@(L_SCR1),%a6@(STAG)		|write the new tag & ete15 
	rts	|end_getop

|
| At this point, only single or double denorms are possible.
| If the inst is not fmove, normalize the source.  If it is,
| do nothing to the input.
|
src_sd_dnrm:
	btst	#4,%a6@(CMDREG1B)		|differentiate between sgl/dbl denorm
	bnes	is_double
is_single:
	movew	#0x3f81,%d1		|write bias for sgl denorm
	bras	common		|goto the common code
is_double:
	movew	#0x3c01,%d1		|write the bias for a dbl denorm
common:
	btst	#sign_bit,%a6@(ETEMP_EX)		|grab sign bit of mantissa
	beqs	pos		|
	bset	#15,%d1		|set sign bit because it is negative
pos:
	movew	%d1,%a6@(ETEMP_EX)
|				;put exponent on stack

	movew	%a6@(CMDREG1B),%d1
	andw	#0xe3ff,%d1		|clear out source specifier
	orw	#0x0800,%d1		|set source specifier to extended prec
	movew	%d1,%a6@(CMDREG1B)		|write back to the command word in stack
|				;this is needed to fix unsupp data stack
	lea	%a6@(ETEMP),%a0		|point %a0 to sop
	
	bsr	mk_norm		|convert sgl/dbl denorm to norm
	moveb	%a6@(L_SCR1),%a6@(STAG)		|put tag into source tag reg - %d0
	rts	|end_getop
|
| At this point, the source is definitely packed, whether
| instruction is dyadic or monadic is still unknown
|
pack_source:
	movel	%a6@(FPTEMP_LO),%a6@(ETEMP)		|write ms part of packed 
|				;number to etemp slot
	bsr	chk_dy_mo		|set dyadic/monadic flag
	bsr	unpack

	tstb	%a6@(DY_MO_FLG)
	beqs	end_getop		|if monadic, exit
|				;else, fix FPTEMP
pack_dya:
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|extract dest fp reg
	movel	#7,%d1
	subl	%d0,%d1
	clrl	%d0
	bset	%d1,%d0		|set up %d0 as a dynamic register mask
	fmovemx	%d0,%a6@(FPTEMP)		|write to FPTEMP

	btst	#7,%a6@(DTAG)		|check dest tag for unnorm or denorm
	bne	dst_ex_dnrm		|else, handle the unnorm or ext denorm
|
| Dest is not denormalized.  Check for norm, and set fpte15 
| accordingly.
|
	moveb	%a6@(DTAG),%d0
	andib	#0xf0,%d0		|strip to only dtag:fpte15
	tstb	%d0		|check for normalized value
	bnes	end_getop		|if inf/nan/zero leave get_op
	movew	%a6@(FPTEMP_EX),%d0
	andiw	#0x7fff,%d0
	cmpiw	#0x3fff,%d0		|check if fpte15 needs setting
	bges	end_getop		|if >= 0x3fff, leave fpte15=0
	orb	#0x10,%a6@(DTAG)
	bras	end_getop

|
| At this point, it is either an fmoveout packed, unnorm or denorm
|
opclass3:
	clrb	%a6@(DY_MO_FLG)		|set dyadic/monadic flag to monadic
	bfextu	%a6@(CMDREG1B){#4:#2},%d0
	cmpib	#3,%d0
	bne	src_ex_dnrm		|if not equal, must be unnorm or denorm
|				;else it is a packed move out
|				;exit
end_getop:
	rts

|
| Sets the DY_MO_FLG correctly. This is used only on if it is an
| unsupported data type exception.  Set if dyadic.
|
chk_dy_mo:
	movew	%a6@(CMDREG1B),%d0		|
	btst	#5,%d0		|testing extension command word
	beqs	set_mon		|if bit 5 = 0 then monadic
	btst	#4,%d0		|know that bit 5 = 1
	beqs	set_dya		|if bit 4 = 0 then dyadic
	andiw	#0x007f,%d0		|get rid of all but extension bits {#6:#0}
	cmpiw	#0x0038,%d0		|if extension = 0x38 then fcmp (dyadic)
	bnes	set_mon
set_dya:
	st	%a6@(DY_MO_FLG)		|set the inst flag type to dyadic
	rts
set_mon:
	clrb	%a6@(DY_MO_FLG)		|set the inst flag type to monadic
	rts
|
|	MK_NORM
|
| Normalizes unnormalized numbers, sets tag to norm or denorm, sets unfl
| exception if denorm.
|
| CASE opclass 0x0 unsupp
|	mk_norm till msb set
|	set tag = norm
|
| CASE opclass 0x0 unimp
|	mk_norm till msb set or exp = 0
|	if integer bit = 0
|	   tag = denorm
|	else
|	   tag = norm
|
| CASE opclass 011 unsupp
|	mk_norm till msb set or exp = 0
|	if integer bit = 0
|	   tag = denorm
|	   set unfl_nmcexe = 1
|	else
|	   tag = norm
|
| if exp <= 0x3fff
|   set ete15 or fpte15 = 1
| else set ete15 or fpte15 = 0

| input:
|	%a0 = points to operand to be normalized
| output:
|	L_SCR1{#7:#5} = operand tag (000 = norm, 100 = denorm)
|	L_SCR1{4}   = fpte15 or ete15 (0 = exp > 0x3fff, 1 = exp <=0x3fff)
|	the normalized operand is placed back on the fsave stack
mk_norm:
	clrl	%a6@(L_SCR1)
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|transform into internal extended format

	cmpib	#0x2c,%a6@(1+EXC_VEC)		|check if unimp
	bnes	uns_data		|branch if unsupp
	bsr	uni_inst		|call if unimp (opclass 0x0)
	bras	reload
uns_data:
	btst	#direction_bit,%a6@(CMDREG1B)		|check transfer direction
	bnes	bit_set		|branch if set (opclass 011)
	bsr	uns_opx		|call if opclass 0x0
	bras	reload
bit_set:
	bsr	uns_op3		|opclass 011
reload:
	cmpw	#0x3fff,%a0@(LOCAL_EX)		|if exp > 0x3fff
	bgts	end_mk		|   fpte15/ete15 already set to 0
	bset	#4,%a6@(L_SCR1)		|else set fpte15/ete15 to 1
|				;calling routine actually sets the 
|				;value on the stack (along with the 
|				;tag), since this routine doesn't 
|				;know if it should set ete15 or fpte15
|				;ie, it doesn't know if this is the 
|				;src op or dest op.
end_mk:
	bfclr	%a0@(LOCAL_SGN){#0:#8}
	beqs	end_mk_pos
	bset	#sign_bit,%a0@(LOCAL_EX)		|convert back to IEEE format
end_mk_pos:
	rts
|
|     CASE opclass 011 unsupp
|
uns_op3:
	bsr	nrm_zero		|normalize till msb = 1 or exp = zero
	btst	#7,%a0@(LOCAL_HI)		|if msb = 1
	bnes	no_unfl		|then branch
set_unfl:
	orb	#dnrm_tag,%a6@(L_SCR1)		|set denorm tag
	bset	#unfl_bit,%a6@(FPSR_EXCEPT)		|set unfl exception bit
no_unfl:
	rts
|
|     CASE opclass 0x0 unsupp
|
uns_opx:
	bsr	nrm_zero		|normalize the number
	btst	#7,%a0@(LOCAL_HI)		|check if integer bit (j-bit) is set 
	beqs	uns_den		|if clear then now have a denorm
uns_nrm:
	orb	#norm_tag,%a6@(L_SCR1)		|set tag to norm
	rts
uns_den:
	orb	#dnrm_tag,%a6@(L_SCR1)		|set tag to denorm
	rts
|
|     CASE opclass 0x0 unimp
|
uni_inst:
	bsr	nrm_zero
	btst	#7,%a0@(LOCAL_HI)		|check if integer bit (j-bit) is set 
	beqs	uni_den		|if clear then now have a denorm
uni_nrm:
	orb	#norm_tag,%a6@(L_SCR1)		|set tag to norm
	rts
uni_den:
	orb	#dnrm_tag,%a6@(L_SCR1)		|set tag to denorm
	rts

|
|	Decimal to binary conversion
|
| Special cases of inf and NaNs are completed outside of decbin.  
| If the input is an snan, the snan bit is not set.
| 
| input:
|	%a6@(ETEMP)	- points to packed decimal string in memory
| output:
|	%fp0	- contains packed string converted to extended precision
|	ETEMP	- same as %fp0
unpack:
	movew	%a6@(CMDREG1B),%d0		|examine command word, looking for fmove's
	andw	#0x3b,%d0
	beq	move_unpack		|special handling for fmove: must set FPSR_CC

	movew	%a6@(ETEMP),%d0		|get word with inf information
	bfextu	%d0{#20:#12},%d1		|get exponent into %d1
	cmpiw	#0x0fff,%d1		|test for inf or NaN
	bnes	try_zero		|if not equal, it is not special
	bfextu	%d0{#17:#3},%d1		|get SE and y bits into %d1
	cmpiw	#7,%d1		|SE and y bits must be on for special
	bnes	try_zero		|if not on, it is not special
|input is of the special cases of inf and NaN
	tstl	%a6@(ETEMP_HI)		|check ms mantissa
	bnes	fix_nan		|if non-zero, it is a NaN
	tstl	%a6@(ETEMP_LO)		|check ls mantissa
	bnes	fix_nan		|if non-zero, it is a NaN
	bra	finish		|special already on stack
fix_nan:
	btst	#signan_bit,%a6@(ETEMP_HI)		|test for snan
	bne	finish
	orl	#snaniop_mask,%a6@(USER_FPSR)		|always set snan if it is so
	bra	finish
try_zero:
	movew	%a6@(ETEMP_EX+2),%d0		|get word 4
	andiw	#0x000f,%d0		|clear all but last ni(y)bble
	tstw	%d0		|check for zero.
	bne	not_spec
	tstl	%a6@(ETEMP_HI)		|check words 3 and 2
	bne	not_spec
	tstl	%a6@(ETEMP_LO)		|check words 1 and 0
	bne	not_spec
	tstl	%a6@(ETEMP)		|test sign of the zero
	bges	pos_zero
	movel	#0x80000000,%a6@(ETEMP)		|write neg zero to etemp
	clrl	%a6@(ETEMP_HI)
	clrl	%a6@(ETEMP_LO)
	bra	finish
pos_zero:
	clrl	%a6@(ETEMP)
	clrl	%a6@(ETEMP_HI)
	clrl	%a6@(ETEMP_LO)
	bra	finish

not_spec:
	fmovemx	%fp0-%fp1,%a7@-		|save %fp0 - decbin returns in it
	bsr	decbin
	fmovex	%fp0,%a6@(ETEMP)		|put the unpacked sop in the fsave stack
	fmovemx	%a7@+,%fp0-%fp1
	fmovel	#0,%FPSR		|clr %fpsr from decbin
	bra	finish

|
| Special handling for packed move in:  Same results as all other
| packed cases, but we must set the %FPSR condition codes properly.
|
move_unpack:
	movew	%a6@(ETEMP),%d0		|get word with inf information
	bfextu	%d0{#20:#12},%d1		|get exponent into %d1
	cmpiw	#0x0fff,%d1		|test for inf or NaN
	bnes	mtry_zero		|if not equal, it is not special
	bfextu	%d0{#17:#3},%d1		|get SE and y bits into %d1
	cmpiw	#7,%d1		|SE and y bits must be on for special
	bnes	mtry_zero		|if not on, it is not special
|input is of the special cases of inf and NaN
	tstl	%a6@(ETEMP_HI)		|check ms mantissa
	bnes	mfix_nan		|if non-zero, it is a NaN
	tstl	%a6@(ETEMP_LO)		|check ls mantissa
	bnes	mfix_nan		|if non-zero, it is a NaN
|input is inf
	orl	#inf_mask,%a6@(USER_FPSR)		|set I bit
	tstl	%a6@(ETEMP)		|check sign
	bge	finish
	orl	#neg_mask,%a6@(USER_FPSR)		|set N bit
	bra	finish		|special already on stack
mfix_nan:
	orl	#nan_mask,%a6@(USER_FPSR)		|set NaN bit
	moveb	#nan_tag,%a6@(STAG)		|set stag to NaN
	btst	#signan_bit,%a6@(ETEMP_HI)		|test for snan
	bnes	mn_snan
	orl	#snaniop_mask,%a6@(USER_FPSR)		|set snan bit
	btst	#snan_bit,%a6@(FPCR_ENABLE)		|test for snan enabled
	bnes	mn_snan
	bset	#signan_bit,%a6@(ETEMP_HI)		|force snans to qnans
mn_snan:
	tstl	%a6@(ETEMP)		|check for sign
	bge	finish		|if clr, go on
	orl	#neg_mask,%a6@(USER_FPSR)		|set N bit
	bra	finish

mtry_zero:
	movew	%a6@(ETEMP_EX+2),%d0		|get word 4
	andiw	#0x000f,%d0		|clear all but last ni(y)bble
	tstw	%d0		|check for zero.
	bnes	mnot_spec
	tstl	%a6@(ETEMP_HI)		|check words 3 and 2
	bnes	mnot_spec
	tstl	%a6@(ETEMP_LO)		|check words 1 and 0
	bnes	mnot_spec
	tstl	%a6@(ETEMP)		|test sign of the zero
	bges	mpos_zero
	orl	#neg_mask+z_mask,%a6@(USER_FPSR)		|set N and Z
	movel	#0x80000000,%a6@(ETEMP)		|write neg zero to etemp
	clrl	%a6@(ETEMP_HI)
	clrl	%a6@(ETEMP_LO)
	bras	finish
mpos_zero:
	orl	#z_mask,%a6@(USER_FPSR)		|set Z
	clrl	%a6@(ETEMP)
	clrl	%a6@(ETEMP_HI)
	clrl	%a6@(ETEMP_LO)
	bras	finish

mnot_spec:
	fmovemx	%fp0-%fp1,%a7@-		|save %fp0 ,%fp1 - decbin returns in %fp0
	bsr	decbin
	fmovex	%fp0,%a6@(ETEMP)
|				;put the unpacked sop in the fsave stack
	fmovemx	%a7@+,%fp0-%fp1

finish:
	movew	%a6@(CMDREG1B),%d0		|get the command word
	andw	#0xfbff,%d0		|change the source specifier field to 
|				;extended (was packed).
	movew	%d0,%a6@(CMDREG1B)		|write command word back to fsave stack
|				;we need to do this so the 040 will 
|				;re-execute the inst. without taking 
|				;another packed trap.

fix_stag:
|Converted result is now in etemp on fsave stack, now set the source 
|tag (stag) 
|	if (ete =0x7fff) then INF or NAN
|		if (etemp = $x.0----0) then
|			stag = INF
|		else
|			stag = NAN
|	else
|		if (ete = 0x0000) then
|			stag = ZERO
|		else
|			stag = NORM
|
| Note also that the etemp_15 bit (just right of the stag) must
| be set accordingly.  
|
	movew	%a6@(ETEMP_EX),%d1
	andiw	#0x7fff,%d1		|strip sign
	cmpw	#0x7fff,%d1
	bnes	z_or_nrm
	movel	%a6@(ETEMP_HI),%d1
	bnes	is_nan
	movel	%a6@(ETEMP_LO),%d1
	bnes	is_nan
is_inf:
	moveb	#0x40,%a6@(STAG)
	movel	#0x40,%d0
	rts
is_nan:
	moveb	#0x60,%a6@(STAG)
	movel	#0x60,%d0
	rts
z_or_nrm:
	tstw	%d1		|
	bnes	is_nrm
is_zro:
| For a zero, set etemp_15
	moveb	#0x30,%a6@(STAG)
	movel	#0x20,%d0
	rts
is_nrm:
| For a norm, check if the exp <= 0x3fff; if so, set etemp_15
	cmpiw	#0x3fff,%d1
	bles	set_bit15
	clrb	%a6@(STAG)
	bras	end_is_nrm
set_bit15:
	moveb	#0x10,%a6@(STAG)
end_is_nrm:
	clrl	%d0
end_fix:
	rts
 
end_get:
	rts
|	end
