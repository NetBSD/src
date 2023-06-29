|	$NetBSD: srem_mod.sa,v 1.3 1994/10/26 07:49:58 cgd Exp $

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
|	srem_mod.sa 3.1 12/10/90
|
|      The entry point sMOD computes the floating point MOD of the
|      input values X and Y. The entry point sREM computes the floating
|      point (IEEE) REM of the input values X and Y.
|
|      INPUT
|      -----
|      Double-extended value Y is pointed to by address in register
|      %A0. Double-extended value X is located in %A0@(-12). The values
|      of X and Y are both nonzero and finite; although either or both
|      of them can be denormalized. The special cases of zeros, NaNs,
|      and infinities are handled elsewhere.
|
|      OUTPUT
|      ------
|      FREM(X,Y) or FMOD(X,Y), depending on entry point.
|
|       ALGORITHM
|       ---------
|
|       Step 1.  Save and strip signs of X and Y: signX := sign(X),
|                signY := sign(Y), X := |X|, Y := |Y|, 
|                signQ := signX EOR signY. Record whether MOD or REM
|                is requested.
|
|       Step 2.  Set L := expo(X)-expo(Y), k := 0, Q := 0.
|                If (L < 0) then
|                   R := X, go to Step 4.
|                else
|                   R := 2^(-L)X, j := L.
|                endif
|
|       Step 3.  Perform MOD(X,Y)
|            3.1 If R = Y, go to Step 9.
|            3.2 If R > Y, then { R := R - Y, Q := Q + 1}
|            3.3 If j = 0, go to Step 4.
|            3.4 k := k + 1, j := j - 1, Q := 2Q, R := 2R. Go to
|                Step 3.1.
|
|       Step 4.  At this point, R = X - QY = MOD(X,Y). Set
|                Last_Subtract := false (used in Step 7 below). If
|                MOD is requested, go to Step 6. 
|
|       Step 5.  R = MOD(X,Y), but REM(X,Y) is requested.
|            5.1 If R < Y/2, then R = MOD(X,Y) = REM(X,Y). Go to
|                Step 6.
|            5.2 If R > Y/2, then { set Last_Subtract := true,
|                Q := Q + 1, Y := signY*Y }. Go to Step 6.
|            5.3 This is the tricky case of R = Y/2. If Q is odd,
|                then { Q := Q + 1, signX := -signX }.
|
|       Step 6.  R := signX*R.
|
|       Step 7.  If Last_Subtract = true, R := R - Y.
|
|       Step 8.  Return signQ, last 7 bits of Q, and R as required.
|
|       Step 9.  At this point, R = 2^(-j)*X - Q Y = Y. Thus,
|                X = 2^(j)*(Q+1)Y. set Q := 2^(j)*(Q+1),
|                R := 0. Return signQ, last 7 bits of Q, and R.
|                

|SREM_MOD    IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

	.set	Mod_Flag,L_SCR3
	.set	SignY,FP_SCR3+4
	.set	SignX,FP_SCR3+8
	.set	SignQ,FP_SCR3+12
	.set	Sc_Flag,FP_SCR4

	.set	Y,FP_SCR1
	.set	Y_Hi,Y+4
	.set	Y_Lo,Y+8

	.set	R,FP_SCR2
	.set	R_Hi,R+4
	.set	R_Lo,R+8


Scale:
	.long	0x00010000,0x80000000,0x00000000,0x00000000

|	xref	t_avoid_unsupp

	.global	smod
smod:

	clrl	%a6@(Mod_Flag)
	bras	Mod_Rem

	.global	srem
srem:

	movel	#1,%a6@(Mod_Flag)

Mod_Rem:
|..Save sign of X and Y
	moveml	%D2-%D7,%A7@-		|...save data registers
	movew	%A0@,%D3
	movew	%D3,%a6@(SignY)
	andil	#0x00007FFF,%D3		|...Y := |Y|

|
	movel	%A0@(4),%D4
	movel	%A0@(8),%D5		|...(%D3,%D4,%D5) is |Y|

	tstl	%D3
	bnes	Y_Normal

	movel	#0x00003FFE,%D3		|...0x3FFD + 1
	tstl	%D4
	bnes	HiY_not0

HiY_0:
	movel	%D5,%D4
	clrl	%D5
	subil	#32,%D3
	clrl	%D6
	bfffo	%D4{#0:#32},%D6
	lsll	%D6,%D4
	subl	%D6,%D3		|...(%D3,%D4,%D5) is normalized
|                                       ...with bias 0x7FFD
	bras	Chk_X

HiY_not0:
	clrl	%D6
	bfffo	%D4{#0:#32},%D6
	subl	%D6,%D3
	lsll	%D6,%D4
	movel	%D5,%D7		|...a copy of %D5
	lsll	%D6,%D5
	negl	%D6
	addil	#32,%D6
	lsrl	%D6,%D7
	orl	%D7,%D4		|...(%D3,%D4,%D5) normalized
|                                       ...with bias 0x7FFD
	bras	Chk_X

Y_Normal:
	addil	#0x00003FFE,%D3		|...(%D3,%D4,%D5) normalized
|                                       ...with bias 0x7FFD

Chk_X:
	movew	%A0@(-12),%D0
	movew	%D0,%a6@(SignX)
	movew	%a6@(SignY),%D1
	eorl	%D0,%D1
	andil	#0x00008000,%D1
	movew	%D1,%a6@(SignQ)		|...sign(Q) obtained
	andil	#0x00007FFF,%D0
	movel	%A0@(-8),%D1
	movel	%A0@(-4),%D2		|...(%D0,%D1,%D2) is |X|
	tstl	%D0
	bnes	X_Normal
	movel	#0x00003FFE,%D0
	tstl	%D1
	bnes	HiX_not0

HiX_0:
	movel	%D2,%D1
	clrl	%D2
	subil	#32,%D0
	clrl	%D6
	bfffo	%D1{#0:#32},%D6
	lsll	%D6,%D1
	subl	%D6,%D0		|...(%D0,%D1,%D2) is normalized
|                                       ...with bias 0x7FFD
	bras	Init

HiX_not0:
	clrl	%D6
	bfffo	%D1{#0:#32},%D6
	subl	%D6,%D0
	lsll	%D6,%D1
	movel	%D2,%D7		|...a copy of %D2
	lsll	%D6,%D2
	negl	%D6
	addil	#32,%D6
	lsrl	%D6,%D7
	orl	%D7,%D1		|...(%D0,%D1,%D2) normalized
|                                       ...with bias 0x7FFD
	bras	Init

X_Normal:
	addil	#0x00003FFE,%D0		|...(%D0,%D1,%D2) normalized
|                                       ...with bias 0x7FFD

Init:
|
	movel	%D3,%a6@(L_SCR1)		|...save biased expo(Y)
	movel	%d0,%a6@(L_SCR2)		|save %d0
	subl	%D3,%D0		|...L := expo(X)-expo(Y)
|   Move.L               %D0,L            ...%D0 is j
	clrl	%D6		|...%D6 := carry <- 0
	clrl	%D3		|...%D3 is Q
	moveal	#0,%A1		|...%A1 is k| j+k=L, Q=0

|..(Carry,%D1,%D2) is R
	tstl	%D0
	bges	Mod_Loop

|..expo(X) < expo(Y). Thus X = mod(X,Y)
|
	movel	%a6@(L_SCR2),%d0		|restore %d0
	bra	Get_Mod

|..At this point  R = 2^(-L)X; Q = 0; k = 0; and  k+j = L


Mod_Loop:
	tstl	%D6		|...test carry bit
	bgts	R_GT_Y

|..At this point carry = 0, R = (%D1,%D2), Y = (%D4,%D5)
	cmpl	%D4,%D1		|...compare hi(R) and hi(Y)
	bnes	R_NE_Y
	cmpl	%D5,%D2		|...compare lo(R) and lo(Y)
	bnes	R_NE_Y

|..At this point, R = Y
	bra	Rem_is_0

R_NE_Y:
|..use the borrow of the previous compare
	bcss	R_LT_Y		|...borrow is set iff R < Y

R_GT_Y:
|..If Carry is set, then Y < (Carry,%D1,%D2) < 2Y. Otherwise, Carry = 0
|..and Y < (%D1,%D2) < 2Y. Either way, perform R - Y
	subl	%D5,%D2		|...lo(R) - lo(Y)
	subxl	%D4,%D1		|...hi(R) - hi(Y)
	clrl	%D6		|...clear carry
	addql	#1,%D3		|...Q := Q + 1

R_LT_Y:
|..At this point, Carry=0, R < Y. R = 2^(k-L)X - QY; k+j = L; j >= 0.
	tstl	%D0		|...see if j = 0.
	beqs	PostLoop

	addl	%D3,%D3		|...Q := 2Q
	addl	%D2,%D2		|...lo(R) = 2lo(R)
	addxl	%D1,%D1		|...hi(R) = 2hi(R) + carry
	scs	%D6		|...set Carry if 2(R) overflows
	addql	#1,%A1		|...k := k+1
	subql	#1,%D0		|...j := j - 1
|..At this point, R=(Carry,%D1,%D2) = 2^(k-L)X - QY, j+k=L, j >= 0, R < 2Y.

	bras	Mod_Loop

PostLoop:
|..k = L, j = 0, Carry = 0, R = (%D1,%D2) = X - QY, R < Y.

|..normalize R.
	movel	%a6@(L_SCR1),%D0		|...new biased expo of R
	tstl	%D1
	bnes	HiR_not0

HiR_0:
	movel	%D2,%D1
	clrl	%D2
	subil	#32,%D0
	clrl	%D6
	bfffo	%D1{#0:#32},%D6
	lsll	%D6,%D1
	subl	%D6,%D0		|...(%D0,%D1,%D2) is normalized
|                                       ...with bias 0x7FFD
	bras	Get_Mod

HiR_not0:
	clrl	%D6
	bfffo	%D1{#0:#32},%D6
	bmis	Get_Mod		|...already normalized
	subl	%D6,%D0
	lsll	%D6,%D1
	movel	%D2,%D7		|...a copy of %D2
	lsll	%D6,%D2
	negl	%D6
	addil	#32,%D6
	lsrl	%D6,%D7
	orl	%D7,%D1		|...(%D0,%D1,%D2) normalized

|
Get_Mod:
	cmpil	#0x000041FE,%D0
	bges	No_Scale
Do_Scale:
	movew	%D0,%a6@(R)
	clrw	%a6@(R+2)
	movel	%D1,%a6@(R_Hi)
	movel	%D2,%a6@(R_Lo)
	movel	%a6@(L_SCR1),%D6
	movew	%D6,%a6@(Y)
	clrw	%a6@(Y+2)
	movel	%D4,%a6@(Y_Hi)
	movel	%D5,%a6@(Y_Lo)
	fmovex	%a6@(R),%fp0		|...no exception
	movel	#1,%a6@(Sc_Flag)
	bras	ModOrRem
No_Scale:
	movel	%D1,%a6@(R_Hi)
	movel	%D2,%a6@(R_Lo)
	subil	#0x3FFE,%D0
	movew	%D0,%a6@(R)
	clrw	%a6@(R+2)
	movel	%a6@(L_SCR1),%D6
	subil	#0x3FFE,%D6
	movel	%D6,%a6@(L_SCR1)
	fmovex	%a6@(R),%fp0
	movew	%D6,%a6@(Y)
	movel	%D4,%a6@(Y_Hi)
	movel	%D5,%a6@(Y_Lo)
	clrl	%a6@(Sc_Flag)

|


ModOrRem:
	movel	%a6@(Mod_Flag),%D6
	beqs	Fix_Sign

	movel	%a6@(L_SCR1),%D6		|...new biased expo(Y)
	subql	#1,%D6		|...biased expo(Y/2)
	cmpl	%D6,%D0
	blts	Fix_Sign
	bgts	Last_Sub

	cmpl	%D4,%D1
	bnes	Not_EQ
	cmpl	%D5,%D2
	bnes	Not_EQ
	bra	Tie_Case

Not_EQ:
	bcss	Fix_Sign

Last_Sub:
|
	fsubx	%a6@(Y),%fp0		|...no exceptions
	addql	#1,%D3		|...Q := Q + 1

|

Fix_Sign:
|..Get sign of X
	movew	%a6@(SignX),%D6
	bges	Get_Q
	fnegx	%fp0

|..Get Q
|
Get_Q:
	clrl	%d6		|
	movew	%a6@(SignQ),%D6		|...%D6 is sign(Q)
	movel	#8,%D7
	lsrl	%D7,%D6		|
	andil	#0x0000007F,%D3		|...7 bits of Q
	orl	%D6,%D3		|...sign and bits of Q
	swap	%D3
	fmovel	%fpsr,%D6
	andil	#0xFF00FFFF,%D6
	orl	%D3,%D6
	fmovel	%D6,%fpsr		|...put Q in %fpsr

|
Restore:
	moveml	%A7@+,%D2-%D7
	fmovel	%a6@(USER_FPCR),%fpcr
	movel	%a6@(Sc_Flag),%D0
	beqs	Finish
	fmulx	%pc@(Scale),%fp0		|...may cause underflow
	bra	t_avoid_unsupp		|check for denorm as a
|					;result of the scaling

Finish:
	fmovex	%fp0,%fp0		|capture exceptions & round
	rts

Rem_is_0:
|..R = 2^(-j)X - Q Y = Y, thus R = 0 and quotient = 2^j (Q+1)
	addql	#1,%D3
	cmpil	#8,%D0		|...%D0 is j 
	bges	Q_Big

	lsll	%D0,%D3
	bras	Set_R_0

Q_Big:
	clrl	%D3

Set_R_0:
	fmoves	#0r0.0,%fp0
	clrl	%a6@(Sc_Flag)
	bra	Fix_Sign

Tie_Case:
|..Check parity of Q
	movel	%D3,%D6
	andil	#0x00000001,%D6
	tstl	%D6
	beq	Fix_Sign		|...Q is even

|..Q is odd, Q := Q + 1, signX := -signX
	addql	#1,%D3
	movew	%a6@(SignX),%D6
	eoril	#0x00008000,%D6
	movew	%D6,%a6@(SignX)
	bra	Fix_Sign

|	end
