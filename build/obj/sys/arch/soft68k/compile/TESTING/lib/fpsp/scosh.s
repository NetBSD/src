|	$NetBSD: scosh.sa,v 1.2 1994/10/26 07:49:39 cgd Exp $

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
|	scosh.sa 3.1 12/10/90
|
|	The entry point sCosh computes the hyperbolic cosine of
|	an input argument; sCoshd does the same except for denormalized
|	input.
|
|	Input: Double-extended number X in location pointed to
|		by address register %a0.
|
|	Output: The value cosh(X) returned in floating-point register %Fp0.
|
|	Accuracy and Monotonicity: The returned result is within 3 ulps in
|		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|		result is subsequently rounded to double precision. The
|		result is provably monotonic in double precision.
|
|	Speed: The program sCOSH takes approximately 250 cycles.
|
|	Algorithm:
|
|	COSH
|	1. If |X| > 16380 log2, go to 3.
|
|	2. (|X| <= 16380 log2) Cosh(X) is obtained by the formulae
|		y = |X|, z = exp(Y), and
|		cosh(X) = (1/2)*( z + 1/z ).
|		Exit.
|
|	3. (|X| > 16380 log2). If |X| > 16480 log2, go to 5.
|
|	4. (16380 log2 < |X| <= 16480 log2)
|		cosh(X) = sign(X) * exp(|X|)/2.
|		However, invoking exp(|X|) may cause premature overflow.
|		Thus, we calculate sinh(X) as follows:
|		Y	:= |X|
|		Fact	:=	2**(16380)
|		Y'	:= Y - 16381 log2
|		cosh(X) := Fact * exp(Y').
|		Exit.
|
|	5. (|X| > 16480 log2) sinh(X) must overflow. Return
|		Huge*Huge to generate overflow and an infinity with
|		the appropriate sign. Huge is the largest finite number in
|		extended format. Exit.
|

|SCOSH	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

|	xref	t_ovfl
|	xref	t_frcinx
|	xref	setox

T1:
	.long	0x40C62D38,0xD3D64634		|... 16381 LOG2 LEAD
T2:
	.long	0x3D6F90AE,0xB1E75CC7		|... 16381 LOG2 TRAIL

TWO16380:
	.long	0x7FFB0000,0x80000000,0x00000000,0x00000000

	.global	scoshd
scoshd:
|--COSH(X) = 1 FOR DENORMALIZED X

	fmoves	#0r1.0,%FP0

	fmovel	%d1,%FPCR
		.long	0xf23c4422,0x00800000
	bra	t_frcinx

	.global	scosh
scosh:
	fmovex	%a0@,%FP0		|...LOAD INPUT

	movel	%a0@,%d0
	movew	%a0@(4),%d0
	andil	#0x7FFFFFFF,%d0
	cmpil	#0x400CB167,%d0
	bgts	COSHBIG

|--THIS IS THE USUAL CASE, |X| < 16380 LOG2
|--COSH(X) = (1/2) * ( EXP(X) + 1/EXP(X) )

	fabsx	%FP0		|...|X|

	movel	%d1,%sp@-
	clrl	%d1
	fmovemx	%fp0-%fp0,%a0@		|pass parameter to setox
	bsr	setox		|...%FP0 IS EXP(|X|)
	fmuls	#0r0.5,%FP0		|...(1/2)EXP(|X|)
	movel	%sp@+,%d1

	fmoves	#0r0.25,%FP1		|...(1/4)
	fdivx	%FP0,%FP1		|...1/(2 EXP(|X|))

	fmovel	%d1,%FPCR
	faddx	%fp1,%FP0

	bra	t_frcinx

COSHBIG:
	cmpil	#0x400CB2B3,%d0
	bgts	COSHHUGE

	fabsx	%FP0
	fsubd	%pc@(T1),%FP0		|...(|X|-16381LOG2_LEAD)
	fsubd	%pc@(T2),%FP0		|...|X| - 16381 LOG2, ACCURATE

	movel	%d1,%sp@-
	clrl	%d1
	fmovemx	%fp0-%fp0,%a0@
	bsr	setox
	fmovel	%sp@+,%fpcr

	fmulx	%pc@(TWO16380),%FP0
	bra	t_frcinx

COSHHUGE:
	fmovel	#0,%fpsr		|clr N bit if set by source
	bclr	#7,%a0@		|always return positive value
	fmovemx	%a0@,%fp0-%fp0
	bra	t_ovfl

|	end
