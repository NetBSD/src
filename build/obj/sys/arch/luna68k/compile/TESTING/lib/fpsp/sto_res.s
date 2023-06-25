|	$NetBSD: sto_res.sa,v 1.4 2000/03/13 23:52:32 soren Exp $

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
|	sto_res.sa 3.1 12/10/90
|
|	Takes the result and puts it in where the user expects it.
|	Library functions return result in %fp0.	If %fp0 is not the
|	users destination register then %fp0 is moved to the
|	correct floating-point destination register.  %fp0 and %fp1
|	are then restored to the original contents. 
|
|	Input:	result in %fp0,%fp1 
|
|		%d2 & %a0 should be kept unmodified
|
|	Output:	moves the result to the true destination reg or mem
|
|	Modifies: destination floating point register
|

|STO_RES	IDNT	2,1 Motorola 040 Floating Point Software Package


	.text

	.include "fpsp.defs"

	.global	sto_cos
sto_cos:
	bfextu	%a6@(CMDREG1B){#13:#3},%d0		|extract cos destination
	cmpib	#3,%d0		|check for %fp0/%fp1 cases
	bles	c_fp0123
	fmovemx	%fp1-%fp1,%a7@-
	moveql	#7,%d1
	subl	%d0,%d1		|%d1 = 7- (dest. reg. no.)
	clrl	%d0
	bset	%d1,%d0		|%d0 is dynamic register mask
	fmovemx	%a7@+,%d0
	rts
c_fp0123:
	tstb	%d0
	beqs	c_is_fp0
	cmpib	#1,%d0
	beqs	c_is_fp1
	cmpib	#2,%d0
	beqs	c_is_fp2
c_is_fp3:
	fmovemx	%fp1-%fp1,%a6@(USER_FP3)
	rts
c_is_fp2:
	fmovemx	%fp1-%fp1,%a6@(USER_FP2)
	rts
c_is_fp1:
	fmovemx	%fp1-%fp1,%a6@(USER_FP1)
	rts
c_is_fp0:
	fmovemx	%fp1-%fp1,%a6@(USER_FP0)
	rts


	.global	sto_res
sto_res:
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|extract destination register
	cmpib	#3,%d0		|check for %fp0/%fp1 cases
	bles	fp0123
	fmovemx	%fp0-%fp0,%a7@-
	moveql	#7,%d1
	subl	%d0,%d1		|%d1 = 7- (dest. reg. no.)
	clrl	%d0
	bset	%d1,%d0		|%d0 is dynamic register mask
	fmovemx	%a7@+,%d0
	rts
fp0123:
	tstb	%d0
	beqs	is_fp0
	cmpib	#1,%d0
	beqs	is_fp1
	cmpib	#2,%d0
	beqs	is_fp2
is_fp3:
	fmovemx	%fp0-%fp0,%a6@(USER_FP3)
	rts
is_fp2:
	fmovemx	%fp0-%fp0,%a6@(USER_FP2)
	rts
is_fp1:
	fmovemx	%fp0-%fp0,%a6@(USER_FP1)
	rts
is_fp0:
	fmovemx	%fp0-%fp0,%a6@(USER_FP0)
	rts

|	end
