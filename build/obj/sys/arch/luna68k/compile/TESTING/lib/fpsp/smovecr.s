|	$NetBSD: smovecr.sa,v 1.2 1994/10/26 07:49:57 cgd Exp $

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
|	smovecr.sa 3.1 12/10/90
|
|	The entry point sMOVECR returns the constant at the
|	offset given in the instruction field.
|
|	Input: An offset in the instruction word.
|
|	Output:	The constant rounded to the user's rounding
|		mode unchecked for overflow.
|
|	Modified: %fp0.
|

|SMOVECR	IDNT	2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

|	xref	nrm_set
|	xref	round
|	xref	PIRN
|	xref	PIRZRM
|	xref	PIRP
|	xref	SMALRN
|	xref	SMALRZRM
|	xref	SMALRP
|	xref	BIGRN
|	xref	BIGRZRM
|	xref	BIGRP

FZERO:
	.long	00000000
|
|	FMOVECR 
|
	.global	smovcr
smovcr:
	bfextu	%a6@(CMDREG1B){#9:#7},%d0		|get offset
	bfextu	%a6@(USER_FPCR){#26:#2},%d1		|get rmode
|
| check range of offset
|
	tstb	%d0		|if zero, offset is to pi
	beqs	PI_TBL		|it is pi
	cmpib	#0x0a,%d0		|check range 0x01 - 0x0a
	bles	Z_VAL		|if in this range, return zero
	cmpib	#0x0e,%d0		|check range 0x0b - 0x0e
	bles	SM_TBL		|valid constants in this range
	cmpib	#0x2f,%d0		|check range 0x10 - 0x2f
	bles	Z_VAL		|if in this range, return zero 
	cmpib	#0x3f,%d0		|check range 0x30 - 0x3f
	ble	BG_TBL		|valid constants in this range
Z_VAL:
	fmoves	FZERO,%fp0
	rts
PI_TBL:
	tstb	%d1		|offset is zero, check for rmode
	beqs	PI_RN		|if zero, rn mode
	cmpib	#0x3,%d1		|check for rp
	beqs	PI_RP		|if 3, rp mode
PI_RZRM:
	lea	PIRZRM,%a0		|rmode is rz or rm, load PIRZRM in %a0
	bra	set_finx
PI_RN:
	lea	PIRN,%a0		|rmode is rn, load PIRN in %a0
	bra	set_finx
PI_RP:
	lea	PIRP,%a0		|rmode is rp, load PIRP in %a0
	bra	set_finx
SM_TBL:
	subil	#0xb,%d0		|make offset in 0 - 4 range
	tstb	%d1		|check for rmode
	beqs	SM_RN		|if zero, rn mode
	cmpib	#0x3,%d1		|check for rp
	beqs	SM_RP		|if 3, rp mode
SM_RZRM:
	lea	SMALRZRM,%a0		|rmode is rz or rm, load SMRZRM in %a0
	cmpib	#0x2,%d0		|check if result is inex
	ble	set_finx		|if 0 - 2, it is inexact
	bra	no_finx		|if 3, it is exact
SM_RN:
	lea	SMALRN,%a0		|rmode is rn, load SMRN in %a0
	cmpib	#0x2,%d0		|check if result is inex
	ble	set_finx		|if 0 - 2, it is inexact
	bra	no_finx		|if 3, it is exact
SM_RP:
	lea	SMALRP,%a0		|rmode is rp, load SMRP in %a0
	cmpib	#0x2,%d0		|check if result is inex
	ble	set_finx		|if 0 - 2, it is inexact
	bra	no_finx		|if 3, it is exact
BG_TBL:
	subil	#0x30,%d0		|make offset in 0 - f range
	tstb	%d1		|check for rmode
	beqs	BG_RN		|if zero, rn mode
	cmpib	#0x3,%d1		|check for rp
	beqs	BG_RP		|if 3, rp mode
BG_RZRM:
	lea	BIGRZRM,%a0		|rmode is rz or rm, load BGRZRM in %a0
	cmpib	#0x1,%d0		|check if result is inex
	ble	set_finx		|if 0 - 1, it is inexact
	cmpib	#0x7,%d0		|second check
	ble	no_finx		|if 0 - 7, it is exact
	bra	set_finx		|if 8 - f, it is inexact
BG_RN:
	lea	BIGRN,%a0		|rmode is rn, load BGRN in %a0
	cmpib	#0x1,%d0		|check if result is inex
	ble	set_finx		|if 0 - 1, it is inexact
	cmpib	#0x7,%d0		|second check
	ble	no_finx		|if 0 - 7, it is exact
	bra	set_finx		|if 8 - f, it is inexact
BG_RP:
	lea	BIGRP,%a0		|rmode is rp, load SMRP in %a0
	cmpib	#0x1,%d0		|check if result is inex
	ble	set_finx		|if 0 - 1, it is inexact
	cmpib	#0x7,%d0		|second check
	ble	no_finx		|if 0 - 7, it is exact
|	bra	set_finx	;if 8 - f, it is inexact
set_finx:
	orl	#inx2a_mask,%a6@(USER_FPSR)		|set inex2/ainex
no_finx:
	mulul	#12,%d0		|use offset to point into tables
	movel	%d1,%a6@(L_SCR1)		|load mode for round call
	bfextu	%a6@(USER_FPCR){#24:#2},%d1		|get precision
	tstl	%d1		|check if extended precision
|
| Precision is extended
|
	bnes	not_ext		|if extended, do not call round
	fmovemx	%a0@(%d0),%fp0-%fp0		|return result in %fp0
	rts
|
| Precision is single or double
|
not_ext:
	swap	%d1		|rnd prec in upper word of %d1
	addl	%a6@(L_SCR1),%d1		|merge rmode in low word of %d1
	movel	%a0@(%d0),%a6@(FP_SCR1)		|load first word to temp storage
	movel	%a0@(4,%d0),%a6@(FP_SCR1+4)		|load second word
	movel	%a0@(8,%d0),%a6@(FP_SCR1+8)		|load third word
	clrl	%d0		|clear g,r,s
	lea	%a6@(FP_SCR1),%a0
	btst	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|convert to internal ext. format
	
	bsr	round		|go round the mantissa

	bfclr	%a0@(LOCAL_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	fin_fcr
	bset	#sign_bit,%a0@(LOCAL_EX)
fin_fcr:
	fmovemx	%a0@,%fp0-%fp0
	rts

|	end
