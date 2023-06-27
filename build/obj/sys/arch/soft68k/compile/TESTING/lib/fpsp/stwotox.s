|	$NetBSD: stwotox.sa,v 1.3 1994/10/26 07:50:15 cgd Exp $

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
|	stwotox.sa 3.1 12/10/90
|
|	stwotox  --- 2**X
|	stwotoxd --- 2**X for denormalized X
|	stentox  --- 10**X
|	stentoxd --- 10**X for denormalized X
|
|	Input: Double-extended number X in location pointed to
|		by address register %a0.
|
|	Output: The function values are returned in %Fp0.
|
|	Accuracy and Monotonicity: The returned result is within 2 ulps in
|		64 significant bit, i.e. within 0.5001 ulp to 53 bits if the
|		result is subsequently rounded to double precision. The
|		result is provably monotonic in double precision.
|
|	Speed: The program stwotox takes approximately 190 cycles and the
|		program stentox takes approximately 200 cycles.
|
|	Algorithm:
|
|	twotox
|	1. If |X| > 16480, go to ExpBig.
|
|	2. If |X| < 2**(-70), go to ExpSm.
|
|	3. Decompose X as X = N/64 + r where |r| <= 1/128. Furthermore
|		decompose N as
|		 N = 64(M + M') + j,  j = 0,1,2,...,63.
|
|	4. Overwrite r := r * log2. Then
|		2**X = 2**(M') * 2**(M) * 2**(j/64) * exp(r).
|		Go to expr to compute that expression.
|
|	tentox
|	1. If |X| > 16480*log_10(2) (base 10 log of 2), go to ExpBig.
|
|	2. If |X| < 2**(-70), go to ExpSm.
|
|	3. Set y := X*log_2(10)*64 (base 2 log of 10). Set
|		N := round-to-int(y). Decompose N as
|		 N = 64(M + M') + j,  j = 0,1,2,...,63.
|
|	4. Define r as
|		r := ((X - N*L1)-N*L2) * L10
|		where L1, L2 are the leading and trailing parts of log_10(2)/64
|		and L10 is the natural log of 10. Then
|		10**X = 2**(M') * 2**(M) * 2**(j/64) * exp(r).
|		Go to expr to compute that expression.
|
|	expr
|	1. Fetch 2**(j/64) from table as Fact1 and Fact2.
|
|	2. Overwrite Fact1 and Fact2 by
|		Fact1 := 2**(M) * Fact1
|		Fact2 := 2**(M) * Fact2
|		Thus Fact1 + Fact2 = 2**(M) * 2**(j/64).
|
|	3. Calculate P where 1 + P approximates exp(r):
|		P = r + r*r*(%A1+r*(%A2+...+r*%A5)).
|
|	4. Let AdjFact := 2**(M'). Return
|		AdjFact * ( Fact1 + ((Fact1*P) + Fact2) ).
|		Exit.
|
|	ExpBig
|	1. Generate overflow by Huge * Huge if X > 0; otherwise, generate
|		underflow by Tiny * Tiny.
|
|	ExpSm
|	1. Return 1 + X.
|

|STWOTOX	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

BOUNDS1:
	.long	0x3FB98000,0x400D80C0		|... 2^(-70),16480
BOUNDS2:
	.long	0x3FB98000,0x400B9B07		|... 2^(-70),16480 LOG2/LOG10

L2TEN64:
	.long	0x406A934F,0x0979A371		|... 64LOG10/LOG2
L10TWO1:
	.long	0x3F734413,0x509F8000		|... LOG2/64LOG10

L10TWO2:
	.long	0xBFCD0000,0xC0219DC1,0xDA994FD2,0x00000000

LOG10:
	.long	0x40000000,0x935D8DDD,0xAAA8AC17,0x00000000

LOG2:
	.long	0x3FFE0000,0xB17217F7,0xD1CF79AC,0x00000000

EXPA5:
	.long	0x3F56C16D,0x6F7BD0B2
EXPA4:
	.long	0x3F811112,0x302C712C
EXPA3:
	.long	0x3FA55555,0x55554CC1
EXPA2:
	.long	0x3FC55555,0x55554A54
EXPA1:
	.long	0x3FE00000,0x00000000,0x00000000,0x00000000

HUGE:
	.long	0x7FFE0000,0xFFFFFFFF,0xFFFFFFFF,0x00000000
TINY:
	.long	0x00010000,0xFFFFFFFF,0xFFFFFFFF,0x00000000

EXPTBL:
	.long	0x3FFF0000,0x80000000,0x00000000,0x3F738000
	.long	0x3FFF0000,0x8164D1F3,0xBC030773,0x3FBEF7CA
	.long	0x3FFF0000,0x82CD8698,0xAC2BA1D7,0x3FBDF8A9
	.long	0x3FFF0000,0x843A28C3,0xACDE4046,0x3FBCD7C9
	.long	0x3FFF0000,0x85AAC367,0xCC487B15,0xBFBDE8DA
	.long	0x3FFF0000,0x871F6196,0x9E8D1010,0x3FBDE85C
	.long	0x3FFF0000,0x88980E80,0x92DA8527,0x3FBEBBF1
	.long	0x3FFF0000,0x8A14D575,0x496EFD9A,0x3FBB80CA
	.long	0x3FFF0000,0x8B95C1E3,0xEA8BD6E7,0xBFBA8373
	.long	0x3FFF0000,0x8D1ADF5B,0x7E5BA9E6,0xBFBE9670
	.long	0x3FFF0000,0x8EA4398B,0x45CD53C0,0x3FBDB700
	.long	0x3FFF0000,0x9031DC43,0x1466B1DC,0x3FBEEEB0
	.long	0x3FFF0000,0x91C3D373,0xAB11C336,0x3FBBFD6D
	.long	0x3FFF0000,0x935A2B2F,0x13E6E92C,0xBFBDB319
	.long	0x3FFF0000,0x94F4EFA8,0xFEF70961,0x3FBDBA2B
	.long	0x3FFF0000,0x96942D37,0x20185A00,0x3FBE91D5
	.long	0x3FFF0000,0x9837F051,0x8DB8A96F,0x3FBE8D5A
	.long	0x3FFF0000,0x99E04593,0x20B7FA65,0xBFBCDE7B
	.long	0x3FFF0000,0x9B8D39B9,0xD54E5539,0xBFBEBAAF
	.long	0x3FFF0000,0x9D3ED9A7,0x2CFFB751,0xBFBD86DA
	.long	0x3FFF0000,0x9EF53260,0x91A111AE,0xBFBEBEDD
	.long	0x3FFF0000,0xA0B0510F,0xB9714FC2,0x3FBCC96E
	.long	0x3FFF0000,0xA2704303,0x0C496819,0xBFBEC90B
	.long	0x3FFF0000,0xA43515AE,0x09E6809E,0x3FBBD1DB
	.long	0x3FFF0000,0xA5FED6A9,0xB15138EA,0x3FBCE5EB
	.long	0x3FFF0000,0xA7CD93B4,0xE965356A,0xBFBEC274
	.long	0x3FFF0000,0xA9A15AB4,0xEA7C0EF8,0x3FBEA83C
	.long	0x3FFF0000,0xAB7A39B5,0xA93ED337,0x3FBECB00
	.long	0x3FFF0000,0xAD583EEA,0x42A14AC6,0x3FBE9301
	.long	0x3FFF0000,0xAF3B78AD,0x690A4375,0xBFBD8367
	.long	0x3FFF0000,0xB123F581,0xD2AC2590,0xBFBEF05F
	.long	0x3FFF0000,0xB311C412,0xA9112489,0x3FBDFB3C
	.long	0x3FFF0000,0xB504F333,0xF9DE6484,0x3FBEB2FB
	.long	0x3FFF0000,0xB6FD91E3,0x28D17791,0x3FBAE2CB
	.long	0x3FFF0000,0xB8FBAF47,0x62FB9EE9,0x3FBCDC3C
	.long	0x3FFF0000,0xBAFF5AB2,0x133E45FB,0x3FBEE9AA
	.long	0x3FFF0000,0xBD08A39F,0x580C36BF,0xBFBEAEFD
	.long	0x3FFF0000,0xBF1799B6,0x7A731083,0xBFBCBF51
	.long	0x3FFF0000,0xC12C4CCA,0x66709456,0x3FBEF88A
	.long	0x3FFF0000,0xC346CCDA,0x24976407,0x3FBD83B2
	.long	0x3FFF0000,0xC5672A11,0x5506DADD,0x3FBDF8AB
	.long	0x3FFF0000,0xC78D74C8,0xABB9B15D,0xBFBDFB17
	.long	0x3FFF0000,0xC9B9BD86,0x6E2F27A3,0xBFBEFE3C
	.long	0x3FFF0000,0xCBEC14FE,0xF2727C5D,0xBFBBB6F8
	.long	0x3FFF0000,0xCE248C15,0x1F8480E4,0xBFBCEE53
	.long	0x3FFF0000,0xD06333DA,0xEF2B2595,0xBFBDA4AE
	.long	0x3FFF0000,0xD2A81D91,0xF12AE45A,0x3FBC9124
	.long	0x3FFF0000,0xD4F35AAB,0xCFEDFA1F,0x3FBEB243
	.long	0x3FFF0000,0xD744FCCA,0xD69D6AF4,0x3FBDE69A
	.long	0x3FFF0000,0xD99D15C2,0x78AFD7B6,0xBFB8BC61
	.long	0x3FFF0000,0xDBFBB797,0xDAF23755,0x3FBDF610
	.long	0x3FFF0000,0xDE60F482,0x5E0E9124,0xBFBD8BE1
	.long	0x3FFF0000,0xE0CCDEEC,0x2A94E111,0x3FBACB12
	.long	0x3FFF0000,0xE33F8972,0xBE8A5A51,0x3FBB9BFE
	.long	0x3FFF0000,0xE5B906E7,0x7C8348A8,0x3FBCF2F4
	.long	0x3FFF0000,0xE8396A50,0x3C4BDC68,0x3FBEF22F
	.long	0x3FFF0000,0xEAC0C6E7,0xDD24392F,0xBFBDBF4A
	.long	0x3FFF0000,0xED4F301E,0xD9942B84,0x3FBEC01A
	.long	0x3FFF0000,0xEFE4B99B,0xDCDAF5CB,0x3FBE8CAC
	.long	0x3FFF0000,0xF281773C,0x59FFB13A,0xBFBCBB3F
	.long	0x3FFF0000,0xF5257D15,0x2486CC2C,0x3FBEF73A
	.long	0x3FFF0000,0xF7D0DF73,0x0AD13BB9,0xBFB8B795
	.long	0x3FFF0000,0xFA83B2DB,0x722A033A,0x3FBEF84B
	.long	0x3FFF0000,0xFD3E0C0C,0xF486C175,0xBFBEF581

	.set	N,L_SCR1

	.set	X,FP_SCR1
	.set	XDCARE,X+2
	.set	XFRAC,X+4

	.set	ADJFACT,FP_SCR2

	.set	FACT1,FP_SCR3
	.set	FACT1HI,FACT1+4
	.set	FACT1LOW,FACT1+8

	.set	FACT2,FP_SCR4
	.set	FACT2HI,FACT2+4
	.set	FACT2LOW,FACT2+8

|	xref	t_unfl
|	xref	t_ovfl
|	xref	t_frcinx

	.global	stwotoxd
stwotoxd:
|--ENTRY POINT FOR 2**(X) FOR DENORMALIZED ARGUMENT

	fmovel	%d1,%fpcr		|...set user's rounding mode/precision
	fmoves	#0r1.0,%FP0		|...RETURN 1 + X
	movel	%a0@,%d0
	orl	#0x00800001,%d0
	fadds	%d0,%fp0
	bra	t_frcinx

	.global	stwotox
stwotox:
|--ENTRY POINT FOR 2**(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
	fmovemx	%a0@,%FP0-%FP0		|...LOAD INPUT, do not set cc's

	movel	%A0@,%D0
	movew	%A0@(4),%D0
	fmovex	%FP0,%a6@(X)
	andil	#0x7FFFFFFF,%D0

	cmpil	#0x3FB98000,%D0		|...|X| >= 2**(-70)?
	bges	TWOOK1
	bra	EXPBORS

TWOOK1:
	cmpil	#0x400D80C0,%D0		|...|X| > 16480?
	bles	TWOMAIN
	bra	EXPBORS
	

TWOMAIN:
|--USUAL CASE, 2^(-70) <= |X| <= 16480

	fmovex	%FP0,%FP1
	fmuls	#0r6.40e+01,%FP1		|...64 * X
	
	fmovel	%FP1,%a6@(N)		|...N = ROUND-TO-INT(64 X)
	movel	%d2,%sp@-
	lea	EXPTBL,%a1		|...LOAD ADDRESS OF TABLE OF 2^(J/64)
	fmovel	%a6@(N),%FP1		|...N --> FLOATING FMT
	movel	%a6@(N),%D0
	movel	%D0,%d2
	andil	#0x3F,%D0		|...%D0 IS J
	asll	#4,%D0		|...DISPLACEMENT FOR 2^(J/64)
	addal	%D0,%a1		|...ADDRESS FOR 2^(J/64)
	asrl	#6,%d2		|...%d2 IS L, N = 64L + J
	movel	%d2,%D0
	asrl	#1,%D0		|...%D0 IS M
	subl	%D0,%d2		|...%d2 IS M', N = 64(M+M') + J
	addil	#0x3FFF,%d2
	movew	%d2,%a6@(ADJFACT)		|...ADJFACT IS 2^(M')
	movel	%sp@+,%d2
|--SUMMARY: %a1 IS ADDRESS FOR THE LEADING PORTION OF 2^(J/64),
|--%D0 IS M WHERE N = 64(M+M') + J. NOTE THAT |M| <= 16140 BY DESIGN.
|--ADJFACT = 2^(M').
|--REGISTERS SAVED SO FAR ARE (IN ORDER) %FPCR, %D0, %FP1, %a1, AND %FP2.

	fmuls	#0r1.56250e-02,%FP1		|...(1/64)*N
	movel	%a1@+,%a6@(FACT1)
	movel	%a1@+,%a6@(FACT1HI)
	movel	%a1@+,%a6@(FACT1LOW)
	movew	%a1@+,%a6@(FACT2)
	clrw	%a6@(FACT2+2)

	fsubx	%FP1,%FP0		|...X - (1/64)*INT(64 X)

	movew	%a1@+,%a6@(FACT2HI)
	clrw	%a6@(FACT2HI+2)
	clrl	%a6@(FACT2LOW)
	addw	%D0,%a6@(FACT1)
	
	fmulx	LOG2,%FP0		|...%FP0 IS R
	addw	%D0,%a6@(FACT2)

	bra	expr

EXPBORS:
|--%FPCR, %D0 SAVED
	cmpil	#0x3FFF8000,%D0
	bgts	EXPBIG

EXPSM:
|--|X| IS SMALL, RETURN 1 + X

	fmovel	%d1,%FPCR		|restore users exceptions
	fadds	#0r1.0,%FP0		|...RETURN 1 + X

	bra	t_frcinx

EXPBIG:
|--|X| IS LARGE, GENERATE OVERFLOW IF X > 0; ELSE GENERATE UNDERFLOW
|--REGISTERS SAVE SO FAR ARE %FPCR AND  %D0
	movel	%a6@(X),%D0
	tstl	%D0
	blts	EXPNEG

	bclr	#7,%a0@		|t_ovfl expects positive value
	bra	t_ovfl

EXPNEG:
	bclr	#7,%a0@		|t_unfl expects positive value
	bra	t_unfl

	.global	stentoxd
stentoxd:
|--ENTRY POINT FOR 10**(X) FOR DENORMALIZED ARGUMENT

	fmovel	%d1,%fpcr		|...set user's rounding mode/precision
	fmoves	#0r1.0,%FP0		|...RETURN 1 + X
	movel	%a0@,%d0
	orl	#0x00800001,%d0
	fadds	%d0,%fp0
	bra	t_frcinx

	.global	stentox
stentox:
|--ENTRY POINT FOR 10**(X), HERE X IS FINITE, NON-ZERO, AND NOT NAN'S
	fmovemx	%a0@,%FP0-%FP0		|...LOAD INPUT, do not set cc's

	movel	%A0@,%D0
	movew	%A0@(4),%D0
	fmovex	%FP0,%a6@(X)
	andil	#0x7FFFFFFF,%D0

	cmpil	#0x3FB98000,%D0		|...|X| >= 2**(-70)?
	bges	TENOK1
	bra	EXPBORS

TENOK1:
	cmpil	#0x400B9B07,%D0		|...|X| <= 16480*log2/log10 ?
	bles	TENMAIN
	bra	EXPBORS

TENMAIN:
|--USUAL CASE, 2^(-70) <= |X| <= 16480 LOG 2 / LOG 10

	fmovex	%FP0,%FP1
	fmuld	L2TEN64,%FP1		|...X*64*LOG10/LOG2
	
	fmovel	%FP1,%a6@(N)		|...N=INT(X*64*LOG10/LOG2)
	movel	%d2,%sp@-
	lea	EXPTBL,%a1		|...LOAD ADDRESS OF TABLE OF 2^(J/64)
	fmovel	%a6@(N),%FP1		|...N --> FLOATING FMT
	movel	%a6@(N),%D0
	movel	%D0,%d2
	andil	#0x3F,%D0		|...%D0 IS J
	asll	#4,%D0		|...DISPLACEMENT FOR 2^(J/64)
	addal	%D0,%a1		|...ADDRESS FOR 2^(J/64)
	asrl	#6,%d2		|...%d2 IS L, N = 64L + J
	movel	%d2,%D0
	asrl	#1,%D0		|...%D0 IS M
	subl	%D0,%d2		|...%d2 IS M', N = 64(M+M') + J
	addil	#0x3FFF,%d2
	movew	%d2,%a6@(ADJFACT)		|...ADJFACT IS 2^(M')
	movel	%sp@+,%d2

|--SUMMARY: %a1 IS ADDRESS FOR THE LEADING PORTION OF 2^(J/64),
|--%D0 IS M WHERE N = 64(M+M') + J. NOTE THAT |M| <= 16140 BY DESIGN.
|--ADJFACT = 2^(M').
|--REGISTERS SAVED SO FAR ARE (IN ORDER) %FPCR, %D0, %FP1, %a1, AND %FP2.

	fmovex	%FP1,%FP2

	fmuld	L10TWO1,%FP1		|...N*(LOG2/64LOG10)_LEAD
	movel	%a1@+,%a6@(FACT1)

	fmulx	L10TWO2,%FP2		|...N*(LOG2/64LOG10)_TRAIL

	movel	%a1@+,%a6@(FACT1HI)
	movel	%a1@+,%a6@(FACT1LOW)
	fsubx	%FP1,%FP0		|...X - N L_LEAD
	movew	%a1@+,%a6@(FACT2)

	fsubx	%FP2,%FP0		|...X - N L_TRAIL

	clrw	%a6@(FACT2+2)
	movew	%a1@+,%a6@(FACT2HI)
	clrw	%a6@(FACT2HI+2)
	clrl	%a6@(FACT2LOW)

	fmulx	LOG10,%FP0		|...%FP0 IS R
	
	addw	%D0,%a6@(FACT1)
	addw	%D0,%a6@(FACT2)

expr:
|--%FPCR, %FP2, %FP3 ARE SAVED IN ORDER AS SHOWN.
|--ADJFACT CONTAINS 2**(M'), FACT1 + FACT2 = 2**(M) * 2**(J/64).
|--%FP0 IS R. THE FOLLOWING CODE COMPUTES
|--	2**(M'+M) * 2**(J/64) * EXP(R)

	fmovex	%FP0,%FP1
	fmulx	%FP1,%FP1		|...%FP1 IS S = R*R

	fmoved	EXPA5,%FP2		|...%FP2 IS %A5
	fmoved	EXPA4,%FP3		|...%FP3 IS %A4

	fmulx	%FP1,%FP2		|...%FP2 IS S*%A5
	fmulx	%FP1,%FP3		|...%FP3 IS S*%A4

	faddd	EXPA3,%FP2		|...%FP2 IS %A3+S*%A5
	faddd	EXPA2,%FP3		|...%FP3 IS %A2+S*%A4

	fmulx	%FP1,%FP2		|...%FP2 IS S*(%A3+S*%A5)
	fmulx	%FP1,%FP3		|...%FP3 IS S*(%A2+S*%A4)

	faddd	EXPA1,%FP2		|...%FP2 IS %A1+S*(%A3+S*%A5)
	fmulx	%FP0,%FP3		|...%FP3 IS R*S*(%A2+S*%A4)

	fmulx	%FP1,%FP2		|...%FP2 IS S*(%A1+S*(%A3+S*%A5))
	faddx	%FP3,%FP0		|...%FP0 IS R+R*S*(%A2+S*%A4)
	
	faddx	%FP2,%FP0		|...%FP0 IS EXP(R) - 1
	

|--FINAL RECONSTRUCTION PROCESS
|--EXP(X) = 2^M*2^(J/64) + 2^M*2^(J/64)*(EXP(R)-1)  -  (1 OR 0)

	fmulx	%a6@(FACT1),%FP0
	faddx	%a6@(FACT2),%FP0
	faddx	%a6@(FACT1),%FP0

	fmovel	%d1,%FPCR		|restore users exceptions
	clrw	%a6@(ADJFACT+2)
	movel	#0x80000000,%a6@(ADJFACT+4)
	clrl	%a6@(ADJFACT+8)
	fmulx	%a6@(ADJFACT),%FP0		|...FINAL ADJUSTMENT

	bra	t_frcinx

|	end
