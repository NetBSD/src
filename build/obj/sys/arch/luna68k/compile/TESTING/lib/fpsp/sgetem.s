|	$NetBSD: sgetem.sa,v 1.2 1994/10/26 07:49:45 cgd Exp $

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
|	sgetem.sa 3.1 12/10/90
|
|	The entry point sGETEXP returns the exponent portion 
|	of the input argument.  The exponent bias is removed
|	and the exponent value is returned as an extended 
|	precision number in %fp0.  sGETEXPD handles denormalized
|	numbers.
|
|	The entry point sGETMAN extracts the mantissa of the 
|	input argument.  The mantissa is converted to an 
|	extended precision number and returned in %fp0.  The
|	range of the result is [1.0 - 2.0).
|
|
|	Input:  Double-extended number X in the ETEMP space in
|		the floating-point save stack.
|
|	Output:	The functions return exp(X) or man(X) in %fp0.
|
|	Modified: %fp0.
|

|SGETEM	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	nrm_set

|
| This entry point is used by the unimplemented instruction exception
| handler.  It points %a0 to the input operand.
|
|
|
|	SGETEXP
|

	.global	sgetexp
sgetexp:
	movew	%a0@(LOCAL_EX),%d0		|get the exponent
	bclr	#15,%d0		|clear the sign bit
	subw	#0x3fff,%d0		|subtract off the bias
	fmovew	%d0,%fp0		|move the exp to %fp0
	rts

	.global	sgetexpd
sgetexpd:
	bclr	#sign_bit,%a0@(LOCAL_EX)
	bsr	nrm_set		|normalize (exp will go negative)
	movew	%a0@(LOCAL_EX),%d0		|load resulting exponent into %d0
	subw	#0x3fff,%d0		|subtract off the bias
	fmovew	%d0,%fp0		|move the exp to %fp0
	rts
|
|
| This entry point is used by the unimplemented instruction exception
| handler.  It points %a0 to the input operand.
|
|
|
|	SGETMAN
|
|
| For normalized numbers, leave the mantissa alone, simply load
| with an exponent of +/- 0x3fff.
|
	.global	sgetman
sgetman:
	movel	%a6@(USER_FPCR),%d0
	andil	#0xffffff00,%d0		|clear rounding precision and mode
	fmovel	%d0,%fpcr		|this %fpcr setting is used by the 882
	movew	%a0@(LOCAL_EX),%d0		|get the exp (really just want sign bit)
	orw	#0x7fff,%d0		|clear old exp
	bclr	#14,%d0		|make it the new exp +-3fff
	movew	%d0,%a0@(LOCAL_EX)		|move the sign & exp back to fsave stack
	fmovex	%a0@,%fp0		|put new value back in %fp0
	rts

|
| For denormalized numbers, shift the mantissa until the j-bit = 1,
| then load the exponent with +/1 0x3fff.
|
	.global	sgetmand
sgetmand:
	movel	%a0@(LOCAL_HI),%d0		|load ms mant in %d0
	movel	%a0@(LOCAL_LO),%d1		|load ls mant in %d1
	bsr	shft		|shift mantissa bits till msbit is set
	movel	%d0,%a0@(LOCAL_HI)		|put ms mant back on stack
	movel	%d1,%a0@(LOCAL_LO)		|put ls mant back on stack
	bras	sgetman

|
|	SHFT
|
|	Shifts the mantissa bits until msbit is set.
|	input:
|		ms mantissa part in %d0
|		ls mantissa part in %d1
|	output:
|		shifted bits in %d0 and %d1
shft:
	tstl	%d0		|if any bits set in ms mant
	bnes	upper		|then branch
|				;else no bits set in ms mant
	tstl	%d1		|test if any bits set in ls mant
	bnes	cont		|if set then continue
	bras	shft_end		|else return
cont:
	movel	%d3,%a7@-		|save %d3
	exg	%d0,%d1		|shift ls mant to ms mant
	bfffo	%d0{#0:#32},%d3		|find first 1 in ls mant to %d0
	lsll	%d3,%d0		|shift first 1 to integer bit in ms mant
	movel	%a7@+,%d3		|restore %d3
	bras	shft_end
upper:

	moveml	%d3/%d5/%d6,%a7@-		|save registers
	bfffo	%d0{#0:#32},%d3		|find first 1 in ls mant to %d0
	lsll	%d3,%d0		|shift ms mant until j-bit is set
	movel	%d1,%d6		|save ls mant in %d6
	lsll	%d3,%d1		|shift ls mant by count
	movel	#32,%d5
	subl	%d3,%d5		|sub 32 from shift for ls mant
	lsrl	%d5,%d6		|shift off all bits but those that will
|				;be shifted into ms mant
	orl	%d6,%d0		|shift the ls mant bits into the ms mant
	moveml	%a7@+,%d3/%d5/%d6		|restore registers
shft_end:
	rts

|	end
