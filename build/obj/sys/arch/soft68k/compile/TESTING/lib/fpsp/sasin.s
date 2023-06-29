|	$NetBSD: sasin.sa,v 1.2 1994/10/26 07:49:29 cgd Exp $

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
|	sasin.sa 3.3 12/19/90
|
|	Description: The entry point sAsin computes the inverse sine of
|		an input argument; sAsind does the same except for denormalized
|		input.
|
|	Input: Double-extended number X in location pointed to
|		by address register %a0.
|
|	Output: The value arcsin(X) returned in floating-point register %Fp0.
|
|	Accuracy and Monotonicity: The returned result is within 3 ulps in
|		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|		result is subsequently rounded to double precision. The 
|		result is provably monotonic in double precision.
|
|	Speed: The program sASIN takes approximately 310 cycles.
|
|	Algorithm:
|
|	ASIN
|	1. If |X| >= 1, go to 3.
|
|	2. (|X| < 1) Calculate asin(X) by
|		z := sqrt( [1-X][1+X] )
|		asin(X) = atan( x / z ).
|		Exit.
|
|	3. If |X| > 1, go to 5.
|
|	4. (|X| = 1) sgn := sign(X), return asin(X) := sgn * Pi/2. Exit.
|
|	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
|		Exit.
|

|SASIN	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

PIBY2:
	.long	0x3FFF0000,0xC90FDAA2,0x2168C235,0x00000000

|	xref	t_operr
|	xref	t_frcinx
|	xref	t_extdnrm
|	xref	satan

	.global	sasind
sasind:
|--ASIN(X) = X FOR DENORMALIZED X

	bra	t_extdnrm

	.global	sasin
sasin:
	fmovex	%a0@,%FP0		|...LOAD INPUT

	movel	%a0@,%d0
	movew	%a0@(4),%d0
	andil	#0x7FFFFFFF,%D0
	cmpil	#0x3FFF8000,%D0
	bges	asinbig

|--THIS IS THE USUAL CASE, |X| < 1
|--ASIN(X) = ATAN( X / SQRT( (1-X)(1+X) ) )

	fmoves	#0r1.0,%FP1
	fsubx	%FP0,%FP1		|...1-X
	fmovemx	%fp2-%fp2,%a7@-
	fmoves	#0r1.0,%FP2
	faddx	%FP0,%FP2		|...1+X
	fmulx	%FP2,%FP1		|...(1+X)(1-X)
	fmovemx	%a7@+,%fp2-%fp2
	fsqrtx	%FP1		|...SQRT([1-X][1+X])
	fdivx	%FP1,%FP0		|...X/SQRT([1-X][1+X])
	fmovemx	%fp0-%fp0,%a0@
	bsr	satan
	bra	t_frcinx

asinbig:
	fabsx	%FP0		|...|X|
	fcmps	#0r1.0,%FP0
	fbgt	t_operr		|cause an operr exception

|--|X| = 1, ASIN(X) = +- PI/2.

	fmovex	PIBY2,%FP0
	movel	%a0@,%d0
	andil	#0x80000000,%D0		|...SIGN BIT OF X
	oril	#0x3F800000,%D0		|...+-1 IN SGL FORMAT
	movel	%D0,%sp@-		|...push SIGN(X) IN SGL-FMT
	fmovel	%d1,%FPCR		|
	fmuls	%sp@+,%FP0
	bra	t_frcinx

|	end
