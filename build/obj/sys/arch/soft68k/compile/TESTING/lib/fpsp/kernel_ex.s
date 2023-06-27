|	$NetBSD: kernel_ex.sa,v 1.2 1994/10/26 07:49:12 cgd Exp $

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
|	kernel_ex.sa 3.3 12/19/90 
|
| This file contains routines to force exception status in the 
| fpu for exceptional cases detected or reported within the
| transcendental functions.  Typically, the t_xx routine will
| set the appropriate bits in the USER_FPSR word on the stack.
| The bits are tested in gen_except.sa to determine if an exceptional
| situation needs to be created on return from the FPSP. 
|

|KERNEL_EX    IDNT    2,1 Motorola 040 Floating Point Software Package

	.text

	.include "fpsp.defs"

mns_inf:
	.long	0xffff0000,0x00000000,0x00000000
pls_inf:
	.long	0x7fff0000,0x00000000,0x00000000
nan:
	.long	0x7fff0000,0xffffffff,0xffffffff
huge:
	.long	0x7ffe0000,0xffffffff,0xffffffff

|	xref	ovf_r_k
|	xref	unf_sub
|	xref	nrm_set

	.global	t_dz
	.global	t_dz2
	.global	t_operr
	.global	t_unfl
	.global	t_ovfl
	.global	t_ovfl2
	.global	t_inx2
	.global	t_frcinx
	.global	t_extdnrm
	.global	t_resdnrm
	.global	dst_nan
	.global	src_nan
|
|	DZ exception
|
|
|	if dz trap disabled
|		store properly signed inf (use sign of etemp) into %fp0
|		set %FPSR exception status dz bit, condition code 
|		inf bit, and accrued dz bit
|		return
|		frestore the frame into the machine (done by unimp_hd)
|
|	else dz trap enabled
|		set exception status bit & accrued bits in %FPSR
|		set flag to disable sto_res from corrupting fp register
|		return
|		frestore the frame into the machine (done by unimp_hd)
|
| t_dz2 is used by monadic functions such as flogn (from do_func).
| t_dz is used by monadic functions such as satanh (from the 
| transcendental function).
|
t_dz2:
	bset	#neg_bit,%a6@(FPSR_CC)		|set neg bit in %FPSR
	fmovel	#0,%FPSR		|clr status bits (Z set)
	btst	#dz_bit,%a6@(FPCR_ENABLE)		|test %FPCR for dz exc enabled
	bnes	dz_ena_end
	bras	m_inf		|flogx always returns -inf
t_dz:
	fmovel	#0,%FPSR		|clr status bits (Z set)
	btst	#dz_bit,%a6@(FPCR_ENABLE)		|test %FPCR for dz exc enabled
	bnes	dz_ena
|
|	dz disabled
|
	btst	#sign_bit,%a6@(ETEMP_EX)		|check sign for neg or pos
	beqs	p_inf		|branch if pos sign

m_inf:
	fmovemx	mns_inf,%fp0-%fp0		|load -inf
	bset	#neg_bit,%a6@(FPSR_CC)		|set neg bit in %FPSR
	bras	set_fpsr
p_inf:
	fmovemx	pls_inf,%fp0-%fp0		|load +inf
set_fpsr:
	orl	#dzinf_mask,%a6@(USER_FPSR)		|set I,DZ,ADZ
	rts
|
|	dz enabled
|
dz_ena:
	btst	#sign_bit,%a6@(ETEMP_EX)		|check sign for neg or pos
	beqs	dz_ena_end
	bset	#neg_bit,%a6@(FPSR_CC)		|set neg bit in %FPSR
dz_ena_end:
	orl	#dzinf_mask,%a6@(USER_FPSR)		|set I,DZ,ADZ
	st	%a6@(STORE_FLG)
	rts
|
|	OPERR exception
|
|	if (operr trap disabled)
|		set %FPSR exception status operr bit, condition code 
|		nan bit; Store default NAN into %fp0
|		frestore the frame into the machine (done by unimp_hd)
|	
|	else (operr trap enabled)
|		set %FPSR exception status operr bit, accrued operr bit
|		set flag to disable sto_res from corrupting fp register
|		frestore the frame into the machine (done by unimp_hd)
|
t_operr:
	orl	#opnan_mask,%a6@(USER_FPSR)		|set NaN, OPERR, AIOP

	btst	#operr_bit,%a6@(FPCR_ENABLE)		|test %FPCR for operr enabled
	bnes	op_ena

	fmovemx	nan,%fp0-%fp0		|load default nan
	rts
op_ena:
	st	%a6@(STORE_FLG)		|do not corrupt destination
	rts

|
|	t_unfl --- UNFL exception
|
| This entry point is used by all routines requiring unfl, inex2,
| aunfl, and ainex to be set on exit.
|
| On entry, %a0 points to the exceptional operand.  The final exceptional
| operand is built in FP_SCR1 and only the sign from the original operand
| is used.
|
t_unfl:
	clrl	%a6@(FP_SCR1)		|set exceptional operand to zero
	clrl	%a6@(FP_SCR1+4)
	clrl	%a6@(FP_SCR1+8)
	tstb	%a0@		|extract sign from caller's exop
	bpls	unfl_signok
	bset	#sign_bit,%a6@(FP_SCR1)
unfl_signok:
	lea	%a6@(FP_SCR1),%a0
	orl	#unfinx_mask,%a6@(USER_FPSR)
|					;set UNFL, INEX2, AUNFL, AINEX
unfl_con:
	btst	#unfl_bit,%a6@(FPCR_ENABLE)
	beqs	unfl_dis

unfl_ena:
	bfclr	%a6@(STAG){#5:#3}		|clear wbtm66,wbtm1,wbtm0
	bset	#wbtemp15_bit,%a6@(WB_BYTE)		|set wbtemp15
	bset	#sticky_bit,%a6@(STICKY)		|set sticky bit

	bclr	#E1,%a6@(E_BYTE)

unfl_dis:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0		|get round precision
	
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|convert to internal ext format

	bsr	unf_sub		|returns IEEE result at %a0
|					;and sets FPSR_CC accordingly
	
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	unfl_fin

	bset	#sign_bit,%a0@(LOCAL_EX)
	bset	#sign_bit,%a6@(FP_SCR1)		|set sign bit of exc operand

unfl_fin:
	fmovemx	%a0@,%fp0-%fp0		|store result in %fp0
	rts
	

|
|	t_ovfl2 --- OVFL exception (without inex2 returned)
|
| This entry is used by scale to force catastrophic overflow.  The
| ovfl, aovfl, and ainex bits are set, but not the inex2 bit.
|
t_ovfl2:
	orl	#ovfl_inx_mask,%a6@(USER_FPSR)
	movel	%a6@(ETEMP),%a6@(FP_SCR1)
	movel	%a6@(ETEMP_HI),%a6@(FP_SCR1+4)
	movel	%a6@(ETEMP_LO),%a6@(FP_SCR1+8)
|
| Check for single or double round precision.  If single, check if
| the lower 40 bits of ETEMP are zero; if not, set inex2.  If double,
| check if the lower 21 bits are zero; if not, set inex2.
|
	moveb	%a6@(FPCR_MODE),%d0
	andib	#0xc0,%d0
	beq	t_work		|if extended, finish ovfl processing
	cmpib	#0x40,%d0		|test for single
	bnes	t_dbl
t_sgl:
	tstb	%a6@(ETEMP_LO)
	bnes	t_setinx2
	movel	%a6@(ETEMP_HI),%d0
	andil	#0xff,%d0		|look at only lower 8 bits
	bnes	t_setinx2
	bra	t_work
t_dbl:
	movel	%a6@(ETEMP_LO),%d0
	andil	#0x7ff,%d0		|look at only lower 11 bits
	beq	t_work
t_setinx2:
	orl	#inex2_mask,%a6@(USER_FPSR)
	bras	t_work
|
|	t_ovfl --- OVFL exception
|
|** Note: the exc operand is returned in ETEMP.
|
t_ovfl:
	orl	#ovfinx_mask,%a6@(USER_FPSR)
t_work:
	btst	#ovfl_bit,%a6@(FPCR_ENABLE)		|test %FPCR for ovfl enabled
	beqs	ovf_dis

ovf_ena:
	clrl	%a6@(FP_SCR1)		|set exceptional operand
	clrl	%a6@(FP_SCR1+4)
	clrl	%a6@(FP_SCR1+8)

	bfclr	%a6@(STAG){#5:#3}		|clear wbtm66,wbtm1,wbtm0
	bclr	#wbtemp15_bit,%a6@(WB_BYTE)		|clear wbtemp15
	bset	#sticky_bit,%a6@(STICKY)		|set sticky bit

	bclr	#E1,%a6@(E_BYTE)
|					;fall through to disabled case

| For disabled overflow call 'ovf_r_k'.  This routine loads the
| correct result based on the rounding precision, destination
| format, rounding mode and sign.
|
ovf_dis:
	bsr	ovf_r_k		|returns unsigned ETEMP_EX
|					;and sets FPSR_CC accordingly.
	bfclr	%a6@(ETEMP_SGN){#0:#8}		|fix sign
	beqs	ovf_pos
	bset	#sign_bit,%a6@(ETEMP_EX)
	bset	#sign_bit,%a6@(FP_SCR1)		|set exceptional operand sign
ovf_pos:
	fmovemx	%a6@(ETEMP),%fp0-%fp0		|move the result to %fp0
	rts


|
|	INEX2 exception
|
| The inex2 and ainex bits are set.
|
t_inx2:
	orl	#inx2a_mask,%a6@(USER_FPSR)		|set INEX2, AINEX
	rts

|
|	Force Inex2
|
| This routine is called by the transcendental routines to force
| the inex2 exception bits set in the %FPSR.  If the underflow bit
| is set, but the underflow trap was not taken, the aunfl bit in
| the %FPSR must be set.
|
t_frcinx:
	orl	#inx2a_mask,%a6@(USER_FPSR)		|set INEX2, AINEX
	btst	#unfl_bit,%a6@(FPSR_EXCEPT)		|test for unfl bit set
	beqs	no_uacc1		|if clear, do not set aunfl
	bset	#aunfl_bit,%a6@(FPSR_AEXCEPT)
no_uacc1:
	rts

|
|	DST_NAN
|
| Determine if the destination nan is signalling or non-signalling,
| and set the %FPSR bits accordingly.  See the MC68040 User's Manual 
| section 3.2.2.5 NOT-A-NUMBERS.
|
dst_nan:
	btst	#sign_bit,%a6@(FPTEMP_EX)		|test sign of nan
	beqs	dst_pos		|if clr, it was positive
	bset	#neg_bit,%a6@(FPSR_CC)		|set N bit
dst_pos:
	btst	#signan_bit,%a6@(FPTEMP_HI)		|check if signalling 
	beqs	dst_snan		|branch if signalling

	fmovel	%d1,%fpcr		|restore user's rmode/prec
	fmovex	%a6@(FPTEMP),%fp0		|return the non-signalling nan
|
| Check the source nan.  If it is signalling, snan will be reported.
|
	moveb	%a6@(STAG),%d0
	andib	#0xe0,%d0
	cmpib	#0x60,%d0
	bnes	no_snan
	btst	#signan_bit,%a6@(ETEMP_HI)		|check if signalling 
	bnes	no_snan
	orl	#snaniop_mask,%a6@(USER_FPSR)		|set NAN, SNAN, AIOP
no_snan:
	rts	

dst_snan:
	btst	#snan_bit,%a6@(FPCR_ENABLE)		|check if trap enabled 
	beqs	dst_dis		|branch if disabled

	orb	#nan_tag,%a6@(DTAG)		|set up dtag for nan
	st	%a6@(STORE_FLG)		|do not store a result
	orl	#snaniop_mask,%a6@(USER_FPSR)		|set NAN, SNAN, AIOP
	rts

dst_dis:
	bset	#signan_bit,%a6@(FPTEMP_HI)		|set SNAN bit in sop 
	fmovel	%d1,%fpcr		|restore user's rmode/prec
	fmovex	%a6@(FPTEMP),%fp0		|load non-sign. nan 
	orl	#snaniop_mask,%a6@(USER_FPSR)		|set NAN, SNAN, AIOP
	rts

|
|	SRC_NAN
|
| Determine if the source nan is signalling or non-signalling,
| and set the %FPSR bits accordingly.  See the MC68040 User's Manual 
| section 3.2.2.5 NOT-A-NUMBERS.
|
src_nan:
	btst	#sign_bit,%a6@(ETEMP_EX)		|test sign of nan
	beqs	src_pos		|if clr, it was positive
	bset	#neg_bit,%a6@(FPSR_CC)		|set N bit
src_pos:
	btst	#signan_bit,%a6@(ETEMP_HI)		|check if signalling 
	beqs	src_snan		|branch if signalling
	fmovel	%d1,%fpcr		|restore user's rmode/prec
	fmovex	%a6@(ETEMP),%fp0		|return the non-signalling nan
	rts	

src_snan:
	btst	#snan_bit,%a6@(FPCR_ENABLE)		|check if trap enabled 
	beqs	src_dis		|branch if disabled
	bset	#signan_bit,%a6@(ETEMP_HI)		|set SNAN bit in sop 
	orb	#norm_tag,%a6@(DTAG)		|set up dtag for norm
	orb	#nan_tag,%a6@(STAG)		|set up stag for nan
	st	%a6@(STORE_FLG)		|do not store a result
	orl	#snaniop_mask,%a6@(USER_FPSR)		|set NAN, SNAN, AIOP
	rts

src_dis:
	bset	#signan_bit,%a6@(ETEMP_HI)		|set SNAN bit in sop 
	fmovel	%d1,%fpcr		|restore user's rmode/prec
	fmovex	%a6@(ETEMP),%fp0		|load non-sign. nan 
	orl	#snaniop_mask,%a6@(USER_FPSR)		|set NAN, SNAN, AIOP
	rts

|
| For all functions that have a denormalized input and that f(x)=x,
| this is the entry point
|
t_extdnrm:
	orl	#unfinx_mask,%a6@(USER_FPSR)
|					;set UNFL, INEX2, AUNFL, AINEX
	bras	xdnrm_con
|
| Entry point for scale with extended denorm.  The function does
| not set inex2, aunfl, or ainex.  
|
t_resdnrm:
	orl	#unfl_mask,%a6@(USER_FPSR)

xdnrm_con:
	btst	#unfl_bit,%a6@(FPCR_ENABLE)
	beqs	xdnrm_dis

|
| If exceptions are enabled, the additional task of setting up WBTEMP
| is needed so that when the underflow exception handler is entered,
| the user perceives no difference between what the 040 provides vs.
| what the FPSP provides.
|
xdnrm_ena:
	movel	%a0,%a7@-

	movel	%a0@(LOCAL_EX),%a6@(FP_SCR1)
	movel	%a0@(LOCAL_HI),%a6@(FP_SCR1+4)
	movel	%a0@(LOCAL_LO),%a6@(FP_SCR1+8)

	lea	%a6@(FP_SCR1),%a0

	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|convert to internal ext format
	tstw	%a0@(LOCAL_EX)		|check if input is denorm
	beqs	xdnrm_dn		|if so, skip nrm_set
	bsr	nrm_set		|normalize the result (exponent
|					;will be negative
xdnrm_dn:
	bclr	#sign_bit,%a0@(LOCAL_EX)		|take off false sign
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|change back to IEEE ext format
	beqs	xdep
	bset	#sign_bit,%a0@(LOCAL_EX)
xdep:
	bfclr	%a6@(STAG){#5:#3}		|clear wbtm66,wbtm1,wbtm0
	bset	#wbtemp15_bit,%a6@(WB_BYTE)		|set wbtemp15
	bclr	#sticky_bit,%a6@(STICKY)		|clear sticky bit
	bclr	#E1,%a6@(E_BYTE)
	movel	%a7@+,%a0
xdnrm_dis:
	bfextu	%a6@(FPCR_MODE){#0:#2},%d0		|get round precision
	bnes	not_ext		|if not round extended, store
|					;IEEE defaults
is_ext:
	btst	#sign_bit,%a0@(LOCAL_EX)
	beqs	xdnrm_store

	bset	#neg_bit,%a6@(FPSR_CC)		|set N bit in FPSR_CC

	bras	xdnrm_store

not_ext:
	bclr	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|convert to internal ext format
	bsr	unf_sub		|returns IEEE result pointed by
|					;%a0; sets FPSR_CC accordingly
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	xdnrm_store
	bset	#sign_bit,%a0@(LOCAL_EX)
xdnrm_store:
	fmovemx	%a0@,%fp0-%fp0		|store result in %fp0
	rts

|
| This subroutine is used for dyadic operations that use an extended
| denorm within the kernel. The approach used is to capture the frame,
| fix/restore.
|
	.global	t_avoid_unsupp
t_avoid_unsupp:
	link	%a2,#-LOCAL_SIZE		|so that %a2 fpsp.h negative 
|					;offsets may be used
	fsave	%a7@-
	tstb	%a7@(1)		|check if idle, exit if so
	beq	idle_end
	btst	#E1,%a2@(E_BYTE)		|check for an E1 exception if
|					;enabled, there is an unsupp
	beq	end_avun		|else, exit
	btst	#7,%a2@(DTAG)		|check for denorm destination
	beqs	src_den		|else, must be a source denorm
|
| handle destination denorm
|
	lea	%a2@(FPTEMP),%a0
	btst	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|convert to internal ext format
	bclr	#7,%a2@(DTAG)		|set DTAG to norm
	bsr	nrm_set		|normalize result, exponent
|					;will become negative
	bclr	#sign_bit,%a0@(LOCAL_EX)		|get rid of fake sign
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	ck_src_den		|check if source is also denorm
	bset	#sign_bit,%a0@(LOCAL_EX)
ck_src_den:
	btst	#7,%a2@(STAG)
	beqs	end_avun
src_den:
	lea	%a2@(ETEMP),%a0
	btst	#sign_bit,%a0@(LOCAL_EX)
	sne	%a0@(LOCAL_SGN)		|convert to internal ext format
	bclr	#7,%a2@(STAG)		|set STAG to norm
	bsr	nrm_set		|normalize result, exponent
|					;will become negative
	bclr	#sign_bit,%a0@(LOCAL_EX)		|get rid of fake sign
	bfclr	%a0@(LOCAL_SGN){#0:#8}		|convert back to IEEE ext format
	beqs	den_com
	bset	#sign_bit,%a0@(LOCAL_EX)
den_com:
	moveb	#0xfe,%a2@(CU_SAVEPC)		|set continue frame
	clrw	%a2@(NMNEXC)		|clear NMNEXC
	bclr	#E1,%a2@(E_BYTE)
|	fmove.l	%FPSR,%a2@(FPSR_SHADOW)
|	bset.b	#SFLAG,%a2@(E_BYTE)
|	bset.b	#XFLAG,%a2@(T_BYTE)
end_avun:
	frestore	%a7@+
	unlk	%a2
	rts
idle_end:
	addl	#4,%a7
	unlk	%a2
	rts
|	end
