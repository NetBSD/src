|	$NetBSD: ssinh.sa,v 1.3 1994/10/26 07:50:05 cgd Exp $

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
|	ssinh.sa 3.1 12/10/90
|
|       The entry point sSinh computes the hyperbolic sine of
|       an input argument; sSinhd does the same except for denormalized
|       input.
|
|       Input: Double-extended number X in location pointed to 
|		by address register %a0.
|
|       Output: The value sinh(X) returned in floating-point register %Fp0.
|
|       Accuracy and Monotonicity: The returned result is within 3 ulps in
|               64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|               result is subsequently rounded to double precision. The
|               result is provably monotonic in double precision.
|
|       Speed: The program sSINH takes approximately 280 cycles.
|
|       Algorithm:
|
|       SINH
|       1. If |X| > 16380 log2, go to 3.
|
|       2. (|X| <= 16380 log2) Sinh(X) is obtained by the formulae
|               y = |X|, sgn = sign(X), and z = expm1(Y),
|               sinh(X) = sgn*(1/2)*( z + z/(1+z) ).
|          Exit.
|
|       3. If |X| > 16480 log2, go to 5.
|
|       4. (16380 log2 < |X| <= 16480 log2)
|               sinh(X) = sign(X) * exp(|X|)/2.
|          However, invoking exp(|X|) may cause premature overflow.
|          Thus, we calculate sinh(X) as follows:
|             Y       := |X|
|             sgn     := sign(X)
|             sgnFact := sgn * 2**(16380)
|             Y'      := Y - 16381 log2
|             sinh(X) := sgnFact * exp(Y').
|          Exit.
|
|       5. (|X| > 16480 log2) sinh(X) must overflow. Return
|          sign(X)*Huge*Huge to generate overflow and an infinity with
|          the appropriate sign. Huge is the largest finite number in
|          extended format. Exit.
|

|SSINH	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

T1:
	.long	0x40C62D38,0xD3D64634		|... 16381 LOG2 LEAD
T2:
	.long	0x3D6F90AE,0xB1E75CC7		|... 16381 LOG2 TRAIL

|	xref	t_frcinx
|	xref	t_ovfl
|	xref	t_extdnrm
|	xref	setox
|	xref	setoxm1

	.global	ssinhd
ssinhd:
|--SINH(X) = X FOR DENORMALIZED X

	bra	t_extdnrm

	.global	ssinh
ssinh:
	fmovex	%a0@,%FP0		|...LOAD INPUT

	movel	%a0@,%d0
	movew	%a0@(4),%d0
	movel	%d0,%a1		|save a copy of original (compacted) operand
	andl	#0x7FFFFFFF,%D0
	cmpl	#0x400CB167,%D0
	bgts	SINHBIG

|--THIS IS THE USUAL CASE, |X| < 16380 LOG2
|--Y = |X|, Z = EXPM1(Y), SINH(X) = SIGN(X)*(1/2)*( Z + Z/(1+Z) )

	fabsx	%FP0		|...Y = |X|

	moveml	%a1/%d1,%sp@-
	fmovemx	%fp0-%fp0,%a0@
	clrl	%d1
	bsr	setoxm1		|...%FP0 IS Z = EXPM1(Y)
	fmovel	#0,%fpcr
	moveml	%sp@+,%a1/%d1

	fmovex	%FP0,%FP1
	fadds	#0r1.0,%FP1		|...1+Z
	fmovex	%FP0,%sp@-
	fdivx	%FP1,%FP0		|...Z/(1+Z)
	movel	%a1,%d0
	andl	#0x80000000,%D0
	orl	#0x3F000000,%D0
	faddx	%sp@+,%FP0
	movel	%D0,%sp@-

	fmovel	%d1,%fpcr
	fmuls	%sp@+,%fp0		|last fp inst - possible exceptions set

	bra	t_frcinx

SINHBIG:
	cmpl	#0x400CB2B3,%D0
	bgt	t_ovfl
	fabsx	%FP0
	fsubd	%pc@(T1),%FP0		|...(|X|-16381LOG2_LEAD)
	clrl	%sp@-
	movel	#0x80000000,%sp@-
	movel	%a1,%d0
	andl	#0x80000000,%D0
	orl	#0x7FFB0000,%D0
	movel	%D0,%sp@-		|...EXTENDED FMT
	fsubd	%pc@(T2),%FP0		|...|X| - 16381 LOG2, ACCURATE

	movel	%d1,%sp@-
	clrl	%d1
	fmovemx	%fp0-%fp0,%a0@
	bsr	setox
	fmovel	%sp@+,%fpcr

	fmulx	%sp@+,%fp0		|possible exception
	bra	t_frcinx

|	end
