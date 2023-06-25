|	$NetBSD: res_func.sa,v 1.6 2021/12/07 21:37:36 andvar Exp $

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
|	res_func.sa 3.9 7/29/91
|
| Normalizes denormalized numbers if necessary and updates the
| stack frame.  The function is then restored back into the
| machine and the 040 completes the operation.  This routine
| is only used by the unsupported data type/format handler.
| (Exception vector 55).
|
| For packed move out (fmove.p fpm,<ea>) the operation is
| completed here; data is packed and moved to user memory. 
| The stack is restored to the 040 only in the case of a
| reportable exception in the conversion.
|

|RES_FUNC    IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

sp_bnds:
	.short	0x3f81,0x407e
	.short	0x3f6a,0x0000
dp_bnds:
	.short	0x3c01,0x43fe
	.short	0x3bcd,0x0000

|	xref	mem_write
|	xref	bindec
|	xref	get_fline
|	xref	round
|	xref	denorm
|	xref	dest_ext
|	xref	dest_dbl
|	xref	dest_sgl
|	xref	unf_sub
|	xref	nrm_set
|	xref	dnrm_lp
|	xref	ovf_res
|	xref	reg_dest
|	xref	t_ovfl
|	xref	t_unfl

	.global	res_func
	.global	p_move

res_func:
	clrb	%a6@(DNRM_FLG)
	clrb	%a6@(RES_FLG)
	clrb	%a6@(CU_ONLY)
	tstb	%a6@(DY_MO_FLG)
	beqs	monadic
dyadic:
	btst	#7,%a6@(DTAG)		|if dop = norm=000, zero=001,
|				;inf=010 or nan=011
	beqs	monadic		|then branch
|				;else denorm
| HANDLE DESTINATION DENORM HERE
|				;set dtag to norm
|				;write the tag & fpte15 to the fstack
	lea	%a6@(FPTEMP),%a0

	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)

	bsr	nrm_set		|normalize number (exp will go negative)
	bclr	#sign_bit,%a0@(LOCAL_EX)		|get rid of false sign
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|change back to IEEE ext format
	beqs	dpos
	bset	#sign_bit,%a0@(LOCAL_EX)
dpos:
	bfclr	%a6@(DTAG){#0:#4}		|set tag to normalized, FPTE15 = 0
	bset	#4,%a6@(DTAG)		|set FPTE15
	orb	#0x0f,%a6@(DNRM_FLG)
monadic:
	lea	%a6@(ETEMP),%a0
	btst	#direction_bit,%a6@(CMDREG1B)		|check direction
	bne	opclass3		|it is a mv out
|
| At this point, only oplcass 0 and 2 possible
|
	btst	#7,%a6@(STAG)		|if sop = norm=000, zero=001,
|				;inf=010 or nan=011
	bne	mon_dnrm		|else denorm
	tstb	%a6@(DY_MO_FLG)		|all cases of dyadic instructions would
	bne	normal		|require normalization of denorm

| At this point:
|	monadic instructions:	fabs  = 0x18  fneg   = 0x1a  ftst   = 0x3a
|				fmove = 0x00  fsmove = 0x40  fdmove = 0x44
|				fsqrt = 0x05* fssqrt = 0x41  fdsqrt = 0x45
|				(*fsqrt reencoded to 0x05)
|
	movew	%a6@(CMDREG1B),%d0		|get command register
	andil	#0x7f,%d0		|strip to only command word
|
| At this point, fabs, fneg, fsmove, fdmove, ftst, fsqrt, fssqrt, and 
| fdsqrt are possible.
| For cases fabs, fneg, fsmove, and fdmove goto spos (do not normalize)
| For cases fsqrt, fssqrt, and fdsqrt goto nrm_src (do normalize)
|
	btst	#0,%d0
	bne	normal		|weed out fsqrt instructions
|
| cu_norm handles fmove in instructions with normalized inputs.
| The routine round is used to correctly round the input for the
| destination precision and mode.
|
cu_norm:
	st	%a6@(CU_ONLY)		|set cu-only inst flag
	movew	%a6@(CMDREG1B),%d0
	andib	#0x3b,%d0		|isolate bits to select inst
	tstb	%d0
	beql	cu_nmove		|if zero, it is an fmove
	cmpib	#0x18,%d0
	beql	cu_nabs		|if 0x18, it is fabs
	cmpib	#0x1a,%d0
	beql	cu_nneg		|if 0x1a, it is fneg
|
| Inst is ftst.  Check the source operand and set the cc's accordingly.
| No write is done, so simply rts.
|
cu_ntst:
	movew	%a0@(LOCAL_EX),%d0
	bclr	#15,%d0
	sne	%a0@(LOCAL_SGN)
	beqs	cu_ntpo
	orl	#neg_mask,%a6@(USER_FPSR)		|set N
cu_ntpo:
	cmpiw	#0x7fff,%d0		|test for inf/nan
	bnes	cu_ntcz
	tstl	%a0@(LOCAL_HI)
	bnes	cu_ntn
	tstl	%a0@(LOCAL_LO)
	bnes	cu_ntn
	orl	#inf_mask,%a6@(USER_FPSR)
	rts
cu_ntn:
	orl	#nan_mask,%a6@(USER_FPSR)
	movel	%a6@(ETEMP_EX),%a6@(FPTEMP_EX)		|set up fptemp sign for 
|						;snan handler

	rts
cu_ntcz:
	tstl	%a0@(LOCAL_HI)
	bnel	cu_ntsx
	tstl	%a0@(LOCAL_LO)
	bnel	cu_ntsx
	orl	#z_mask,%a6@(USER_FPSR)
cu_ntsx:
	rts
|
| Inst is fabs.  Execute the absolute value function on the input.
| Branch to the fmove code.  If the operand is NaN, do nothing.
|
cu_nabs:
	moveb	%a6@(STAG),%d0
	btst	#5,%d0		|test for NaN or zero
	bne	wr_etemp		|if either, simply write it
	bclr	#7,%a0@(LOCAL_EX)		|do abs
	bras	cu_nmove		|fmove code will finish
|
| Inst is fneg.  Execute the negate value function on the input.
| Fall though to the fmove code.  If the operand is NaN, do nothing.
|
cu_nneg:
	moveb	%a6@(STAG),%d0
	btst	#5,%d0		|test for NaN or zero
	bne	wr_etemp		|if either, simply write it
	bchg	#7,%a0@(LOCAL_EX)		|do neg
|
| Inst is fmove.  This code also handles all result writes.
| If bit 2 is set, round is forced to double.  If it is clear,
| and bit 6 is set, round is forced to single.  If both are clear,
| the round precision is found in the %fpcr.  If the rounding precision
| is double or single, round the result before the write.
|
cu_nmove:
	moveb	%a6@(STAG),%d0
	andib	#0xe0,%d0		|isolate stag bits
	bne	wr_etemp		|if not norm, simply write it
	btst	#2,%a6@(CMDREG1B+1)		|check for rd
	bne	cu_nmrd
	btst	#6,%a6@(CMDREG1B+1)		|check for rs
	bne	cu_nmrs
|
| The move or operation is not with forced precision.  Test for
| nan or inf as the input; if so, simply write it to FPn.  Use the
| FPCR_MODE byte to get rounding on norms and zeros.
|
cu_nmnr:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0
	tstb	%d0		|check for extended
	beq	cu_wrexn		|if so, just write result
	cmpib	#1,%d0		|check for single
	beq	cu_nmrs		|fall through to double
|
| The move is fdmove or round precision is double.
|
cu_nmrd:
	movel	#2,%d0		|set up the size for denorm
	movew	%a0@(LOCAL_EX),%d1		|compare exponent to double threshold
	andw	#0x7fff,%d1		|
	cmpw	#0x3c01,%d1
	bls	cu_nunfl
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		|get rmode
	orl	#0x00020000,%d1		|or in rprec (double)
	clrl	%d0		|clear g,r,s for round
	bclr	#sign_bit,%a0@(LOCAL_EX)		|convert to internal format
	sne	%a0@(LOCAL_SGN)
	bsrl	round
	bfclr	%a0@(LOCAL_SGN){#0:#8}
	beqs	cu_nmrdc
	bset	#sign_bit,%a0@(LOCAL_EX)
cu_nmrdc:
	movew	%a0@(LOCAL_EX),%d1		|check for overflow
	andw	#0x7fff,%d1
	cmpw	#0x43ff,%d1
	bge	cu_novfl		|take care of overflow case
	bra	cu_wrexn
|
| The move is fsmove or round precision is single.
|
cu_nmrs:
	movel	#1,%d0
	movew	%a0@(LOCAL_EX),%d1
	andw	#0x7fff,%d1
	cmpw	#0x3f81,%d1
	bls	cu_nunfl
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1
	orl	#0x00010000,%d1
	clrl	%d0
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)
	bsrl	round
	bfclr	%a0@(LOCAL_SGN){#0:#8}
	beqs	cu_nmrsc
	bset	#sign_bit,%a0@(LOCAL_EX)
cu_nmrsc:
	movew	%a0@(LOCAL_EX),%d1
	andw	#0x7FFF,%d1
	cmpw	#0x407f,%d1
	blt	cu_wrexn
|
| The operand is above precision boundaries.  Use t_ovfl to
| generate the correct value.
|
cu_novfl:
	bsr	t_ovfl
	bra	cu_wrexn
|
| The operand is below precision boundaries.  Use denorm to
| generate the correct value.
|
cu_nunfl:
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)
	bsr	denorm
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|change back to IEEE ext format
	beqs	cu_nucont
	bset	#sign_bit,%a0@(LOCAL_EX)
cu_nucont:
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1
	btst	#2,%a6@(CMDREG1B+1)		|check for rd
	bne	inst_d
	btst	#6,%a6@(CMDREG1B+1)		|check for rs
	bne	inst_s
	swap	%d1
	moveb	%a6@(FPCR_MODE),%d1
	lsrb	#6,%d1
	swap	%d1
	bra	inst_sd
inst_d:
	orl	#0x00020000,%d1
	bra	inst_sd
inst_s:
	orl	#0x00010000,%d1
inst_sd:
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)
	bsrl	round
	bfclr	%a0@(LOCAL_SGN){#0:#8}
	beqs	cu_nuflp
	bset	#sign_bit,%a0@(LOCAL_EX)
cu_nuflp:
	btst	#inex2_bit,%a6@(FPSR_EXCEPT)
	beqs	cu_nuninx
	orl	#aunfl_mask,%a6@(USER_FPSR)		|if the round was inex, set AUNFL
cu_nuninx:
	tstl	%a0@(LOCAL_HI)		|test for zero
	bnes	cu_nunzro
	tstl	%a0@(LOCAL_LO)
	bnes	cu_nunzro
|
| The mantissa is zero from the denorm loop.  Check sign and rmode
| to see if rounding should have occurred which would leave the lsb.
|
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0		|isolate rmode
	cmpil	#0x20,%d0
	blts	cu_nzro
	bnes	cu_nrp
cu_nrm:
	tstw	%a0@(LOCAL_EX)		|if positive, set lsb
	bges	cu_nzro
	btst	#7,%a6@(FPCR_MODE)		|check for double
	beqs	cu_nincs
	bras	cu_nincd
cu_nrp:
	tstw	%a0@(LOCAL_EX)		|if positive, set lsb
	blts	cu_nzro
	btst	#7,%a6@(FPCR_MODE)		|check for double
	beqs	cu_nincs
cu_nincd:
	orl	#0x800,%a0@(LOCAL_LO)		|inc for double
	bra	cu_nunzro
cu_nincs:
	orl	#0x100,%a0@(LOCAL_HI)		|inc for single
	bra	cu_nunzro
cu_nzro:
	orl	#z_mask,%a6@(USER_FPSR)
	moveb	%a6@(STAG),%d0
	andib	#0xe0,%d0
	cmpib	#0x40,%d0		|check if input was tagged zero
	beqs	cu_numv
cu_nunzro:
	orl	#unfl_mask,%a6@(USER_FPSR)		|set unfl
cu_numv:
	movel	%a0@,%a6@(ETEMP)
	movel	%a0@(4),%a6@(ETEMP_HI)
	movel	%a0@(8),%a6@(ETEMP_LO)
|
| Write the result to memory, setting the %fpsr cc bits.  NaN and Inf
| bypass cu_wrexn.
|
cu_wrexn:
	tstw	%a0@(LOCAL_EX)		|test for zero
	beqs	cu_wrzero
	cmpw	#0x8000,%a0@(LOCAL_EX)		|test for zero
	bnes	cu_wreon
cu_wrzero:
	orl	#z_mask,%a6@(USER_FPSR)		|set Z bit
cu_wreon:
	tstw	%a0@(LOCAL_EX)
	bpl	wr_etemp
	orl	#neg_mask,%a6@(USER_FPSR)
	bra	wr_etemp

|
| HANDLE SOURCE DENORM HERE
|
|				;clear denorm stag to norm
|				;write the new tag & ete15 to the fstack
mon_dnrm:
|
| At this point, check for the cases in which normalizing the 
| denorm produces incorrect results.
|
	tstb	%a6@(DY_MO_FLG)		|all cases of dyadic instructions would
	bnes	nrm_src		|require normalization of denorm

| At this point:
|	monadic instructions:	fabs  = 0x18  fneg   = 0x1a  ftst   = 0x3a
|				fmove = 0x00  fsmove = 0x40  fdmove = 0x44
|				fsqrt = 0x05* fssqrt = 0x41  fdsqrt = 0x45
|				(*fsqrt reencoded to 0x05)
|
	movew	%a6@(CMDREG1B),%d0		|get command register
	andil	#0x7f,%d0		|strip to only command word
|
| At this point, fabs, fneg, fsmove, fdmove, ftst, fsqrt, fssqrt, and 
| fdsqrt are possible.
| For cases fabs, fneg, fsmove, and fdmove goto spos (do not normalize)
| For cases fsqrt, fssqrt, and fdsqrt goto nrm_src (do normalize)
|
	btst	#0,%d0
	bnes	nrm_src		|weed out fsqrt instructions
	st	%a6@(CU_ONLY)		|set cu-only inst flag
	bra	cu_dnrm		|fmove, fabs, fneg, ftst 
|				;cases go to cu_dnrm
nrm_src:
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)
	bsr	nrm_set		|normalize number (exponent will go 
|				; negative)
	bclr	#sign_bit,%a0@(LOCAL_EX)		|get rid of false sign

	bfclr	%a0@(LOCAL_SGN){#0:#8}		|change back to IEEE ext format
	beqs	spos
	bset	#sign_bit,%a0@(LOCAL_EX)
spos:
	bfclr	%a6@(STAG){#0:#4}		|set tag to normalized, FPTE15 = 0
	bset	#4,%a6@(STAG)		|set ETE15
	orb	#0xf0,%a6@(DNRM_FLG)
normal:
	tstb	%a6@(DNRM_FLG)		|check if any of the ops were denorms
	bne	ck_wrap		|if so, check if it is a potential
|				;wrap-around case
fix_stk:
	moveb	#0xfe,%a6@(CU_SAVEPC)
	bclr	#E1,%a6@(E_BYTE)

	clrw	%a6@(NMNEXC)

	st	%a6@(RES_FLG)		|indicate that a restore is needed
	rts

|
| cu_dnrm handles all cu-only instructions (fmove, fabs, fneg, and
| ftst) completely in software without an frestore to the 040. 
|
cu_dnrm:
	st	%a6@(CU_ONLY)
	movew	%a6@(CMDREG1B),%d0
	andib	#0x3b,%d0		|isolate bits to select inst
	tstb	%d0
	beql	cu_dmove		|if zero, it is an fmove
	cmpib	#0x18,%d0
	beql	cu_dabs		|if 0x18, it is fabs
	cmpib	#0x1a,%d0
	beql	cu_dneg		|if 0x1a, it is fneg
|
| Inst is ftst.  Check the source operand and set the cc's accordingly.
| No write is done, so simply rts.
|
cu_dtst:
	movew	%a0@(LOCAL_EX),%d0
	bclr	#15,%d0
	sne	%a0@(LOCAL_SGN)
	beqs	cu_dtpo
	orl	#neg_mask,%a6@(USER_FPSR)		|set N
cu_dtpo:
	cmpiw	#0x7fff,%d0		|test for inf/nan
	bnes	cu_dtcz
	tstl	%a0@(LOCAL_HI)
	bnes	cu_dtn
	tstl	%a0@(LOCAL_LO)
	bnes	cu_dtn
	orl	#inf_mask,%a6@(USER_FPSR)
	rts
cu_dtn:
	orl	#nan_mask,%a6@(USER_FPSR)
	movel	%a6@(ETEMP_EX),%a6@(FPTEMP_EX)		|set up fptemp sign for 
|						;snan handler
	rts
cu_dtcz:
	tstl	%a0@(LOCAL_HI)
	bnel	cu_dtsx
	tstl	%a0@(LOCAL_LO)
	bnel	cu_dtsx
	orl	#z_mask,%a6@(USER_FPSR)
cu_dtsx:
	rts
|
| Inst is fabs.  Execute the absolute value function on the input.
| Branch to the fmove code.
|
cu_dabs:
	bclr	#7,%a0@(LOCAL_EX)		|do abs
	bras	cu_dmove		|fmove code will finish
|
| Inst is fneg.  Execute the negate value function on the input.
| Fall though to the fmove code.
|
cu_dneg:
	bchg	#7,%a0@(LOCAL_EX)		|do neg
|
| Inst is fmove.  This code also handles all result writes.
| If bit 2 is set, round is forced to double.  If it is clear,
| and bit 6 is set, round is forced to single.  If both are clear,
| the round precision is found in the %fpcr.  If the rounding precision
| is double or single, the result is zero, and the mode is checked
| to determine if the lsb of the result should be set.
|
cu_dmove:
	btst	#2,%a6@(CMDREG1B+1)		|check for rd
	bne	cu_dmrd
	btst	#6,%a6@(CMDREG1B+1)		|check for rs
	bne	cu_dmrs
|
| The move or operation is not with forced precision.  Use the
| FPCR_MODE byte to get rounding.
|
cu_dmnr:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0
	tstb	%d0		|check for extended
	beq	cu_wrexd		|if so, just write result
	cmpib	#1,%d0		|check for single
	beq	cu_dmrs		|fall through to double
|
| The move is fdmove or round precision is double.  Result is zero.
| Check rmode for rp or rm and set lsb accordingly.
|
cu_dmrd:
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		|get rmode
	tstw	%a0@(LOCAL_EX)		|check sign
	blts	cu_dmdn
	cmpib	#3,%d1		|check for rp
	bne	cu_dpd		|load double pos zero
	bra	cu_dpdr		|load double pos zero w/lsb
cu_dmdn:
	cmpib	#2,%d1		|check for rm
	bne	cu_dnd		|load double neg zero
	bra	cu_dndr		|load double neg zero w/lsb
|
| The move is fsmove or round precision is single.  Result is zero.
| Check for rp or rm and set lsb accordingly.
|
cu_dmrs:
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		|get rmode
	tstw	%a0@(LOCAL_EX)		|check sign
	blts	cu_dmsn
	cmpib	#3,%d1		|check for rp
	bne	cu_spd		|load single pos zero
	bra	cu_spdr		|load single pos zero w/lsb
cu_dmsn:
	cmpib	#2,%d1		|check for rm
	bne	cu_snd		|load single neg zero
	bra	cu_sndr		|load single neg zero w/lsb
|
| The precision is extended, so the result in etemp is correct.
| Simply set unfl (not inex2 or aunfl) and write the result to 
| the correct fp register.
cu_wrexd:
	orl	#unfl_mask,%a6@(USER_FPSR)
	tstw	%a0@(LOCAL_EX)
	beq	wr_etemp
	orl	#neg_mask,%a6@(USER_FPSR)
	bra	wr_etemp
|
| These routines write +/- zero in double format.  The routines
| cu_dpdr and cu_dndr set the double lsb.
|
cu_dpd:
	movel	#0x3c010000,%a0@(LOCAL_EX)		|force pos double zero
	clrl	%a0@(LOCAL_HI)
	clrl	%a0@(LOCAL_LO)
	orl	#z_mask,%a6@(USER_FPSR)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
cu_dpdr:
	movel	#0x3c010000,%a0@(LOCAL_EX)		|force pos double zero
	clrl	%a0@(LOCAL_HI)
	movel	#0x800,%a0@(LOCAL_LO)		|with lsb set
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
cu_dnd:
	movel	#0xbc010000,%a0@(LOCAL_EX)		|force pos double zero
	clrl	%a0@(LOCAL_HI)
	clrl	%a0@(LOCAL_LO)
	orl	#z_mask,%a6@(USER_FPSR)
	orl	#neg_mask,%a6@(USER_FPSR)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
cu_dndr:
	movel	#0xbc010000,%a0@(LOCAL_EX)		|force pos double zero
	clrl	%a0@(LOCAL_HI)
	movel	#0x800,%a0@(LOCAL_LO)		|with lsb set
	orl	#neg_mask,%a6@(USER_FPSR)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
|
| These routines write +/- zero in single format.  The routines
| cu_dpdr and cu_dndr set the single lsb.
|
cu_spd:
	movel	#0x3f810000,%a0@(LOCAL_EX)		|force pos single zero
	clrl	%a0@(LOCAL_HI)
	clrl	%a0@(LOCAL_LO)
	orl	#z_mask,%a6@(USER_FPSR)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
cu_spdr:
	movel	#0x3f810000,%a0@(LOCAL_EX)		|force pos single zero
	movel	#0x100,%a0@(LOCAL_HI)		|with lsb set
	clrl	%a0@(LOCAL_LO)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
cu_snd:
	movel	#0xbf810000,%a0@(LOCAL_EX)		|force pos single zero
	clrl	%a0@(LOCAL_HI)
	clrl	%a0@(LOCAL_LO)
	orl	#z_mask,%a6@(USER_FPSR)
	orl	#neg_mask,%a6@(USER_FPSR)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
cu_sndr:
	movel	#0xbf810000,%a0@(LOCAL_EX)		|force pos single zero
	movel	#0x100,%a0@(LOCAL_HI)		|with lsb set
	clrl	%a0@(LOCAL_LO)
	orl	#neg_mask,%a6@(USER_FPSR)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	bra	wr_etemp
	
|
| This code checks for 16-bit overflow conditions on dyadic
| operations which are not restorable into the floating-point
| unit and must be completed in software.  Basically, this
| condition exists with a very large norm and a denorm.  One
| of the operands must be denormalized to enter this code.
|
| Flags used:
|	DY_MO_FLG contains 0 for monadic op, 0xff for dyadic
|	DNRM_FLG contains 0x00 for neither op denormalized
|	                  0x0f for the destination op denormalized
|	                  0xf0 for the source op denormalized
|	                  0xff for both ops denormalzed
|
| The wrap-around condition occurs for add, sub, div, and cmp
| when 
|
|	abs(dest_exp - src_exp) >= 0x8000
|
| and for mul when
|
|	(dest_exp + src_exp) < 0x0
|
| we must process the operation here if this case is true.
|
| The rts following the frcfpn routine is the exit from res_func
| for this condition.  The restore flag (RES_FLG) is left clear.
| No frestore is done unless an exception is to be reported.
|
| For fadd: 
|	if(sign_of(dest) != sign_of(src))
|		replace exponent of src with 0x3fff (keep sign)
|		use fpu to perform dest+new_src (user's rmode and X)
|		clr sticky
|	else
|		set sticky
|	call round with user's precision and mode
|	move result to fpn and wbtemp
|
| For fsub:
|	if(sign_of(dest) == sign_of(src))
|		replace exponent of src with 0x3fff (keep sign)
|		use fpu to perform dest+new_src (user's rmode and X)
|		clr sticky
|	else
|		set sticky
|	call round with user's precision and mode
|	move result to fpn and wbtemp
|
| For fdiv/fsgldiv:
|	if(both operands are denorm)
|		restore_to_fpu;
|	if(dest is norm)
|		force_ovf;
|	else(dest is denorm)
|		force_unf:
|
| For fcmp:
|	if(dest is norm)
|		N = sign_of(dest);
|	else(dest is denorm)
|		N = sign_of(src);
|
| For fmul:
|	if(both operands are denorm)
|		force_unf;
|	if((dest_exp + src_exp) < 0)
|		force_unf:
|	else
|		restore_to_fpu;
|
| local equates:
	.set	addcode,0x22
	.set	subcode,0x28
	.set	mulcode,0x23
	.set	divcode,0x20
	.set	cmpcode,0x38
ck_wrap:
	tstb	%a6@(DY_MO_FLG)		|check for fsqrt
	beq	fix_stk		|if zero, it is fsqrt
	movew	%a6@(CMDREG1B),%d0
	andiw	#0x3b,%d0		|strip to command bits
	cmpiw	#addcode,%d0
	beq	wrap_add
	cmpiw	#subcode,%d0
	beq	wrap_sub
	cmpiw	#mulcode,%d0
	beq	wrap_mul
	cmpiw	#cmpcode,%d0
	beq	wrap_cmp
|
| Inst is fdiv.  
|
wrap_div:
	cmpb	#0xff,%a6@(DNRM_FLG)		|if both ops denorm, 
	beq	fix_stk		|restore to fpu
|
| One of the ops is denormalized.  Test for wrap condition
| and force the result.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|check for dest denorm
	bnes	div_srcd
div_destd:
	bsrl	ckinf_ns
	bne	fix_stk
	bfextu	%a6@(ETEMP_EX){#1:#15},%d0		|get src exp (always pos)
	bfexts	%a6@(FPTEMP_EX){#1:#15},%d1		|get dest exp (always neg)
	subl	%d1,%d0		|subtract dest from src
	cmpl	#0x7fff,%d0
	blt	fix_stk		|if less, not wrap case
	clrb	%a6@(WBTEMP_SGN)
	movew	%a6@(ETEMP_EX),%d0		|find the sign of the result
	movew	%a6@(FPTEMP_EX),%d1
	eorw	%d1,%d0
	andiw	#0x8000,%d0
	beq	force_unf
	st	%a6@(WBTEMP_SGN)
	bra	force_unf

ckinf_ns:
	moveb	%a6@(STAG),%d0		|check source tag for inf or nan
	bra	ck_in_com
ckinf_nd:
	moveb	%a6@(DTAG),%d0		|check destination tag for inf or nan
ck_in_com:
	andib	#0x60,%d0		|isolate tag bits
	cmpb	#0x40,%d0		|is it inf?
	beq	nan_or_inf		|not wrap case
	cmpb	#0x60,%d0		|is it nan?
	beq	nan_or_inf		|yes, not wrap case?
	cmpb	#0x20,%d0		|is it a zero?
	beq	nan_or_inf		|yes
	clrl	%d0
	rts	|then		|it is either a zero of norm,
|					;check wrap case
nan_or_inf:
	moveql	#-1,%d0
	rts



div_srcd:
	bsrl	ckinf_nd
	bne	fix_stk
	bfextu	%a6@(FPTEMP_EX){#1:#15},%d0		|get dest exp (always pos)
	bfexts	%a6@(ETEMP_EX){#1:#15},%d1		|get src exp (always neg)
	subl	%d1,%d0		|subtract src from dest
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
	clrb	%a6@(WBTEMP_SGN)
	movew	%a6@(ETEMP_EX),%d0		|find the sign of the result
	movew	%a6@(FPTEMP_EX),%d1
	eorw	%d1,%d0
	andiw	#0x8000,%d0
	beqs	force_ovf
	st	%a6@(WBTEMP_SGN)
|
| This code handles the case of the instruction resulting in 
| an overflow condition.
|
force_ovf:
	bclr	#E1,%a6@(E_BYTE)
	orl	#ovfl_inx_mask,%a6@(USER_FPSR)
	clrw	%a6@(NMNEXC)
	lea	%a6@(WBTEMP),%a0		|point %a0 to memory location
	movew	%a6@(CMDREG1B),%d0
	btst	#6,%d0		|test for forced precision
	beqs	frcovf_fpcr
	btst	#2,%d0		|check for double
	bnes	frcovf_dbl
	movel	#0x1,%d0		|inst is forced single
	bras	frcovf_rnd
frcovf_dbl:
	movel	#0x2,%d0		|inst is forced double
	bras	frcovf_rnd
frcovf_fpcr:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0		|inst not forced - use %fpcr prec
frcovf_rnd:

| The 881/882 does not set inex2 for the following case, so the 
| line is commented out to be compatible with 881/882
|	tst.b	%d0
|	beq.b	frcovf_x
|	or.l	#inex2_mask,%a6@(USER_FPSR) ;if prec is s or d, set inex2

|frcovf_x:
	bsrl	ovf_res		|get correct result based on
|					;round precision/mode.  This 
|					;sets FPSR_CC correctly
|					;returns in external format
	bfclr	%a6@(WBTEMP_SGN){#0:#8}
	beq	frcfpn
	bset	#sign_bit,%a6@(WBTEMP_EX)
	bra	frcfpn
|
| Inst is fadd.
|
wrap_add:
	cmpb	#0xff,%a6@(DNRM_FLG)		|if both ops denorm, 
	beq	fix_stk		|restore to fpu
|
| One of the ops is denormalized.  Test for wrap condition
| and complete the instruction.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|check for dest denorm
	bnes	add_srcd
add_destd:
	bsrl	ckinf_ns
	bne	fix_stk
	bfextu	%a6@(ETEMP_EX){#1:#15},%d0		|get src exp (always pos)
	bfexts	%a6@(FPTEMP_EX){#1:#15},%d1		|get dest exp (always neg)
	subl	%d1,%d0		|subtract dest from src
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
	bra	add_wrap
add_srcd:
	bsrl	ckinf_nd
	bne	fix_stk
	bfextu	%a6@(FPTEMP_EX){#1:#15},%d0		|get dest exp (always pos)
	bfexts	%a6@(ETEMP_EX){#1:#15},%d1		|get src exp (always neg)
	subl	%d1,%d0		|subtract src from dest
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
|
| Check the signs of the operands.  If they are unlike, the fpu
| can be used to add the norm and 1.0 with the sign of the
| denorm and it will correctly generate the result in extended
| precision.  We can then call round with no sticky and the result
| will be correct for the user's rounding mode and precision.  If
| the signs are the same, we call round with the sticky bit set
| and the result will be correctfor the user's rounding mode and
| precision.
|
add_wrap:
	movew	%a6@(ETEMP_EX),%d0
	movew	%a6@(FPTEMP_EX),%d1
	eorw	%d1,%d0
	andiw	#0x8000,%d0
	beq	add_same
|
| The signs are unlike.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|is dest the denorm?
	bnes	add_u_srcd
	movew	%a6@(FPTEMP_EX),%d0
	andiw	#0x8000,%d0
	orw	#0x3fff,%d0		|force the exponent to +/- 1
	movew	%d0,%a6@(FPTEMP_EX)		|in the denorm
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	fmovel	%d0,%fpcr		|set up users rmode and X
	fmovex	%a6@(ETEMP),%fp0
	faddx	%a6@(FPTEMP),%fp0
	lea	%a6@(WBTEMP),%a0		|point %a0 to wbtemp in frame
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture cc's and inex from fadd
	fmovex	%fp0,%a6@(WBTEMP)		|write result to memory
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	clrl	%d0		|force sticky to zero
	bclr	#sign_bit,%a6@(WBTEMP_EX)
	sne	%a6@(WBTEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(WBTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beq	frcfpnr
	bset	#sign_bit,%a6@(WBTEMP_EX)
	bra	frcfpnr
add_u_srcd:
	movew	%a6@(ETEMP_EX),%d0
	andiw	#0x8000,%d0
	orw	#0x3fff,%d0		|force the exponent to +/- 1
	movew	%d0,%a6@(ETEMP_EX)		|in the denorm
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	fmovel	%d0,%fpcr		|set up users rmode and X
	fmovex	%a6@(ETEMP),%fp0
	faddx	%a6@(FPTEMP),%fp0
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture cc's and inex from fadd
	lea	%a6@(WBTEMP),%a0		|point %a0 to wbtemp in frame
	fmovex	%fp0,%a6@(WBTEMP)		|write result to memory
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	clrl	%d0		|force sticky to zero
	bclr	#sign_bit,%a6@(WBTEMP_EX)
	sne	%a6@(WBTEMP_SGN)		|use internal format for round
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(WBTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beq	frcfpnr
	bset	#sign_bit,%a6@(WBTEMP_EX)
	bra	frcfpnr
|
| Signs are alike:
|
add_same:
	cmpb	#0x0f,%a6@(DNRM_FLG)		|is dest the denorm?
	bnes	add_s_srcd
add_s_destd:
	lea	%a6@(ETEMP),%a0
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	movel	#0x20000000,%d0		|set sticky for round
	bclr	#sign_bit,%a6@(ETEMP_EX)
	sne	%a6@(ETEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(ETEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	add_s_dclr
	bset	#sign_bit,%a6@(ETEMP_EX)
add_s_dclr:
	lea	%a6@(WBTEMP),%a0
	movel	%a6@(ETEMP),%a0@		|write result to wbtemp
	movel	%a6@(ETEMP_HI),%a0@(4)
	movel	%a6@(ETEMP_LO),%a0@(8)
	tstw	%a6@(ETEMP_EX)
	bgt	add_ckovf
	orl	#neg_mask,%a6@(USER_FPSR)
	bra	add_ckovf
add_s_srcd:
	lea	%a6@(FPTEMP),%a0
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	movel	#0x20000000,%d0		|set sticky for round
	bclr	#sign_bit,%a6@(FPTEMP_EX)
	sne	%a6@(FPTEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(FPTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	add_s_sclr
	bset	#sign_bit,%a6@(FPTEMP_EX)
add_s_sclr:
	lea	%a6@(WBTEMP),%a0
	movel	%a6@(FPTEMP),%a0@		|write result to wbtemp
	movel	%a6@(FPTEMP_HI),%a0@(4)
	movel	%a6@(FPTEMP_LO),%a0@(8)
	tstw	%a6@(FPTEMP_EX)
	bgt	add_ckovf
	orl	#neg_mask,%a6@(USER_FPSR)
add_ckovf:
	movew	%a6@(WBTEMP_EX),%d0
	andiw	#0x7fff,%d0
	cmpiw	#0x7fff,%d0
	bne	frcfpnr
|
| The result has overflowed to 0x7fff exponent.  Set I, ovfl,
| and aovfl, and clr the mantissa (incorrectly set by the
| round routine.)
|
	orl	#inf_mask+ovfl_inx_mask,%a6@(USER_FPSR)		|
	clrl	%a0@(4)
	bra	frcfpnr
|
| Inst is fsub.
|
wrap_sub:
	cmpb	#0xff,%a6@(DNRM_FLG)		|if both ops denorm, 
	beq	fix_stk		|restore to fpu
|
| One of the ops is denormalized.  Test for wrap condition
| and complete the instruction.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|check for dest denorm
	bnes	sub_srcd
sub_destd:
	bsrl	ckinf_ns
	bne	fix_stk
	bfextu	%a6@(ETEMP_EX){#1:#15},%d0		|get src exp (always pos)
	bfexts	%a6@(FPTEMP_EX){#1:#15},%d1		|get dest exp (always neg)
	subl	%d1,%d0		|subtract src from dest
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
	bra	sub_wrap
sub_srcd:
	bsrl	ckinf_nd
	bne	fix_stk
	bfextu	%a6@(FPTEMP_EX){#1:#15},%d0		|get dest exp (always pos)
	bfexts	%a6@(ETEMP_EX){#1:#15},%d1		|get src exp (always neg)
	subl	%d1,%d0		|subtract dest from src
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
|
| Check the signs of the operands.  If they are alike, the fpu
| can be used to subtract from the norm 1.0 with the sign of the
| denorm and it will correctly generate the result in extended
| precision.  We can then call round with no sticky and the result
| will be correct for the user's rounding mode and precision.  If
| the signs are unlike, we call round with the sticky bit set
| and the result will be correctfor the user's rounding mode and
| precision.
|
sub_wrap:
	movew	%a6@(ETEMP_EX),%d0
	movew	%a6@(FPTEMP_EX),%d1
	eorw	%d1,%d0
	andiw	#0x8000,%d0
	bne	sub_diff
|
| The signs are alike.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|is dest the denorm?
	bnes	sub_u_srcd
	movew	%a6@(FPTEMP_EX),%d0
	andiw	#0x8000,%d0
	orw	#0x3fff,%d0		|force the exponent to +/- 1
	movew	%d0,%a6@(FPTEMP_EX)		|in the denorm
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	fmovel	%d0,%fpcr		|set up users rmode and X
	fmovex	%a6@(FPTEMP),%fp0
	fsubx	%a6@(ETEMP),%fp0
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture cc's and inex from fadd
	lea	%a6@(WBTEMP),%a0		|point %a0 to wbtemp in frame
	fmovex	%fp0,%a6@(WBTEMP)		|write result to memory
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	clrl	%d0		|force sticky to zero
	bclr	#sign_bit,%a6@(WBTEMP_EX)
	sne	%a6@(WBTEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(WBTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beq	frcfpnr
	bset	#sign_bit,%a6@(WBTEMP_EX)
	bra	frcfpnr
sub_u_srcd:
	movew	%a6@(ETEMP_EX),%d0
	andiw	#0x8000,%d0
	orw	#0x3fff,%d0		|force the exponent to +/- 1
	movew	%d0,%a6@(ETEMP_EX)		|in the denorm
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	fmovel	%d0,%fpcr		|set up users rmode and X
	fmovex	%a6@(FPTEMP),%fp0
	fsubx	%a6@(ETEMP),%fp0
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture cc's and inex from fadd
	lea	%a6@(WBTEMP),%a0		|point %a0 to wbtemp in frame
	fmovex	%fp0,%a6@(WBTEMP)		|write result to memory
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	clrl	%d0		|force sticky to zero
	bclr	#sign_bit,%a6@(WBTEMP_EX)
	sne	%a6@(WBTEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(WBTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beq	frcfpnr
	bset	#sign_bit,%a6@(WBTEMP_EX)
	bra	frcfpnr
|
| Signs are unlike:
|
sub_diff:
	cmpb	#0x0f,%a6@(DNRM_FLG)		|is dest the denorm?
	bnes	sub_s_srcd
sub_s_destd:
	lea	%a6@(ETEMP),%a0
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	movel	#0x20000000,%d0		|set sticky for round
|
| Since the dest is the denorm, the sign is the opposite of the
| norm sign.
|
	eoriw	#0x8000,%a6@(ETEMP_EX)		|flip sign on result
	tstw	%a6@(ETEMP_EX)
	bgts	sub_s_dwr
	orl	#neg_mask,%a6@(USER_FPSR)
sub_s_dwr:
	bclr	#sign_bit,%a6@(ETEMP_EX)
	sne	%a6@(ETEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(ETEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	sub_s_dclr
	bset	#sign_bit,%a6@(ETEMP_EX)
sub_s_dclr:
	lea	%a6@(WBTEMP),%a0
	movel	%a6@(ETEMP),%a0@		|write result to wbtemp
	movel	%a6@(ETEMP_HI),%a0@(4)
	movel	%a6@(ETEMP_LO),%a0@(8)
	bra	sub_ckovf
sub_s_srcd:
	lea	%a6@(FPTEMP),%a0
	movel	%a6@(USER_FPCR),%d0
	andil	#0x30,%d0
	lsrl	#4,%d0		|put rmode in lower 2 bits
	movel	%a6@(USER_FPCR),%d1
	andil	#0xc0,%d1
	lsrl	#6,%d1		|put precision in upper word
	swap	%d1
	orl	%d0,%d1		|set up for round call
	movel	#0x20000000,%d0		|set sticky for round
	bclr	#sign_bit,%a6@(FPTEMP_EX)
	sne	%a6@(FPTEMP_SGN)
	bsrl	round		|round result to users rmode & prec
	bfclr	%a6@(FPTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	sub_s_sclr
	bset	#sign_bit,%a6@(FPTEMP_EX)
sub_s_sclr:
	lea	%a6@(WBTEMP),%a0
	movel	%a6@(FPTEMP),%a0@		|write result to wbtemp
	movel	%a6@(FPTEMP_HI),%a0@(4)
	movel	%a6@(FPTEMP_LO),%a0@(8)
	tstw	%a6@(FPTEMP_EX)
	bgt	sub_ckovf
	orl	#neg_mask,%a6@(USER_FPSR)
sub_ckovf:
	movew	%a6@(WBTEMP_EX),%d0
	andiw	#0x7fff,%d0
	cmpiw	#0x7fff,%d0
	bne	frcfpnr
|
| The result has overflowed to 0x7fff exponent.  Set I, ovfl,
| and aovfl, and clr the mantissa (incorrectly set by the
| round routine.)
|
	orl	#inf_mask+ovfl_inx_mask,%a6@(USER_FPSR)		|
	clrl	%a0@(4)
	bra	frcfpnr
|
| Inst is fcmp.
|
wrap_cmp:
	cmpb	#0xff,%a6@(DNRM_FLG)		|if both ops denorm, 
	beq	fix_stk		|restore to fpu
|
| One of the ops is denormalized.  Test for wrap condition
| and complete the instruction.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|check for dest denorm
	bnes	cmp_srcd
cmp_destd:
	bsrl	ckinf_ns
	bne	fix_stk
	bfextu	%a6@(ETEMP_EX){#1:#15},%d0		|get src exp (always pos)
	bfexts	%a6@(FPTEMP_EX){#1:#15},%d1		|get dest exp (always neg)
	subl	%d1,%d0		|subtract dest from src
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
	tstw	%a6@(ETEMP_EX)		|set N to ~sign_of(src)
	bge	cmp_setn
	rts
cmp_srcd:
	bsrl	ckinf_nd
	bne	fix_stk
	bfextu	%a6@(FPTEMP_EX){#1:#15},%d0		|get dest exp (always pos)
	bfexts	%a6@(ETEMP_EX){#1:#15},%d1		|get src exp (always neg)
	subl	%d1,%d0		|subtract src from dest
	cmpl	#0x8000,%d0
	blt	fix_stk		|if less, not wrap case
	tstw	%a6@(FPTEMP_EX)		|set N to sign_of(dest)
	blt	cmp_setn
	rts
cmp_setn:
	orl	#neg_mask,%a6@(USER_FPSR)
	rts

|
| Inst is fmul.
|
wrap_mul:
	cmpb	#0xff,%a6@(DNRM_FLG)		|if both ops denorm, 
	beq	force_unf		|force an underflow (really!)
|
| One of the ops is denormalized.  Test for wrap condition
| and complete the instruction.
|
	cmpb	#0x0f,%a6@(DNRM_FLG)		|check for dest denorm
	bnes	mul_srcd
mul_destd:
	bsrl	ckinf_ns
	bne	fix_stk
	bfextu	%a6@(ETEMP_EX){#1:#15},%d0		|get src exp (always pos)
	bfexts	%a6@(FPTEMP_EX){#1:#15},%d1		|get dest exp (always neg)
	addl	%d1,%d0		|subtract dest from src
	bgt	fix_stk
	bra	force_unf
mul_srcd:
	bsrl	ckinf_nd
	bne	fix_stk
	bfextu	%a6@(FPTEMP_EX){#1:#15},%d0		|get dest exp (always pos)
	bfexts	%a6@(ETEMP_EX){#1:#15},%d1		|get src exp (always neg)
	addl	%d1,%d0		|subtract src from dest
	bgt	fix_stk
	
|
| This code handles the case of the instruction resulting in 
| an underflow condition.
|
force_unf:
	bclr	#E1,%a6@(E_BYTE)
	orl	#unfinx_mask,%a6@(USER_FPSR)
	clrw	%a6@(NMNEXC)
	clrb	%a6@(WBTEMP_SGN)
	movew	%a6@(ETEMP_EX),%d0		|find the sign of the result
	movew	%a6@(FPTEMP_EX),%d1
	eorw	%d1,%d0
	andiw	#0x8000,%d0
	beqs	frcunfcont
	st	%a6@(WBTEMP_SGN)
frcunfcont:
	lea	%a6@(WBTEMP),%a0		|point %a0 to memory location
	movew	%a6@(CMDREG1B),%d0
	btst	#6,%d0		|test for forced precision
	beqs	frcunf_fpcr
	btst	#2,%d0		|check for double
	bnes	frcunf_dbl
	movel	#0x1,%d0		|inst is forced single
	bras	frcunf_rnd
frcunf_dbl:
	movel	#0x2,%d0		|inst is forced double
	bras	frcunf_rnd
frcunf_fpcr:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0		|inst not forced - use %fpcr prec
frcunf_rnd:
	bsrl	unf_sub		|get correct result based on
|					;round precision/mode.  This 
|					;sets FPSR_CC correctly
	bfclr	%a6@(WBTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	frcfpn
	bset	#sign_bit,%a6@(WBTEMP_EX)
	bra	frcfpn

|
| Write the result to the user's fpn.  All results must be HUGE to be
| written; otherwise the results would have overflowed or underflowed.
| If the rounding precision is single or double, the ovf_res routine
| is needed to correctly supply the max value.
|
frcfpnr:
	movew	%a6@(CMDREG1B),%d0
	btst	#6,%d0		|test for forced precision
	beqs	frcfpn_fpcr
	btst	#2,%d0		|check for double
	bnes	frcfpn_dbl
	movel	#0x1,%d0		|inst is forced single
	bras	frcfpn_rnd
frcfpn_dbl:
	movel	#0x2,%d0		|inst is forced double
	bras	frcfpn_rnd
frcfpn_fpcr:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0		|inst not forced - use %fpcr prec
	tstb	%d0
	beqs	frcfpn		|if extended, write what you got
frcfpn_rnd:
	bclr	#sign_bit,%a6@(WBTEMP_EX)
	sne	%a6@(WBTEMP_SGN)
	bsrl	ovf_res		|get correct result based on
|					;round precision/mode.  This 
|					;sets FPSR_CC correctly
	bfclr	%a6@(WBTEMP_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	frcfpn_clr
	bset	#sign_bit,%a6@(WBTEMP_EX)
frcfpn_clr:
	orl	#ovfinx_mask,%a6@(USER_FPSR)
| 
| Perform the write.
|
frcfpn:
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|extract fp destination register
	cmpib	#3,%d0
	bles	frc0123		|check if dest is %fp0-%fp3
	movel	#7,%d1
	subl	%d0,%d1
	clrl	%d0
	bset	%d1,%d0
	fmovemx	%a6@(WBTEMP),%d0
	rts
frc0123:
	tstb	%d0
	beqs	frc0_dst
	cmpib	#1,%d0
	beqs	frc1_dst		|
	cmpib	#2,%d0
	beqs	frc2_dst		|
frc3_dst:
	movel	%a6@(WBTEMP_EX),%a6@(USER_FP3)
	movel	%a6@(WBTEMP_HI),%a6@(USER_FP3+4)
	movel	%a6@(WBTEMP_LO),%a6@(USER_FP3+8)
	rts
frc2_dst:
	movel	%a6@(WBTEMP_EX),%a6@(USER_FP2)
	movel	%a6@(WBTEMP_HI),%a6@(USER_FP2+4)
	movel	%a6@(WBTEMP_LO),%a6@(USER_FP2+8)
	rts
frc1_dst:
	movel	%a6@(WBTEMP_EX),%a6@(USER_FP1)
	movel	%a6@(WBTEMP_HI),%a6@(USER_FP1+4)
	movel	%a6@(WBTEMP_LO),%a6@(USER_FP1+8)
	rts
frc0_dst:
	movel	%a6@(WBTEMP_EX),%a6@(USER_FP0)
	movel	%a6@(WBTEMP_HI),%a6@(USER_FP0+4)
	movel	%a6@(WBTEMP_LO),%a6@(USER_FP0+8)
	rts

|
| Write etemp to fpn.
| A check is made on enabled and signalled snan exceptions,
| and the destination is not overwritten if this condition exists.
| This code is designed to make fmoveins of unsupported data types
| faster.
|
wr_etemp:
	btst	#snan_bit,%a6@(FPSR_EXCEPT)		|if snan is set, and
	beqs	fmoveinc		|enabled, force restore
	btst	#snan_bit,%a6@(FPCR_ENABLE)		|and don't overwrite
	beqs	fmoveinc		|the dest
	movel	%a6@(ETEMP_EX),%a6@(FPTEMP_EX)		|set up fptemp sign for 
|						;snan handler
	tstb	%a6@(ETEMP)		|check for negative
	blts	snan_neg
	rts
snan_neg:
	orl	#neg_bit,%a6@(USER_FPSR)		|snan is negative; set N
	rts
fmoveinc:
	clrw	%a6@(NMNEXC)
	bclr	#E1,%a6@(E_BYTE)
	moveb	%a6@(STAG),%d0		|check if stag is inf
	andib	#0xe0,%d0
	cmpib	#0x40,%d0
	bnes	fminc_cnan
	orl	#inf_mask,%a6@(USER_FPSR)		|if inf, nothing yet has set I
	tstw	%a0@(LOCAL_EX)		|check sign
	bges	fminc_con
	orl	#neg_mask,%a6@(USER_FPSR)
	bra	fminc_con
fminc_cnan:
	cmpib	#0x60,%d0		|check if stag is NaN
	bnes	fminc_czero
	orl	#nan_mask,%a6@(USER_FPSR)		|if nan, nothing yet has set NaN
	movel	%a6@(ETEMP_EX),%a6@(FPTEMP_EX)		|set up fptemp sign for 
|						;snan handler
	tstw	%a0@(LOCAL_EX)		|check sign
	bges	fminc_con
	orl	#neg_mask,%a6@(USER_FPSR)
	bra	fminc_con
fminc_czero:
	cmpib	#0x20,%d0		|check if zero
	bnes	fminc_con
	orl	#z_mask,%a6@(USER_FPSR)		|if zero, set Z
	tstw	%a0@(LOCAL_EX)		|check sign
	bges	fminc_con
	orl	#neg_mask,%a6@(USER_FPSR)
fminc_con:
	bfextu	%a6@(CMDREG1B){#6:#3},%d0		|extract fp destination register
	cmpib	#3,%d0
	bles	fp0123		|check if dest is %fp0-%fp3
	movel	#7,%d1
	subl	%d0,%d1
	clrl	%d0
	bset	%d1,%d0
	fmovemx	%a6@(ETEMP),%d0
	rts

fp0123:
	tstb	%d0
	beqs	fp0_dst
	cmpib	#1,%d0
	beqs	fp1_dst		|
	cmpib	#2,%d0
	beqs	fp2_dst		|
fp3_dst:
	movel	%a6@(ETEMP_EX),%a6@(USER_FP3)
	movel	%a6@(ETEMP_HI),%a6@(USER_FP3+4)
	movel	%a6@(ETEMP_LO),%a6@(USER_FP3+8)
	rts
fp2_dst:
	movel	%a6@(ETEMP_EX),%a6@(USER_FP2)
	movel	%a6@(ETEMP_HI),%a6@(USER_FP2+4)
	movel	%a6@(ETEMP_LO),%a6@(USER_FP2+8)
	rts
fp1_dst:
	movel	%a6@(ETEMP_EX),%a6@(USER_FP1)
	movel	%a6@(ETEMP_HI),%a6@(USER_FP1+4)
	movel	%a6@(ETEMP_LO),%a6@(USER_FP1+8)
	rts
fp0_dst:
	movel	%a6@(ETEMP_EX),%a6@(USER_FP0)
	movel	%a6@(ETEMP_HI),%a6@(USER_FP0+4)
	movel	%a6@(ETEMP_LO),%a6@(USER_FP0+8)
	rts

opclass3:
	st	%a6@(CU_ONLY)
	movew	%a6@(CMDREG1B),%d0		|check if packed moveout
	andiw	#0x0c00,%d0		|isolate last 2 bits of size field
	cmpiw	#0x0c00,%d0		|if size is 011 or 111, it is packed
	beq	pack_out		|else it is norm or denorm
	bra	mv_out

	
|
|	MOVE OUT
|

mv_tbl:
	.long	li
	.long	sgp
	.long	xp
	.long	mvout_end		|should never be taken
	.long	wi
	.long	dp
	.long	bi
	.long	mvout_end		|should never be taken
mv_out:
	bfextu	%a6@(CMDREG1B){#3:#3},%d1		|put source specifier in %d1
	lea	mv_tbl,%a0
	movel	%a0@(%d1:l:4),%a0
	jmp	%a0@

|
| This exit is for move-out to memory.  The aunfl bit is 
| set if the result is inex and unfl is signalled.
|
mvout_end:
	btst	#inex2_bit,%a6@(FPSR_EXCEPT)
	beqs	no_aufl
	btst	#unfl_bit,%a6@(FPSR_EXCEPT)
	beqs	no_aufl
	bset	#aunfl_bit,%a6@(FPSR_AEXCEPT)
no_aufl:
	clrw	%a6@(NMNEXC)
	bclr	#E1,%a6@(E_BYTE)
	fmovel	#0,%FPSR		|clear any cc bits from res_func
|
| Return ETEMP to extended format from internal extended format so
| that gen_except will have a correctly signed value for ovfl/unfl
| handlers.
|
	bfclr	%a6@(ETEMP_SGN){#0:#8}
	beqs	mvout_con
	bset	#sign_bit,%a6@(ETEMP_EX)
mvout_con:
	rts
|
| This exit is for move-out to int register.  The aunfl bit is 
| not set in any case for this move.
|
mvouti_end:
	clrw	%a6@(NMNEXC)
	bclr	#E1,%a6@(E_BYTE)
	fmovel	#0,%FPSR		|clear any cc bits from res_func
|
| Return ETEMP to extended format from internal extended format so
| that gen_except will have a correctly signed value for ovfl/unfl
| handlers.
|
	bfclr	%a6@(ETEMP_SGN){#0:#8}
	beqs	mvouti_con
	bset	#sign_bit,%a6@(ETEMP_EX)
mvouti_con:
	rts
|
| li is used to handle a long integer source specifier
|

li:
	moveql	#4,%d0		|set byte count

	btst	#7,%a6@(STAG)		|check for extended denorm
	bne	int_dnrm		|if so, branch

	fmovemx	%a6@(ETEMP),%fp0-%fp0
	fcmpd	#0r2147483647.0,%fp0
| 41dfffffffc00000 in dbl prec = 401d0000fffffffe00000000 in ext prec
	fbge	lo_plrg		|
	fcmpd	#0r-2147483648.0,%fp0
| c1e0000000000000 in dbl prec = c01e00008000000000000000 in ext prec
	fble	lo_nlrg
|
| at this point, the answer is between the largest pos and neg values
|
	movel	%a6@(USER_FPCR),%d1		|use user's rounding mode
	andil	#0x30,%d1
	fmovel	%d1,%fpcr
	fmovel	%fp0,%a6@(L_SCR1)		|let the 040 perform conversion
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture inex2/ainex if set
	bra	int_wrt


lo_plrg:
	movel	#0x7fffffff,%a6@(L_SCR1)		|answer is largest positive int
	fbeq	int_wrt		|exact answer
	fcmpd	#0r2147483647.5,%fp0
| 41dfffffffe00000 in dbl prec = 401d0000ffffffff00000000 in ext prec
	fbge	int_operr		|set operr
	bra	int_inx		|set inexact

lo_nlrg:
	movel	#0x80000000,%a6@(L_SCR1)
	fbeq	int_wrt		|exact answer
	fcmpd	#0r-2147483648.5,%fp0
| c1e0000000100000 in dbl prec = c01e00008000000080000000 in ext prec
	fblt	int_operr		|set operr
	bra	int_inx		|set inexact

|
| wi is used to handle a word integer source specifier
|

wi:
	moveql	#2,%d0		|set byte count

	btst	#7,%a6@(STAG)		|check for extended denorm
	bne	int_dnrm		|branch if so

	fmovemx	%a6@(ETEMP),%fp0-%fp0
	fcmps	#0r32767.0,%fp0
| 46fffe00 in sgl prec = 400d0000fffe000000000000 in ext prec
	fbge	wo_plrg		|
	fcmps	#0r-32768.0,%fp0
| c7000000 in sgl prec = c00e00008000000000000000 in ext prec
	fble	wo_nlrg

|
| at this point, the answer is between the largest pos and neg values
|
	movel	%a6@(USER_FPCR),%d1		|use user's rounding mode
	andil	#0x30,%d1
	fmovel	%d1,%fpcr
	fmovew	%fp0,%a6@(L_SCR1)		|let the 040 perform conversion
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture inex2/ainex if set
	bra	int_wrt

wo_plrg:
	movew	#0x7fff,%a6@(L_SCR1)		|answer is largest positive int
	fbeq	int_wrt		|exact answer
	fcmps	#0r32767.5,%fp0
| 46ffff00 in sgl prec = 400d0000ffff000000000000 in ext prec
	fbge	int_operr		|set operr
	bra	int_inx		|set inexact

wo_nlrg:
	movew	#0x8000,%a6@(L_SCR1)
	fbeq	int_wrt		|exact answer
	fcmps	#0r-32768.5,%fp0
| c7000080 in sgl prec = c00e00008000800000000000 in ext prec
	fblt	int_operr		|set operr
	bra	int_inx		|set inexact

|
| bi is used to handle a byte integer source specifier
|

bi:
	moveql	#1,%d0		|set byte count

	btst	#7,%a6@(STAG)		|check for extended denorm
	bne	int_dnrm		|branch if so

	fmovemx	%a6@(ETEMP),%fp0-%fp0
	fcmps	#0r127.0,%fp0
| 42fe0000 in sgl prec = 40050000fe00000000000000 in ext prec
	fbge	by_plrg		|
	fcmps	#0r-128.0,%fp0
| c3000000 in sgl prec = c00600008000000000000000 in ext prec
	fble	by_nlrg

|
| at this point, the answer is between the largest pos and neg values
|
	movel	%a6@(USER_FPCR),%d1		|use user's rounding mode
	andil	#0x30,%d1
	fmovel	%d1,%fpcr
	fmoveb	%fp0,%a6@(L_SCR1)		|let the 040 perform conversion
	fmovel	%fpsr,%d1
	orl	%d1,%a6@(USER_FPSR)		|capture inex2/ainex if set
	bra	int_wrt

by_plrg:
	moveb	#0x7f,%a6@(L_SCR1)		|answer is largest positive int
	fbeq	int_wrt		|exact answer
	fcmps	#0r127.5,%fp0
| 42ff0000 in sgl prec = 40050000ff00000000000000 in ext prec
	fbge	int_operr		|set operr
	bra	int_inx		|set inexact

by_nlrg:
	moveb	#0x80,%a6@(L_SCR1)
	fbeq	int_wrt		|exact answer
	fcmps	#0r-128.5,%fp0
| c3008000 in sgl prec = c00600008080000000000000 in ext prec
	fblt	int_operr		|set operr
	bra	int_inx		|set inexact

|
| Common integer routines
|
| int_drnrm---account for possible nonzero result for round up with positive
| operand and round down for negative answer.  In the first case (result = 1)
| byte-width (store in %d0) of result must be honored.  In the second case,
| -1 in %a6@(L_SCR1) will cover all contingencies (FMOVE.B/W/L out).

int_dnrm:
	clrl	%a6@(L_SCR1)		| initialize result to 0
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		| %d1 is the rounding mode
	cmpb	#2,%d1		|
	bmis	int_inx		| if RN or RZ, done
	bnes	int_rp		| if RP, continue below
	tstw	%a6@(ETEMP)		| RM: store -1 in L_SCR1 if src is negative
	bpls	int_inx		| otherwise result is 0
	movel	#-1,%a6@(L_SCR1)
	bras	int_inx
int_rp:
	tstw	%a6@(ETEMP)		| RP: store +1 of proper width in L_SCR1 if
|				; source is greater than 0
	bmis	int_inx		| otherwise, result is 0
	lea	%a6@(L_SCR1),%a1		| %a1 is address of L_SCR1
	addal	%d0,%a1		| offset by destination width -1
	subal	#1,%a1		|
	bset	#0,%a1@		| set low bit at %a1 address
int_inx:
	oril	#inx2a_mask,%a6@(USER_FPSR)
	bras	int_wrt
int_operr:
	fmovemx	%fp0-%fp0,%a6@(FPTEMP)		|FPTEMP must contain the extended
|				;precision source that needs to be
|				;converted to integer this is required
|				;if the operr exception is enabled.
|				;set operr/aiop (no inex2 on int ovfl)

	oril	#opaop_mask,%a6@(USER_FPSR)
|				;fall through to perform int_wrt
int_wrt:
	movel	%a6@(EXC_EA),%a1		|load destination address
	tstl	%a1		|check to see if it is a dest register
	beqs	wrt_dn		|write data register 
	lea	%a6@(L_SCR1),%a0		|point to supervisor source address
	bsrl	mem_write
	bra	mvouti_end

wrt_dn:
	movel	%d0,%sp@-		|%d0 currently contains the size to write
	bsrl	get_fline		|get_fline returns Dn in %d0
	andiw	#0x7,%d0		|isolate register
	movel	%sp@+,%d1		|get size
	cmpil	#4,%d1		|most frequent case
	beqs	sz_long
	cmpil	#2,%d1
	bnes	sz_con
	orl	#8,%d0		|add 'word' size to register#
	bras	sz_con
sz_long:
	orl	#0x10,%d0		|add 'long' size to register#
sz_con:
	movel	%d0,%d1		|reg_dest expects size:reg in %d1
	bsrl	reg_dest		|load proper data register
	bra	mvouti_end		|
xp:
	lea	%a6@(ETEMP),%a0
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)
	btst	#7,%a6@(STAG)		|check for extended denorm
	bne	xdnrm
	clrl	%d0
	bras	do_fp		|do normal case
sgp:
	lea	%a6@(ETEMP),%a0
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)
	btst	#7,%a6@(STAG)		|check for extended denorm
	bne	sp_catas		|branch if so
	movew	%a0@(LOCAL_EX),%d0
	lea	sp_bnds,%a1
	cmpw	%a1@,%d0
	blt	sp_under
	cmpw	%a1@(2),%d0
	bgt	sp_over
	movel	#1,%d0		|set destination format to single
	bras	do_fp		|do normal case
dp:
	lea	%a6@(ETEMP),%a0
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)

	btst	#7,%a6@(STAG)		|check for extended denorm
	bne	dp_catas		|branch if so

	movew	%a0@(LOCAL_EX),%d0
	lea	dp_bnds,%a1

	cmpw	%a1@,%d0
	blt	dp_under
	cmpw	%a1@(2),%d0
	bgt	dp_over
	
	movel	#2,%d0		|set destination format to double
|				;fall through to do_fp
|
do_fp:
	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		|rnd mode in %d1
	swap	%d0		|rnd prec in upper word
	addl	%d0,%d1		|%d1 has PREC/MODE info
	
	clrl	%d0		|clear g,r,s 

	bsrl	round		|round 

	movel	%a0,%a1
	movel	%a6@(EXC_EA),%a0

	bfextu	%a6@(CMDREG1B){#3:#3},%d1		|extract destination format
|					;at this point only the dest
|					;formats sgl, dbl, ext are
|					;possible
	cmpb	#2,%d1
	bgts	ddbl		|double=5, extended=2, single=1
	bnes	dsgl
|					;fall through to dext
dext:
	bsrl	dest_ext
	bra	mvout_end
dsgl:
	bsrl	dest_sgl
	bra	mvout_end
ddbl:
	bsrl	dest_dbl
	bra	mvout_end

|
| Handle possible denorm or catastrophic underflow cases here
|
xdnrm:
	bsr	set_xop		|initialize WBTEMP
	bset	#wbtemp15_bit,%a6@(WB_BYTE)		|set wbtemp15

	movel	%a0,%a1
	movel	%a6@(EXC_EA),%a0		|%a0 has the destination pointer
	bsrl	dest_ext		|store to memory
	bset	#unfl_bit,%a6@(FPSR_EXCEPT)
	bra	mvout_end
	
sp_under:
	bset	#etemp15_bit,%a6@(STAG)

	cmpw	%a1@(4),%d0
	blts	sp_catas		|catastrophic underflow case	

	movel	#1,%d0		|load in round precision
	movel	#sgl_thresh,%d1		|load in single denorm threshold
	bsrl	dpspdnrm		|expects %d1 to have the proper
|				;denorm threshold
	bsrl	dest_sgl		|stores value to destination
	bset	#unfl_bit,%a6@(FPSR_EXCEPT)
	bra	mvout_end		|exit

dp_under:
	bset	#etemp15_bit,%a6@(STAG)

	cmpw	%a1@(4),%d0
	blts	dp_catas		|catastrophic underflow case
		
	movel	#dbl_thresh,%d1		|load in double precision threshold
	movel	#2,%d0		|
	bsrl	dpspdnrm		|expects %d1 to have proper
|				;denorm threshold
|				;expects %d0 to have round precision
	bsrl	dest_dbl		|store value to destination
	bset	#unfl_bit,%a6@(FPSR_EXCEPT)
	bra	mvout_end		|exit

|
| Handle catastrophic underflow cases here
|
sp_catas:
| Temp fix for z bit set in unf_sub
	movel	%a6@(USER_FPSR),%a7@-

	movel	#1,%d0		|set round precision to sgl

	bsrl	unf_sub		|%a0 points to result

	movel	%a7@+,%a6@(USER_FPSR)

	movel	#1,%d0
	subw	%d0,%a0@(LOCAL_EX)		|account for difference between
|				;denorm/norm bias

	movel	%a0,%a1		|%a1 has the operand input
	movel	%a6@(EXC_EA),%a0		|%a0 has the destination pointer
	
	bsrl	dest_sgl		|store the result
	oril	#unfinx_mask,%a6@(USER_FPSR)
	bra	mvout_end
	
dp_catas:
| Temp fix for z bit set in unf_sub
	movel	%a6@(USER_FPSR),%a7@-

	movel	#2,%d0		|set round precision to dbl
	bsrl	unf_sub		|%a0 points to result

	movel	%a7@+,%a6@(USER_FPSR)

	movel	#1,%d0
	subw	%d0,%a0@(LOCAL_EX)		|account for difference between 
|				;denorm/norm bias

	movel	%a0,%a1		|%a1 has the operand input
	movel	%a6@(EXC_EA),%a0		|%a0 has the destination pointer
	
	bsrl	dest_dbl		|store the result
	oril	#unfinx_mask,%a6@(USER_FPSR)
	bra	mvout_end

|
| Handle catastrophic overflow cases here
|
sp_over:
| Temp fix for z bit set in unf_sub
	movel	%a6@(USER_FPSR),%a7@-

	movel	#1,%d0
	lea	%a6@(FP_SCR1),%a0		|use FP_SCR1 for creating result
	movel	%a6@(ETEMP_EX),%a0@
	movel	%a6@(ETEMP_HI),%a0@(4)
	movel	%a6@(ETEMP_LO),%a0@(8)
	bsrl	ovf_res

	movel	%a7@+,%a6@(USER_FPSR)

	movel	%a0,%a1
	movel	%a6@(EXC_EA),%a0
	bsrl	dest_sgl
	orl	#ovfinx_mask,%a6@(USER_FPSR)
	bra	mvout_end

dp_over:
| Temp fix for z bit set in ovf_res
	movel	%a6@(USER_FPSR),%a7@-

	movel	#2,%d0
	lea	%a6@(FP_SCR1),%a0		|use FP_SCR1 for creating result
	movel	%a6@(ETEMP_EX),%a0@
	movel	%a6@(ETEMP_HI),%a0@(4)
	movel	%a6@(ETEMP_LO),%a0@(8)
	bsrl	ovf_res

	movel	%a7@+,%a6@(USER_FPSR)

	movel	%a0,%a1
	movel	%a6@(EXC_EA),%a0
	bsrl	dest_dbl
	orl	#ovfinx_mask,%a6@(USER_FPSR)
	bra	mvout_end

|
| 	DPSPDNRM
|
| This subroutine takes an extended normalized number and denormalizes
| it to the given round precision. This subroutine also decrements
| the input operand's exponent by 1 to account for the fact that
| dest_sgl or dest_dbl expects a normalized number's bias.
|
| Input: %a0  points to a normalized number in internal extended format
|	 %d0  is the round precision (=1 for sgl; =2 for dbl)
|	 %d1  is the single precision or double precision
|	     denorm threshold
|
| Output: (In the format for dest_sgl or dest_dbl)
|	 %a0   points to the destination
|   	 %a1   points to the operand
|
| Exceptions: Reports inexact 2 exception by setting USER_FPSR bits
|
dpspdnrm:
	movel	%d0,%a7@-		|save round precision
	clrl	%d0		|clear initial g,r,s
	bsrl	dnrm_lp		|careful with %d0, it's needed by round

	bfextu	%a6@(FPCR_MODE){#2:#2},%d1		|get rounding mode
	swap	%d1
	movew	%a7@(2),%d1		|set rounding precision 
	swap	%d1		|at this point %d1 has PREC/MODE info
	bsrl	round		|round result, sets the inex bit in
|				;USER_FPSR if needed

	movew	#1,%d0
	subw	%d0,%a0@(LOCAL_EX)		|account for difference in denorm
|				;vs norm bias

	movel	%a0,%a1		|%a1 has the operand input
	movel	%a6@(EXC_EA),%a0		|%a0 has the destination pointer
	addql	#4,%a7		|pop stack
	rts
|
| SET_XOP initialized WBTEMP with the value pointed to by %a0
| input: %a0 points to input operand in the internal extended format
|
set_xop:
	movel	%a0@(LOCAL_EX),%a6@(WBTEMP_EX)
	movel	%a0@(LOCAL_HI),%a6@(WBTEMP_HI)
	movel	%a0@(LOCAL_LO),%a6@(WBTEMP_LO)
	bfclr	%a6@(WBTEMP_SGN){#0:#8}
	beqs	sxop
	bset	#sign_bit,%a6@(WBTEMP_EX)
sxop:
	bfclr	%a6@(STAG){#5:#4}		|clear wbtm66,wbtm1,wbtm0,sbit
	rts
|
|	P_MOVE
|
p_movet:
	.long	p_move
	.long	p_movez
	.long	p_movei
	.long	p_moven
	.long	p_move
p_regd:
	.long	p_dyd0
	.long	p_dyd1
	.long	p_dyd2
	.long	p_dyd3
	.long	p_dyd4
	.long	p_dyd5
	.long	p_dyd6
	.long	p_dyd7

pack_out:
	lea	p_movet,%a0		|load jmp table address
	movew	%a6@(STAG),%d0		|get source tag
	bfextu	%d0{#16:#3},%d0		|isolate source bits
	movel	%a0@(%d0:w:4),%a0		|load %a0 with routine label for tag
	jmp	%a0@		|go to the routine

p_write:
	movel	#0x0c,%d0		|get byte count
	movel	%a6@(EXC_EA),%a1		|get the destination address
	bsr	mem_write		|write the user's destination
	clrb	%a6@(CU_SAVEPC)		|set the cu save %pc to all 0's

|
| Also note that the dtag must be set to norm here - this is because 
| the 040 uses the dtag to execute the correct microcode.
|
	bfclr	%a6@(DTAG){#0:#3}		|set dtag to norm

	rts

| Notes on handling of special case (zero, inf, and nan) inputs:
|	1. Operr is not signalled if the k-factor is greater than 18.
|	2. Per the manual, status bits are not set.
|

p_move:
	movew	%a6@(CMDREG1B),%d0
	btst	#kfact_bit,%d0		|test for dynamic k-factor
	beqs	statick		|if clear, k-factor is static
dynamick:
	bfextu	%d0{#25:#3},%d0		|isolate register for dynamic k-factor
	lea	p_regd,%a0
	movel	%a0@(%d0:l:4),%a0
	jmp	%a0@
statick:
	andiw	#0x007f,%d0		|get k-factor
	bfexts	%d0{#25:#7},%d0		|sign extend %d0 for bindec
	lea	%a6@(ETEMP),%a0		|%a0 will point to the packed decimal
	bsrl	bindec		|perform the convert; data at %a6
	lea	%a6@(FP_SCR1),%a0		|load %a0 with result address
	bral	p_write
p_movez:
	lea	%a6@(ETEMP),%a0		|%a0 will point to the packed decimal
	clrw	%a0@(2)		|clear lower word of exp
	clrl	%a0@(4)		|load second lword of ZERO
	clrl	%a0@(8)		|load third lword of ZERO
	bra	p_write		|go write results
p_movei:
	fmovel	#0,%FPSR		|clear aiop
	lea	%a6@(ETEMP),%a0		|%a0 will point to the packed decimal
	clrw	%a0@(2)		|clear lower word of exp
	bra	p_write		|go write the result
p_moven:
	lea	%a6@(ETEMP),%a0		|%a0 will point to the packed decimal
	clrw	%a0@(2)		|clear lower word of exp
	bra	p_write		|go write the result

|
| Routines to read the dynamic k-factor from Dn.
|
p_dyd0:
	movel	%a6@(USER_D0),%d0
	bras	statick
p_dyd1:
	movel	%a6@(USER_D1),%d0
	bras	statick
p_dyd2:
	movel	%d2,%d0
	bras	statick
p_dyd3:
	movel	%d3,%d0
	bras	statick
p_dyd4:
	movel	%d4,%d0
	bras	statick
p_dyd5:
	movel	%d5,%d0
	bras	statick
p_dyd6:
	movel	%d6,%d0
	bra	statick
p_dyd7:
	movel	%d7,%d0
	bra	statick

|	end
