|	$NetBSD: round.sa,v 1.6 2022/04/08 14:33:24 andvar Exp $

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
|	round.sa 3.4 7/29/91
|
|	handle rounding and normalization tasks
|

|ROUND	IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|
|	round --- round result according to precision/mode
|
|	%a0 points to the input operand in the internal extended format 
|	%d1(high word) contains rounding precision:
|		ext = 0x0000xxxx
|		sgl = 0x0001xxxx
|		dbl = 0x0002xxxx
|	%d1(low word) contains rounding mode:
|		RN  = $xxxx0000
|		RZ  = $xxxx0001
|		RM  = $xxxx0010
|		RP  = $xxxx0011
|	%d0{#31:#29} contains the g,r,s bits (extended)
|
|	On return the value pointed to by %a0 is correctly rounded,
|	%a0 is preserved and the g-r-s bits in %d0 are cleared.
|	The result is not typed - the tag field is invalid.  The
|	result is still in the internal extended format.
|
|	The INEX bit of USER_FPSR will be set if the rounded result was
|	inexact (i.e. if any of the g-r-s bits were set).
|

	.global	round
round:
| If g=r=s=0 then result is exact and round is done, else set 
| the inex flag in status reg and continue.  
|
	bsrs	ext_grs		|this subroutine looks at the 
|					:rounding precision and sets 
|					;the appropriate g-r-s bits.
	tstl	%d0		|if grs are zero, go force
	bne	rnd_cont		|lower bits to zero for size
	
	swap	%d1		|set up %d1.w for round prec.
	bra	truncate

rnd_cont:
|
| Use rounding mode as an index into a jump table for these modes.
|
	orl	#inx2a_mask,%a6@(USER_FPSR)		|set inex2/ainex
	lea	mode_tab,%a1
	movel	%a1@(%d1:w:4),%a1
	jmp	%a1@
|
| Jump table indexed by rounding mode in %d1.w.  All following assumes
| grs != 0.
|
mode_tab:
	.long	rnd_near
	.long	rnd_zero
	.long	rnd_mnus
	.long	rnd_plus
|
|	ROUND PLUS INFINITY
|
|	If sign of fp number = 0 (positive), then add 1 to l.
|
rnd_plus:
	swap	%d1		|set up %d1 for round prec.
	tstb	%a0@(LOCAL_SGN)		|check for sign
	bmi	truncate		|if positive then truncate
	movel	#0xffffffff,%d0		|force g,r,s to be all f's
	lea	add_to_l,%a1
	movel	%a1@(%d1:w:4),%a1
	jmp	%a1@
|
|	ROUND MINUS INFINITY
|
|	If sign of fp number = 1 (negative), then add 1 to l.
|
rnd_mnus:
	swap	%d1		|set up %d1 for round prec.
	tstb	%a0@(LOCAL_SGN)		|check for sign	
	bpl	truncate		|if negative then truncate
	movel	#0xffffffff,%d0		|force g,r,s to be all f's
	lea	add_to_l,%a1
	movel	%a1@(%d1:w:4),%a1
	jmp	%a1@
|
|	ROUND ZERO
|
|	Always truncate.
rnd_zero:
	swap	%d1		|set up %d1 for round prec.
	bra	truncate
|
|
|	ROUND NEAREST
|
|	If (g=1), then add 1 to l and if (r=s=0), then clear l
|	Note that this will round to even in case of a tie.
|
rnd_near:
	swap	%d1		|set up %d1 for round prec.
	addl	%d0,%d0		|shift g-bit to c-bit
	bcc	truncate		|if (g=1) then
	lea	add_to_l,%a1
	movel	%a1@(%d1:w:4),%a1
	jmp	%a1@

|
|	ext_grs --- extract guard, round and sticky bits
|
| Input:	%d1 =		PREC:ROUND
| Output:  	%d0{#31:#29}=	guard, round, sticky
|
| The ext_grs extract the guard/round/sticky bits according to the
| selected rounding precision. It is called by the round subroutine
| only.  All registers except %d0 are kept intact. %d0 becomes an 
| updated guard,round,sticky in %d0{#31:#29}
|
| Notes: the ext_grs uses the round PREC, and therefore has to swap %d1
|	 prior to usage, and needs to restore %d1 to original.
|
ext_grs:
	swap	%d1		|have %d1.w point to round precision
	tstw	%d1
	bnes	sgl_or_dbl
	bras	end_ext_grs
 
sgl_or_dbl:
	moveml	%d2/%d3,%a7@-		|make some temp registers
	cmpiw	#1,%d1
	bnes	grs_dbl
grs_sgl:
	bfextu	%a0@(LOCAL_HI){#24:#2},%d3		|sgl prec. g-r are 2 bits right
	movel	#30,%d2		|of the sgl prec. limits
	lsll	%d2,%d3		|shift g-r bits to MSB of %d3
	movel	%a0@(LOCAL_HI),%d2		|get word 2 for s-bit test
	andil	#0x0000003f,%d2		|s bit is the or of all other 
	bnes	st_stky		|bits to the right of g-r
	tstl	%a0@(LOCAL_LO)		|test lower mantissa
	bnes	st_stky		|if any are set, set sticky
	tstl	%d0		|test original g,r,s
	bnes	st_stky		|if any are set, set sticky
	bras	end_sd		|if words 3 and 4 are clr, exit
grs_dbl:
	bfextu	%a0@(LOCAL_LO){#21:#2},%d3		|dbl-prec. g-r are 2 bits right
	movel	#30,%d2		|of the dbl prec. limits
	lsll	%d2,%d3		|shift g-r bits to the MSB of %d3
	movel	%a0@(LOCAL_LO),%d2		|get lower mantissa  for s-bit test
	andil	#0x000001ff,%d2		|s bit is the or-ing of all 
	bnes	st_stky		|other bits to the right of g-r
	tstl	%d0		|test word original g,r,s
	bnes	st_stky		|if any are set, set sticky
	bras	end_sd		|if clear, exit
st_stky:
	bset	#rnd_stky_bit,%d3
end_sd:
	movel	%d3,%d0		|return grs to %d0
	moveml	%a7@+,%d2/%d3		|restore scratch registers
end_ext_grs:
	swap	%d1		|restore %d1 to original
	rts

|*******************  Local Equates
	.set	ad_1_sgl,0x00000100		|constant to add 1 to l-bit in sgl prec
	.set	ad_1_dbl,0x00000800		|constant to add 1 to l-bit in dbl prec


|Jump table for adding 1 to the l-bit indexed by rnd prec

add_to_l:
	.long	add_ext
	.long	add_sgl
	.long	add_dbl
	.long	add_dbl
|
|	ADD SINGLE
|
add_sgl:
	addl	#ad_1_sgl,%a0@(LOCAL_HI)
	bccs	scc_clr		|no mantissa overflow
	roxrw	%a0@(LOCAL_HI)		|shift v-bit back in
	roxrw	%a0@(LOCAL_HI+2)		|shift v-bit back in
	addw	#0x1,%a0@(LOCAL_EX)		|and incr exponent
scc_clr:
	tstl	%d0		|test for rs = 0
	bnes	sgl_done
	andiw	#0xfe00,%a0@(LOCAL_HI+2)		|clear the l-bit
sgl_done:
	andil	#0xffffff00,%a0@(LOCAL_HI)		|truncate bits beyond sgl limit
	clrl	%a0@(LOCAL_LO)		|clear %d2
	rts

|
|	ADD EXTENDED
|
add_ext:
	addql	#1,%a0@(LOCAL_LO)		|add 1 to l-bit
	bccs	xcc_clr		|test for carry out
	addql	#1,%a0@(LOCAL_HI)		|propagate carry
	bccs	xcc_clr
	roxrw	%a0@(LOCAL_HI)		|mant is 0 so restore v-bit
	roxrw	%a0@(LOCAL_HI+2)		|mant is 0 so restore v-bit
	roxrw	%a0@(LOCAL_LO)
	roxrw	%a0@(LOCAL_LO+2)
	addw	#0x1,%a0@(LOCAL_EX)		|and inc exp
xcc_clr:
	tstl	%d0		|test rs = 0
	bnes	add_ext_done
	andib	#0xfe,%a0@(LOCAL_LO+3)		|clear the l bit
add_ext_done:
	rts
|
|	ADD DOUBLE
|
add_dbl:
	addl	#ad_1_dbl,%a0@(LOCAL_LO)
	bccs	dcc_clr
	addql	#1,%a0@(LOCAL_HI)		|propagate carry
	bccs	dcc_clr
	roxrw	%a0@(LOCAL_HI)		|mant is 0 so restore v-bit
	roxrw	%a0@(LOCAL_HI+2)		|mant is 0 so restore v-bit
	roxrw	%a0@(LOCAL_LO)
	roxrw	%a0@(LOCAL_LO+2)
	addw	#0x1,%a0@(LOCAL_EX)		|incr exponent
dcc_clr:
	tstl	%d0		|test for rs = 0
	bnes	dbl_done
	andiw	#0xf000,%a0@(LOCAL_LO+2)		|clear the l-bit

dbl_done:
	andil	#0xfffff800,%a0@(LOCAL_LO)		|truncate bits beyond dbl limit
	rts

error:
	rts
|
| Truncate all other bits
|
trunct:
	.long	end_rnd
	.long	sgl_done
	.long	dbl_done
	.long	dbl_done

truncate:
	lea	trunct,%a1
	movel	%a1@(%d1:w:4),%a1
	jmp	%a1@

end_rnd:
	rts

|
|	NORMALIZE
|
| These routines (nrm_zero & nrm_set) normalize the unnorm.  This 
| is done by shifting the mantissa left while decrementing the 
| exponent.
|
| NRM_SET shifts and decrements until there is a 1 set in the integer 
| bit of the mantissa (msb in %d1).
|
| NRM_ZERO shifts and decrements until there is a 1 set in the integer 
| bit of the mantissa (msb in %d1) unless this would mean the exponent 
| would go less than 0.  In that case the number becomes a denorm - the 
| exponent (%d0) is set to 0 and the mantissa (%d1 & %d2) is not 
| normalized.
|
| Note that both routines have been optimized (for the worst case) and 
| therefore do not have the easy to follow decrement/shift loop.
|
|	NRM_ZERO
|
|	Distance to first 1 bit in mantissa = X
|	Distance to 0 from exponent = Y
|	If X < Y
|	Then
|	  nrm_set
|	Else
|	  shift mantissa by Y
|	  set exponent = 0
|
|input:
|	FP_SCR1 = exponent, ms mantissa part, ls mantissa part
|output:
|	L_SCR1{4} = fpte15 or ete15 bit
|
	.global	nrm_zero
nrm_zero:
	movew	%a0@(LOCAL_EX),%d0
	cmpw	#64,%d0		|see if exp > 64 
	bmis	d0_less
	bsr	nrm_set		|exp > 64 so exp won't exceed 0 
	rts
d0_less:
	moveml	%d2/%d3/%d5/%d6,%a7@-
	movel	%a0@(LOCAL_HI),%d1
	movel	%a0@(LOCAL_LO),%d2

	bfffo	%d1{#0:#32},%d3		|get the distance to the first 1 
|				;in ms mant
	beqs	ms_clr		|branch if no bits were set
	cmpw	%d3,%d0		|of X>Y
	bmis	greater		|then exp will go past 0 (neg) if 
|				;it is just shifted
	bsr	nrm_set		|else exp won't go past 0
	moveml	%a7@+,%d2/%d3/%d5/%d6
	rts	
greater:
	movel	%d2,%d6		|save ls mant in %d6
	lsll	%d0,%d2		|shift ls mant by count
	lsll	%d0,%d1		|shift ms mant by count
	movel	#32,%d5
	subl	%d0,%d5		|make op a denorm by shifting bits 
	lsrl	%d5,%d6		|by the number in the exp, then 
|				;set exp = 0.
	orl	%d6,%d1		|shift the ls mant bits into the ms mant
	clrl	%d0		|same as if decremented exp to 0 
|				;while shifting
	movew	%d0,%a0@(LOCAL_EX)
	movel	%d1,%a0@(LOCAL_HI)
	movel	%d2,%a0@(LOCAL_LO)
	moveml	%a7@+,%d2/%d3/%d5/%d6
	rts
ms_clr:
	bfffo	%d2{#0:#32},%d3		|check if any bits set in ls mant
	beqs	all_clr		|branch if none set
	addw	#32,%d3
	cmpw	%d3,%d0		|if X>Y
	bmis	greater		|then branch
	bsr	nrm_set		|else exp won't go past 0
	moveml	%a7@+,%d2/%d3/%d5/%d6
	rts
all_clr:
	clrw	%a0@(LOCAL_EX)		|no mantissa bits set. Set exp = 0.
	moveml	%a7@+,%d2/%d3/%d5/%d6
	rts
|
|	NRM_SET
|
	.global	nrm_set
nrm_set:
	movel	%d7,%a7@-
	bfffo	%a0@(LOCAL_HI){#0:#32},%d7		|find first 1 in ms mant to %d7)
	beqs	lower		|branch if ms mant is all 0's

	movel	%d6,%a7@-

	subw	%d7,%a0@(LOCAL_EX)		|sub exponent by count
	movel	%a0@(LOCAL_HI),%d0		|%d0 has ms mant
	movel	%a0@(LOCAL_LO),%d1		|%d1 has ls mant

	lsll	%d7,%d0		|shift first 1 to j bit position
	movel	%d1,%d6		|copy ls mant into %d6
	lsll	%d7,%d6		|shift ls mant by count
	movel	%d6,%a0@(LOCAL_LO)		|store ls mant into memory
	moveql	#32,%d6
	subl	%d7,%d6		|continue shift
	lsrl	%d6,%d1		|shift off all bits but those that will
|				;be shifted into ms mant
	orl	%d1,%d0		|shift the ls mant bits into the ms mant
	movel	%d0,%a0@(LOCAL_HI)		|store ms mant into memory
	moveml	%a7@+,%d7/%d6		|restore registers
	rts

|
| We get here if ms mant was = 0, and we assume ls mant has bits 
| set (otherwise this would have been tagged a zero not a denorm).
|
lower:
	movew	%a0@(LOCAL_EX),%d0		|%d0 has exponent
	movel	%a0@(LOCAL_LO),%d1		|%d1 has ls mant
	subw	#32,%d0		|account for ms mant being all zeros
	bfffo	%d1{#0:#32},%d7		|find first 1 in ls mant to %d7)
	subw	%d7,%d0		|subtract shift count from exp
	lsll	%d7,%d1		|shift first 1 to integer bit in ms mant
	movew	%d0,%a0@(LOCAL_EX)		|store ms mant
	movel	%d1,%a0@(LOCAL_HI)		|store exp
	clrl	%a0@(LOCAL_LO)		|clear ls mant
	movel	%a7@+,%d7
	rts
|
|	denorm --- denormalize an intermediate result
|
|	Used by underflow.
|
| Input: 
|	%a0	 points to the operand to be denormalized
|		 (in the internal extended format)
|		 
|	%d0: 	 rounding precision
| Output:
|	%a0	 points to the denormalized result
|		 (in the internal extended format)
|
|	%d0 	is guard,round,sticky
|
| %d0 comes into this routine with the rounding precision. It 
| is then loaded with the denormalized exponent threshold for the 
| rounding precision.
|

	.global	denorm
denorm:
	btst	#6,%a0@(LOCAL_EX)		|check for exponents between 0x7fff-0x4000
	beqs	no_sgn_ext		|
	bset	#7,%a0@(LOCAL_EX)		|sign extend if it is so
no_sgn_ext:

	tstb	%d0		|if 0 then extended precision
	bnes	not_ext		|else branch

	clrl	%d1		|load %d1 with ext threshold
	clrl	%d0		|clear the sticky flag
	bsr	dnrm_lp		|denormalize the number
	tstb	%d1		|check for inex
	beq	no_inex		|if clr, no inex
	bras	dnrm_inex		|if set, set inex

not_ext:
	cmpil	#1,%d0		|if 1 then single precision
	beqs	load_sgl		|else must be 2, double prec

load_dbl:
	movew	#dbl_thresh,%d1		|put copy of threshold in %d1
	movel	%d1,%d0		|copy %d1 into %d0
	subw	%a0@(LOCAL_EX),%d0		|diff = threshold - exp
	cmpw	#67,%d0		|if diff > 67 (mant + grs bits)
	bpls	chk_stky		|then branch (all bits would be 
|				; shifted off in denorm routine)
	clrl	%d0		|else clear the sticky flag
	bsr	dnrm_lp		|denormalize the number
	tstb	%d1		|check flag
	beqs	no_inex		|if clr, no inex
	bras	dnrm_inex		|if set, set inex

load_sgl:
	movew	#sgl_thresh,%d1		|put copy of threshold in %d1
	movel	%d1,%d0		|copy %d1 into %d0
	subw	%a0@(LOCAL_EX),%d0		|diff = threshold - exp
	cmpw	#67,%d0		|if diff > 67 (mant + grs bits)
	bpls	chk_stky		|then branch (all bits would be 
|				; shifted off in denorm routine)
	clrl	%d0		|else clear the sticky flag
	bsr	dnrm_lp		|denormalize the number
	tstb	%d1		|check flag
	beqs	no_inex		|if clr, no inex
	bras	dnrm_inex		|if set, set inex

chk_stky:
	tstl	%a0@(LOCAL_HI)		|check for any bits set
	bnes	set_stky
	tstl	%a0@(LOCAL_LO)		|check for any bits set
	bnes	set_stky
	bras	clr_mant
set_stky:
	orl	#inx2a_mask,%a6@(USER_FPSR)		|set inex2/ainex
	movel	#0x20000000,%d0		|set sticky bit in return value
clr_mant:
	movew	%d1,%a0@(LOCAL_EX)		|load exp with threshold
	clrl	%a0@(LOCAL_HI)		|set %d1 = 0 (ms mantissa)
	clrl	%a0@(LOCAL_LO)		|set %d2 = 0 (ms mantissa)
	rts
dnrm_inex:
	orl	#inx2a_mask,%a6@(USER_FPSR)		|set inex2/ainex
no_inex:
	rts

|
|	dnrm_lp --- normalize exponent/mantissa to specified threshold
|
| Input:
|	%a0		points to the operand to be denormalized
|	%d0{#31:#29} 	initial guard,round,sticky
|	%d1{#15:#0}	denormalization threshold
| Output:
|	%a0		points to the denormalized operand
|	%d0{#31:#29}	final guard,round,sticky
|	%d1.b		inexact flag:  all ones means inexact result
|
| The LOCAL_LO and LOCAL_GRS parts of the value are copied to FP_SCR2
| so that bfext can be used to extract the new low part of the mantissa.
| Dnrm_lp can be called with %a0 pointing to ETEMP or WBTEMP and there 
| is no LOCAL_GRS scratch word following it on the fsave frame.
|
	.global	dnrm_lp
dnrm_lp:
	movel	%d2,%sp@-		|save %d2 for temp use
	btst	#E3,%a6@(E_BYTE)		|test for type E3 exception
	beqs	not_E3		|not type E3 exception
	bfextu	%a6@(WBTEMP_GRS){#6:#3},%d2		|extract guard,round, sticky  bit
	movel	#29,%d0
	lsll	%d0,%d2		|shift g,r,s to their positions
	movel	%d2,%d0
not_E3:
	movel	%sp@+,%d2		|restore %d2
	movel	%a0@(LOCAL_LO),%a6@(FP_SCR2+LOCAL_LO)
	movel	%d0,%a6@(FP_SCR2+LOCAL_GRS)
	movel	%d1,%d0		|copy the denorm threshold
	subw	%a0@(LOCAL_EX),%d1		|%d1 = threshold - uns exponent
	bles	no_lp		|%d1 <= 0
	cmpw	#32,%d1		|
	blts	case_1		|0 = %d1 < 32 
	cmpw	#64,%d1
	blts	case_2		|32 <= %d1 < 64
	bra	case_3		|%d1 >= 64
|
| No normalization necessary
|
no_lp:
	clrb	%d1		|set no inex2 reported
	movel	%a6@(FP_SCR2+LOCAL_GRS),%d0		|restore original g,r,s
	rts
|
| case (0<%d1<32)
|
case_1:
	movel	%d2,%sp@-
	movew	%d0,%a0@(LOCAL_EX)		|exponent = denorm threshold
	movel	#32,%d0
	subw	%d1,%d0		|%d0 = 32 - %d1
	bfextu	%a0@(LOCAL_EX){%d0:#32},%d2
	bfextu	%d2{%d1:%d0},%d2		|%d2 = new LOCAL_HI
	bfextu	%a0@(LOCAL_HI){%d0:#32},%d1		|%d1 = new LOCAL_LO
	bfextu	%a6@(FP_SCR2+LOCAL_LO){%d0:#32},%d0		|%d0 = new G,R,S
	movel	%d2,%a0@(LOCAL_HI)		|store new LOCAL_HI
	movel	%d1,%a0@(LOCAL_LO)		|store new LOCAL_LO
	clrb	%d1
	bftst	%d0{#2:#30}		|
	beqs	c1nstky
	bset	#rnd_stky_bit,%d0
	st	%d1
c1nstky:
	movel	%a6@(FP_SCR2+LOCAL_GRS),%d2		|restore original g,r,s
	andil	#0xe0000000,%d2		|clear all but G,R,S
	tstl	%d2		|test if original G,R,S are clear
	beqs	grs_clear
	orl	#0x20000000,%d0		|set sticky bit in %d0
grs_clear:
	andil	#0xe0000000,%d0		|clear all but G,R,S
	movel	%sp@+,%d2
	rts
|
| case (32<=%d1<64)
|
case_2:
	movel	%d2,%sp@-
	movew	%d0,%a0@(LOCAL_EX)		|unsigned exponent = threshold
	subw	#32,%d1		|%d1 now between 0 and 32
	movel	#32,%d0
	subw	%d1,%d0		|%d0 = 32 - %d1
	bfextu	%a0@(LOCAL_EX){%d0:#32},%d2
	bfextu	%d2{%d1:%d0},%d2		|%d2 = new LOCAL_LO
	bfextu	%a0@(LOCAL_HI){%d0:#32},%d1		|%d1 = new G,R,S
	bftst	%d1{#2:#30}
	bnes	c2_sstky		|bra if sticky bit to be set
	bftst	%a6@(FP_SCR2+LOCAL_LO){%d0:#32}
	bnes	c2_sstky		|bra if sticky bit to be set
	movel	%d1,%d0
	clrb	%d1
	bras	end_c2
c2_sstky:
	movel	%d1,%d0
	bset	#rnd_stky_bit,%d0
	st	%d1
end_c2:
	clrl	%a0@(LOCAL_HI)		|store LOCAL_HI = 0
	movel	%d2,%a0@(LOCAL_LO)		|store LOCAL_LO
	movel	%a6@(FP_SCR2+LOCAL_GRS),%d2		|restore original g,r,s
	andil	#0xe0000000,%d2		|clear all but G,R,S
	tstl	%d2		|test if original G,R,S are clear
	beqs	clear_grs		|
	orl	#0x20000000,%d0		|set sticky bit in %d0
clear_grs:
	andil	#0xe0000000,%d0		|get rid of all but G,R,S
	movel	%sp@+,%d2
	rts
|
| %d1 >= 64 Force the exponent to be the denorm threshold with the
| correct sign.
|
case_3:
	movew	%d0,%a0@(LOCAL_EX)
	tstw	%a0@(LOCAL_SGN)
	bges	c3con
c3neg:
	orl	#0x80000000,%a0@(LOCAL_EX)
c3con:
	cmpw	#64,%d1
	beqs	sixty_four
	cmpw	#65,%d1
	beqs	sixty_five
|
| Shift value is out of range.  Set %d1 for inex2 flag and
| return a zero with the given threshold.
|
	clrl	%a0@(LOCAL_HI)
	clrl	%a0@(LOCAL_LO)
	movel	#0x20000000,%d0
	st	%d1
	rts

sixty_four:
	movel	%a0@(LOCAL_HI),%d0
	bfextu	%d0{#2:#30},%d1
	andil	#0xc0000000,%d0
	bras	c3com
	
sixty_five:
	movel	%a0@(LOCAL_HI),%d0
	bfextu	%d0{#1:#31},%d1
	andil	#0x80000000,%d0
	lsrl	#1,%d0		|shift high bit into R bit

c3com:
	tstl	%d1
	bnes	c3ssticky
	tstl	%a0@(LOCAL_LO)
	bnes	c3ssticky
	tstb	%a6@(FP_SCR2+LOCAL_GRS)
	bnes	c3ssticky
	clrb	%d1
	bras	c3end

c3ssticky:
	bset	#rnd_stky_bit,%d0
	st	%d1
c3end:
	clrl	%a0@(LOCAL_HI)
	clrl	%a0@(LOCAL_LO)
	rts

|	end
