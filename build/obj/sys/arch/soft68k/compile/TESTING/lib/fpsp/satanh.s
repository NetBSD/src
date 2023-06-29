|	$NetBSD: satanh.sa,v 1.2 1994/10/26 07:49:33 cgd Exp $

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
|	satanh.sa 3.3 12/19/90
|
|	The entry point satanh computes the inverse
|	hyperbolic tangent of
|	an input argument; satanhd does the same except for denormalized
|	input.
|
|	Input: Double-extended number X in location pointed to
|		by address register %a0.
|
|	Output: The value arctanh(X) returned in floating-point register %Fp0.
|
|	Accuracy and Monotonicity: The returned result is within 3 ulps in
|		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|		result is subsequently rounded to double precision. The 
|		result is provably monotonic in double precision.
|
|	Speed: The program satanh takes approximately 270 cycles.
|
|	Algorithm:
|
|	ATANH
|	1. If |X| >= 1, go to 3.
|
|	2. (|X| < 1) Calculate atanh(X) by
|		sgn := sign(X)
|		y := |X|
|		z := 2y/(1-y)
|		atanh(X) := sgn * (1/2) * logp1(z)
|		Exit.
|
|	3. If |X| > 1, go to 5.
|
|	4. (|X| = 1) Generate infinity with an appropriate sign and
|		divide-by-zero by	
|		sgn := sign(X)
|		atan(X) := sgn / (+0).
|		Exit.
|
|	5. (|X| > 1) Generate an invalid operation by 0 * infinity.
|		Exit.
|

|satanh	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

|	xref	t_dz
|	xref	t_operr
|	xref	t_frcinx
|	xref	t_extdnrm
|	xref	slognp1

	.global	satanhd
satanhd:
|--ATANH(X) = X FOR DENORMALIZED X

	bra	t_extdnrm

	.global	satanh
satanh:
	movel	%a0@,%d0
	movew	%a0@(4),%d0
	andil	#0x7FFFFFFF,%D0
	cmpil	#0x3FFF8000,%D0
	bges	ATANHBIG

|--THIS IS THE USUAL CASE, |X| < 1
|--Y = |X|, Z = 2Y/(1-Y), ATANH(X) = SIGN(X) * (1/2) * LOG1P(Z).

	fabsx	%a0@,%FP0		|...Y = |X|
	fmovex	%FP0,%FP1
	fnegx	%FP1		|...-Y
	faddx	%FP0,%FP0		|...2Y
	fadds	#0r1.0,%FP1		|...1-Y
	fdivx	%FP1,%FP0		|...2Y/(1-Y)
	movel	%a0@,%d0
	andil	#0x80000000,%D0
	oril	#0x3F000000,%D0		|...SIGN(X)*HALF
	movel	%d0,%sp@-

	fmovemx	%fp0-%fp0,%a0@		|...overwrite input
	movel	%d1,%sp@-
	clrl	%d1
	bsr	slognp1		|...LOG1P(Z)
	fmovel	%sp@+,%fpcr
	fmuls	%sp@+,%FP0
	bra	t_frcinx

ATANHBIG:
	fabsx	%a0@,%FP0		|...|X|
	fcmps	#0r1.0,%FP0
	fbgt	t_operr
	bra	t_dz

|	end
