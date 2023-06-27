|	$NetBSD: stanh.sa,v 1.3 1994/10/26 07:50:12 cgd Exp $

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
|	stanh.sa 3.1 12/10/90
|
|	The entry point sTanh computes the hyperbolic tangent of
|	an input argument; sTanhd does the same except for denormalized
|	input.
|
|	Input: Double-extended number X in location pointed to
|		by address register %a0.
|
|	Output: The value tanh(X) returned in floating-point register %Fp0.
|
|	Accuracy and Monotonicity: The returned result is within 3 ulps in
|		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|		result is subsequently rounded to double precision. The
|		result is provably monotonic in double precision.
|
|	Speed: The program stanh takes approximately 270 cycles.
|
|	Algorithm:
|
|	TANH
|	1. If |X| >= (5/2) log2 or |X| <= 2**(-40), go to 3.
|
|	2. (2**(-40) < |X| < (5/2) log2) Calculate tanh(X) by
|		sgn := sign(X), y := 2|X|, z := expm1(Y), and
|		tanh(X) = sgn*( z/(2+z) ).
|		Exit.
|
|	3. (|X| <= 2**(-40) or |X| >= (5/2) log2). If |X| < 1,
|		go to 7.
|
|	4. (|X| >= (5/2) log2) If |X| >= 50 log2, go to 6.
|
|	5. ((5/2) log2 <= |X| < 50 log2) Calculate tanh(X) by
|		sgn := sign(X), y := 2|X|, z := exp(Y),
|		tanh(X) = sgn - [ sgn*2/(1+z) ].
|		Exit.
|
|	6. (|X| >= 50 log2) Tanh(X) = +-1 (round to nearest). Thus, we
|		calculate Tanh(X) by
|		sgn := sign(X), Tiny := 2**(-126),
|		tanh(X) := sgn - sgn*Tiny.
|		Exit.
|
|	7. (|X| < 2**(-40)). Tanh(X) = X.	Exit.
|

|STANH	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text
	
	.include "fpsp.defs"

	.set	X,FP_SCR5
	.set	XDCARE,X+2
	.set	XFRAC,X+4

	.set	SGN,L_SCR3

	.set	V,FP_SCR6

BOUNDS1:
	.long	0x3FD78000,0x3FFFDDCE		|... 2^(-40), (5/2)LOG2

|	xref	t_frcinx
|	xref	t_extdnrm
|	xref	setox
|	xref	setoxm1

	.global	stanhd
stanhd:
|--TANH(X) = X FOR DENORMALIZED X

	bra	t_extdnrm

	.global	stanh
stanh:
	fmovex	%a0@,%FP0		|...LOAD INPUT

	fmovex	%FP0,%a6@(X)
	movel	%a0@,%d0
	movew	%a0@(4),%d0
	movel	%D0,%a6@(X)
	andl	#0x7FFFFFFF,%D0
	cmp2l	%pc@(BOUNDS1),%D0		|...2**(-40) < |X| < (5/2)LOG2 ?
	bcss	TANHBORS

|--THIS IS THE USUAL CASE
|--Y = 2|X|, Z = EXPM1(Y), TANH(X) = SIGN(X) * Z / (Z+2).

	movel	%a6@(X),%D0
	movel	%D0,%a6@(SGN)
	andl	#0x7FFF0000,%D0
	addl	#0x00010000,%D0		|...EXPONENT OF 2|X|
	movel	%D0,%a6@(X)
	andl	#0x80000000,%a6@(SGN)
	fmovex	%a6@(X),%FP0		|...%FP0 IS Y = 2|X|

	movel	%d1,%a7@-
	clrl	%d1
	fmovemx	%fp0-%fp0,%a0@
	bsr	setoxm1		|...%FP0 IS Z = EXPM1(Y)
	movel	%a7@+,%d1

	fmovex	%FP0,%FP1
	fadds	#0r2.0,%FP1		|...Z+2
	movel	%a6@(SGN),%D0
	fmovex	%FP1,%a6@(V)
	eorl	%D0,%a6@(V)

	fmovel	%d1,%FPCR		|restore users exceptions
	fdivx	%a6@(V),%FP0
	bra	t_frcinx

TANHBORS:
	cmpl	#0x3FFF8000,%D0
	blt	TANHSM

	cmpl	#0x40048AA1,%D0
	bgt	TANHHUGE

|-- (5/2) LOG2 < |X| < 50 LOG2,
|--TANH(X) = 1 - (2/[EXP(2X)+1]). LET Y = 2|X|, SGN = SIGN(X),
|--TANH(X) = SGN -	SGN*2/[EXP(Y)+1].

	movel	%a6@(X),%D0
	movel	%D0,%a6@(SGN)
	andl	#0x7FFF0000,%D0
	addl	#0x00010000,%D0		|...EXPO OF 2|X|
	movel	%D0,%a6@(X)		|...Y = 2|X|
	andl	#0x80000000,%a6@(SGN)
	movel	%a6@(SGN),%D0
	fmovex	%a6@(X),%FP0		|...Y = 2|X|

	movel	%d1,%a7@-
	clrl	%d1
	fmovemx	%fp0-%fp0,%a0@
	bsr	setox		|...%FP0 IS EXP(Y)
	movel	%a7@+,%d1
	movel	%a6@(SGN),%d0
	fadds	#0r1.0,%FP0		|...EXP(Y)+1

	eorl	#0xC0000000,%D0		|...-SIGN(X)*2
	fmoves	%d0,%FP1		|...-SIGN(X)*2 IN SGL FMT
	fdivx	%FP0,%FP1		|...-SIGN(X)2 / [EXP(Y)+1 ]

	movel	%a6@(SGN),%D0
	orl	#0x3F800000,%D0		|...SGN
	fmoves	%d0,%FP0		|...SGN IN SGL FMT

	fmovel	%d1,%FPCR		|restore users exceptions
	faddx	%fp1,%FP0

	bra	t_frcinx

TANHSM:
	clrw	%a6@(XDCARE)

	fmovel	%d1,%FPCR		|restore users exceptions
	fmovex	%a6@(X),%FP0		|last inst - possible exception set

	bra	t_frcinx

TANHHUGE:
|---RETURN SGN(X) - SGN(X)EPS
	movel	%a6@(X),%D0
	andl	#0x80000000,%D0
	orl	#0x3F800000,%D0
	fmoves	%d0,%FP0
	andl	#0x80000000,%D0
	eorl	#0x80800000,%D0		|...-SIGN(X)*EPS

	fmovel	%d1,%FPCR		|restore users exceptions
	fadds	%d0,%FP0

	bra	t_frcinx

|	end
