|	$NetBSD: sacos.sa,v 1.3 1994/10/26 07:49:27 cgd Exp $

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
|	sacos.sa 3.3 12/19/90
|
|	Description: The entry point sAcos computes the inverse cosine of
|		an input argument; sAcosd does the same except for denormalized
|		input.
|
|	Input: Double-extended number X in location pointed to
|		by address register %a0.
|
|	Output: The value arccos(X) returned in floating-point register %Fp0.
|
|	Accuracy and Monotonicity: The returned result is within 3 ulps in
|		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|		result is subsequently rounded to double precision. The 
|		result is provably monotonic in double precision.
|
|	Speed: The program sCOS takes approximately 310 cycles.
|
|	Algorithm:
|
|	ACOS
|	1. If |X| >= 1, go to 3.
|
|	2. (|X| < 1) Calculate acos(X) by
|		z := (1-X) / (1+X)
|		acos(X) = 2 * atan( sqrt(z) ).
|		Exit.
|
|	3. If |X| > 1, go to 5.
|
|	4. (|X| = 1) If X > 0, return 0. Otherwise, return Pi. Exit.
|
|	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
|		Exit.
|

|SACOS	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

PI:
	.long	0x40000000,0xC90FDAA2,0x2168C235,0x00000000
PIBY2:
	.long	0x3FFF0000,0xC90FDAA2,0x2168C235,0x00000000

|	xref	t_operr
|	xref	t_frcinx
|	xref	satan

	.global	sacosd
sacosd:
|--ACOS(X) = PI/2 FOR DENORMALIZED X
	fmovel	%d1,%fpcr		|...load user's rounding mode/precision
	fmovex	PIBY2,%FP0
	bra	t_frcinx

	.global	sacos
sacos:
	fmovex	%a0@,%FP0		|...LOAD INPUT

	movel	%a0@,%d0		|...pack exponent with upper 16 fraction
	movew	%a0@(4),%d0
	andil	#0x7FFFFFFF,%D0
	cmpil	#0x3FFF8000,%D0
	bges	ACOSBIG

|--THIS IS THE USUAL CASE, |X| < 1
|--ACOS(X) = 2 * ATAN(	SQRT( (1-X)/(1+X) )	)

	fmoves	#0r1.0,%FP1
	faddx	%FP0,%FP1		|...1+X
	fnegx	%FP0		|... -X
	fadds	#0r1.0,%FP0		|...1-X
	fdivx	%FP1,%FP0		|...(1-X)/(1+X)
	fsqrtx	%FP0		|...SQRT((1-X)/(1+X))
	fmovemx	%fp0-%fp0,%a0@		|...overwrite input
	movel	%d1,%sp@-		|save original users %fpcr
	clrl	%d1
	bsr	satan		|...ATAN(SQRT([1-X]/[1+X]))
	fmovel	%sp@+,%fpcr		|restore users exceptions
	faddx	%FP0,%FP0		|...2 * ATAN( STUFF )
	bra	t_frcinx

ACOSBIG:
	fabsx	%FP0
	fcmps	#0r1.0,%FP0
	fbgt	t_operr		|cause an operr exception

|--|X| = 1, ACOS(X) = 0 OR PI
	movel	%a0@,%d0		|...pack exponent with upper 16 fraction
	movew	%a0@(4),%d0
	tstl	%D0		|%D0 has original exponent+fraction
	bgts	ACOSP1

|--X = -1
|Returns PI and inexact exception
	fmovex	PI,%FP0
	fmovel	%d1,%FPCR
		.long	0xf23c4422,0x00800000		|cause an inexact exception to be put
|					;into the 040 - will not trap until next
|					;fp inst.
	bra	t_frcinx

ACOSP1:
	fmovel	%d1,%FPCR
	fmoves	#0r0.0,%FP0
	rts	|Facos		|of +1 is exact	

|	end
