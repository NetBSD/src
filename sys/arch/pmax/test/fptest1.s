/*
 * Routines for floating point emulation testing.
 */

#include <machine/regdef.h>
#include <machine/machAsmDefs.h>
#include <machine/machConst.h>

	.rdata
jtab:
	.word	add_s
	.word	add_d
	.word	sub_s
	.word	sub_d
	.word	mul_s
	.word	mul_d
	.word	div_s
	.word	div_d
	.word	abs_s
	.word	abs_d
	.word	mov_s
	.word	mov_d
	.word	neg_s
	.word	neg_d
	.word	cvt_s_d
	.word	cvt_s_w
	.word	cvt_d_s
	.word	cvt_d_w
	.word	cvt_w_s
	.word	cvt_w_d
	.word	c_f_s
	.word	c_un_s
	.word	c_eq_s
	.word	c_ueq_s
	.word	c_olt_s
	.word	c_ult_s
	.word	c_ole_s
	.word	c_ule_s
	.word	c_sf_s
	.word	c_ngle_s
	.word	c_seq_s
	.word	c_ngl_s
	.word	c_lt_s
	.word	c_nge_s
	.word	c_le_s
	.word	c_ngt_s
	.word	c_f_d
	.word	c_un_d
	.word	c_eq_d
	.word	c_ueq_d
	.word	c_olt_d
	.word	c_ult_d
	.word	c_ole_d
	.word	c_ule_d
	.word	c_sf_d
	.word	c_ngle_d
	.word	c_seq_d
	.word	c_ngl_d
	.word	c_lt_d
	.word	c_nge_d
	.word	c_le_d
	.word	c_ngt_d
	.text

LEAF(dofp)
	cfc1	v0, MACH_FPC_CSR
	lw	t1, 16(sp)	# get 4th arg
	mtc1	a1, $f4
	mtc1	a2, $f5
	mtc1	a3, $f6
	mtc1	t1, $f7
	ctc1	zero, MACH_FPC_CSR
	sll	a0, a0, 2	# index into jump table
	lw	t0, jtab(a0)
	j	t0

add_s:
	add.s	$f0, $f4, $f6
	b	done
add_d:
	add.d	$f0, $f4, $f6
	b	done
sub_s:
	sub.s	$f0, $f4, $f6
	b	done
sub_d:
	sub.d	$f0, $f4, $f6
	b	done
mul_s:
	mul.s	$f0, $f4, $f6
	b	done
mul_d:
	mul.d	$f0, $f4, $f6
	b	done
div_s:
	div.s	$f0, $f4, $f6
	b	done
div_d:
	div.d	$f0, $f4, $f6
	b	done
abs_s:
	abs.s	$f0, $f4
	b	done
abs_d:
	abs.d	$f0, $f4
	b	done
mov_s:
	mov.s	$f0, $f4
	b	done
mov_d:
	mov.d	$f0, $f4
	b	done
neg_s:
	neg.s	$f0, $f4
	b	done
neg_d:
	neg.d	$f0, $f4
	b	done
cvt_s_d:
	cvt.s.d	$f0, $f4
	b	done
cvt_s_w:
	cvt.s.w	$f0, $f4
	b	done
cvt_d_s:
	cvt.d.s	$f0, $f4
	b	done
cvt_d_w:
	cvt.d.w	$f0, $f4
	b	done
cvt_w_s:
	cvt.w.s	$f0, $f4
	b	done
cvt_w_d:
	cvt.w.d	$f0, $f4
	b	done
c_f_s:
	c.f.s	$f4, $f6
	b	done
c_un_s:
	c.un.s	$f4, $f6
	b	done
c_eq_s:
	c.eq.s	$f4, $f6
	b	done
c_ueq_s:
	c.ueq.s	$f4, $f6
	b	done
c_olt_s:
	c.olt.s	$f4, $f6
	b	done
c_ult_s:
	c.ult.s	$f4, $f6
	b	done
c_ole_s:
	c.ole.s	$f4, $f6
	b	done
c_ule_s:
	c.ule.s	$f4, $f6
	b	done
c_sf_s:
	c.sf.s	$f4, $f6
	b	done
c_ngle_s:
	c.ngle.s	$f4, $f6
	b	done
c_seq_s:
	c.seq.s	$f4, $f6
	b	done
c_ngl_s:
	c.ngl.s	$f4, $f6
	b	done
c_lt_s:
	c.lt.s	$f4, $f6
	b	done
c_nge_s:
	c.nge.s	$f4, $f6
	b	done
c_le_s:
	c.le.s	$f4, $f6
	b	done
c_ngt_s:
	c.ngt.s	$f4, $f6
	b	done
c_f_d:
	c.f.d	$f4, $f6
	b	done
c_un_d:
	c.un.d	$f4, $f6
	b	done
c_eq_d:
	c.eq.d	$f4, $f6
	b	done
c_ueq_d:
	c.ueq.d	$f4, $f6
	b	done
c_olt_d:
	c.olt.d	$f4, $f6
	b	done
c_ult_d:
	c.ult.d	$f4, $f6
	b	done
c_ole_d:
	c.ole.d	$f4, $f6
	b	done
c_ule_d:
	c.ule.d	$f4, $f6
	b	done
c_sf_d:
	c.sf.d	$f4, $f6
	b	done
c_ngle_d:
	c.ngle.d	$f4, $f6
	b	done
c_seq_d:
	c.seq.d	$f4, $f6
	b	done
c_ngl_d:
	c.ngl.d	$f4, $f6
	b	done
c_lt_d:
	c.lt.d	$f4, $f6
	b	done
c_nge_d:
	c.nge.d	$f4, $f6
	b	done
c_le_d:
	c.le.d	$f4, $f6
	b	done
c_ngt_d:
	c.ngt.d	$f4, $f6
done:
	cfc1	t0, MACH_FPC_CSR
	mfc1	v0, $f0
	mfc1	v1, $f1
	sw	t0, fp_res+8
	sw	v0, fp_res
	sw	v1, fp_res+4
	j	ra
END(dofp)

	.rdata
itab:
	.word	0x46062000	# add.s	f0,f4,f6
	.word	0x46262000	# add.d	f0,f4,f6
	.word	0x46062001	# sub.s	f0,f4,f6
	.word	0x46262001	# sub.d	f0,f4,f6
	.word	0x46062002	# mul.s	f0,f4,f6
	.word	0x46262002	# mul.d	f0,f4,f6
	.word	0x46062003	# div.s	f0,f4,f6
	.word	0x46262003	# div.d	f0,f4,f6
	.word	0x46002005	# abs.s	f0,f4
	.word	0x46202005	# abs.d	f0,f4
	.word	0x46002006	# mov.s	f0,f4
	.word	0x46202006	# mov.d	f0,f4
	.word	0x46002007	# neg.s	f0,f4,f0
	.word	0x46202007	# neg.d	f0,f4,f0
	.word	0x46202020	# cvt.s.d	f0,f4
	.word	0x46802020	# cvt.s.w	f0,f4
	.word	0x46002021	# cvt.d.s	f0,f4
	.word	0x46802021	# cvt.d.w	f0,f4
	.word	0x46002024	# cvt.w.s	f0,f4
	.word	0x46202024	# cvt.w.d	f0,f4
	.word	0x46062030	# c.f.s	f4,f6
	.word	0x46062031	# c.un.s	f4,f6
	.word	0x46062032	# c.eq.s	f4,f6
	.word	0x46062033	# c.ueq.s	f4,f6
	.word	0x46062034	# c.olt.s	f4,f6
	.word	0x46062035	# c.ult.s	f4,f6
	.word	0x46062036	# c.ole.s	f4,f6
	.word	0x46062037	# c.ule.s	f4,f6
	.word	0x46062038	# c.sf.s	f4,f6
	.word	0x46062039	# c.ngle.s	f4,f6
	.word	0x4606203a	# c.seq.s	f4,f6
	.word	0x4606203b	# c.ngl.s	f4,f6
	.word	0x4606203c	# c.lt.s	f4,f6
	.word	0x4606203d	# c.nge.s	f4,f6
	.word	0x4606203e	# c.le.s	f4,f6
	.word	0x4606203f	# c.ngt.s	f4,f6
	.word	0x46262030	# c.f.d	f4,f6
	.word	0x46262031	# c.un.d	f4,f6
	.word	0x46262032	# c.eq.d	f4,f6
	.word	0x46262033	# c.ueq.d	f4,f6
	.word	0x46262034	# c.olt.d	f4,f6
	.word	0x46262035	# c.ult.d	f4,f6
	.word	0x46262036	# c.ole.d	f4,f6
	.word	0x46262037	# c.ule.d	f4,f6
	.word	0x46262038	# c.sf.d	f4,f6
	.word	0x46262039	# c.ngle.d	f4,f6
	.word	0x4626203a	# c.seq.d	f4,f6
	.word	0x4626203b	# c.ngl.d	f4,f6
	.word	0x4626203c	# c.lt.d	f4,f6
	.word	0x4626203d	# c.nge.d	f4,f6
	.word	0x4626203e	# c.le.d	f4,f6
	.word	0x4626203f	# c.ngt.d	f4,f6
	.text

NON_LEAF(emfp, STAND_FRAME_SIZE, ra)
	subu	sp, sp, STAND_FRAME_SIZE
	sw	ra, STAND_RA_OFFSET(sp)
	.mask	0x80000000, (STAND_RA_OFFSET - STAND_FRAME_SIZE)

	cfc1	v0, MACH_FPC_CSR
	lw	t1, STAND_FRAME_SIZE+16(sp)	# get 4th arg
	mtc1	a1, $f4
	mtc1	a2, $f5
	mtc1	a3, $f6
	mtc1	t1, $f7
	ctc1	zero, MACH_FPC_CSR
	sll	a0, a0, 2	# index into instruction table
	lw	a0, itab(a0)

	jal	MachEmulateFP

	cfc1	t0, MACH_FPC_CSR
	mfc1	v0, $f0
	mfc1	v1, $f1
	sw	t0, em_res+8
	sw	v0, em_res
	sw	v1, em_res+4
	lw	ra, STAND_RA_OFFSET(sp)
	addu	sp, sp, STAND_FRAME_SIZE
	j	ra
END(emfp)
