#	$NetBSD: Makefile.inc,v 1.1 2011/07/02 13:36:57 mrg Exp $

C_SRCS_LIST= \
	toom6h_mul.c		mpn/generic/toom6h_mul.c \
	toom3_sqr.c		mpn/generic/toom3_sqr.c \
	copyi.c			mpn/generic/copyi.c \
	sqrtrem.c		mpn/generic/sqrtrem.c \
	iorn_n.c		mpn/generic/logops_n.c \
	bdiv_qr.c		mpn/generic/bdiv_qr.c \
	matrix22_mul.c		mpn/generic/matrix22_mul.c \
	powlo.c			mpn/generic/powlo.c \
	tdiv_qr.c		mpn/generic/tdiv_qr.c \
	gcd_1.c			mpn/generic/gcd_1.c \
	sqrmod_bnm1.c		mpn/generic/sqrmod_bnm1.c \
	zero.c			mpn/generic/zero.c \
	cmp.c			mpn/generic/cmp.c \
	toom_couple_handling.c	mpn/generic/toom_couple_handling.c \
	toom32_mul.c		mpn/generic/toom32_mul.c \
	dump.c			mpn/generic/dump.c \
	toom62_mul.c		mpn/generic/toom62_mul.c \
	dive_1.c		mpn/generic/dive_1.c \
	toom44_mul.c		mpn/generic/toom44_mul.c \
	nand_n.c		mpn/generic/logops_n.c \
	hgcd.c			mpn/generic/hgcd.c \
	ior_n.c			mpn/generic/logops_n.c \
	get_d.c			mpn/generic/get_d.c \
	bdiv_dbm1c.c		mpn/generic/bdiv_dbm1c.c \
	nussbaumer_mul.c	mpn/generic/nussbaumer_mul.c \
	pre_divrem_1.c		mpn/generic/pre_divrem_1.c \
	andn_n.c		mpn/generic/logops_n.c \
	dcpi1_div_q.c		mpn/generic/dcpi1_div_q.c \
	sbpi1_div_qr.c		mpn/generic/sbpi1_div_qr.c \
	dcpi1_bdiv_qr.c		mpn/generic/dcpi1_bdiv_qr.c \
	bdiv_q_1.c		mpn/generic/bdiv_q_1.c \
	gcdext_1.c		mpn/generic/gcdext_1.c \
	copyd.c			mpn/generic/copyd.c \
	toom42_mul.c		mpn/generic/toom42_mul.c \
	xnor_n.c		mpn/generic/logops_n.c \
	dcpi1_div_qr.c		mpn/generic/dcpi1_div_qr.c \
	sqr_basecase.c		mpn/generic/sqr_basecase.c \
	toom_interpolate_7pts.c	mpn/generic/toom_interpolate_7pts.c \
	toom63_mul.c		mpn/generic/toom63_mul.c \
	gcdext.c		mpn/generic/gcdext.c \
	mu_divappr_q.c		mpn/generic/mu_divappr_q.c \
	mul_n.c			mpn/generic/mul_n.c \
	gcd_subdiv_step.c	mpn/generic/gcd_subdiv_step.c \
	udiv_w_sdiv.c		mpn/generic/udiv_w_sdiv.c \
	pre_mod_1.c		mpn/generic/pre_mod_1.c \
	toom8h_mul.c		mpn/generic/toom8h_mul.c \
	toom_interpolate_6pts.c	mpn/generic/toom_interpolate_6pts.c \
	mu_div_q.c		mpn/generic/mu_div_q.c \
	add_1.c			mpn/generic/add_1.c \
	hamdist.c		mpn/generic/popham.c \
	powm_sec.c		mpn/generic/powm_sec.c \
	toom22_mul.c		mpn/generic/toom22_mul.c \
	invert.c		mpn/generic/invert.c \
	toom8_sqr.c		mpn/generic/toom8_sqr.c \
	xor_n.c			mpn/generic/logops_n.c \
	mu_bdiv_qr.c		mpn/generic/mu_bdiv_qr.c \
	com.c			mpn/generic/com.c \
	redc_2.c		mpn/generic/redc_2.c \
	toom43_mul.c		mpn/generic/toom43_mul.c \
	subcnd_n.c		mpn/generic/subcnd_n.c \
	invertappr.c		mpn/generic/invertappr.c \
	redc_1.c		mpn/generic/redc_1.c \
	sqr.c			mpn/generic/sqr.c \
	trialdiv.c		mpn/generic/trialdiv.c \
	mod_1_4.c		mpn/generic/mod_1_4.c \
	fib2_ui.c		mpn/generic/fib2_ui.c \
	mu_div_qr.c		mpn/generic/mu_div_qr.c \
	sbpi1_divappr_q.c	mpn/generic/sbpi1_divappr_q.c \
	toom_eval_pm2exp.c	mpn/generic/toom_eval_pm2exp.c \
	perfsqr.c		mpn/generic/perfsqr.c \
	toom6_sqr.c		mpn/generic/toom6_sqr.c \
	mod_1_3.c		mpn/generic/mod_1_3.c \
	dcpi1_divappr_q.c	mpn/generic/dcpi1_divappr_q.c \
	popcount.c		mpn/generic/popham.c \
	mullo_n.c		mpn/generic/mullo_n.c \
	mod_1.c			mpn/generic/mod_1.c \
	scan1.c			mpn/generic/scan1.c \
	random.c		mpn/generic/random.c \
	get_str.c		mpn/generic/get_str.c \
	gcd.c			mpn/generic/gcd.c \
	toom_eval_pm2.c		mpn/generic/toom_eval_pm2.c \
	mod_1_2.c		mpn/generic/mod_1_2.c \
	nior_n.c		mpn/generic/logops_n.c \
	toom53_mul.c		mpn/generic/toom53_mul.c \
	scan0.c			mpn/generic/scan0.c \
	sub_1.c			mpn/generic/sub_1.c \
	hgcd2.c			mpn/generic/hgcd2.c \
	toom_interpolate_12pts.c	mpn/generic/toom_interpolate_12pts.c \
	mulmod_bnm1.c		mpn/generic/mulmod_bnm1.c \
	sub.c			mpn/generic/sub.c \
	rootrem.c		mpn/generic/rootrem.c \
	sbpi1_bdiv_q.c		mpn/generic/sbpi1_bdiv_q.c \
	toom_eval_pm1.c		mpn/generic/toom_eval_pm1.c \
	redc_1_sec.c		mpn/generic/redc_1_sec.c \
	mod_1_1.c		mpn/generic/mod_1_1.c \
	powm.c			mpn/generic/powm.c \
	mul.c			mpn/generic/mul.c \
	divis.c			mpn/generic/divis.c \
	add.c			mpn/generic/add.c \
	toom4_sqr.c		mpn/generic/toom4_sqr.c \
	random2.c		mpn/generic/random2.c \
	lshiftc.c		mpn/generic/lshiftc.c \
	mul_basecase.c		mpn/generic/mul_basecase.c \
	div_q.c			mpn/generic/div_q.c \
	toom_interpolate_8pts.c	mpn/generic/toom_interpolate_8pts.c \
	diveby3.c		mpn/generic/diveby3.c \
	dcpi1_bdiv_q.c		mpn/generic/dcpi1_bdiv_q.c \
	and_n.c			mpn/generic/logops_n.c \
	divexact.c		mpn/generic/divexact.c \
	gcd_lehmer.c		mpn/generic/gcd_lehmer.c \
	redc_n.c		mpn/generic/redc_n.c \
	toom33_mul.c		mpn/generic/toom33_mul.c \
	toom_interpolate_16pts.c	mpn/generic/toom_interpolate_16pts.c \
	set_str.c		mpn/generic/set_str.c \
	toom2_sqr.c		mpn/generic/toom2_sqr.c \
	jacbase.c		mpn/generic/jacbase.c \
	remove.c		mpn/generic/remove.c \
	mode1o.c		mpn/generic/mode1o.c \
	perfpow.c		mpn/generic/perfpow.c \
	toom_eval_dgr3_pm2.c	mpn/generic/toom_eval_dgr3_pm2.c \
	sbpi1_div_q.c		mpn/generic/sbpi1_div_q.c \
	gcdext_subdiv_step.c	mpn/generic/gcdext_subdiv_step.c \
	sbpi1_bdiv_qr.c		mpn/generic/sbpi1_bdiv_qr.c \
	toom_eval_pm2rexp.c	mpn/generic/toom_eval_pm2rexp.c \
	divrem.c		mpn/generic/divrem.c \
	binvert.c		mpn/generic/binvert.c \
	mod_34lsub1.c		mpn/generic/mod_34lsub1.c \
	toom_eval_dgr3_pm1.c	mpn/generic/toom_eval_dgr3_pm1.c \
	mul_fft.c		mpn/generic/mul_fft.c \
	mullo_basecase.c	mpn/generic/mullo_basecase.c \
	divrem_2.c		mpn/generic/divrem_2.c \
	toom52_mul.c		mpn/generic/toom52_mul.c \
	mu_bdiv_q.c		mpn/generic/mu_bdiv_q.c \
	neg.c			mpn/generic/neg.c \
	toom_interpolate_5pts.c	mpn/generic/toom_interpolate_5pts.c \
	divrem_1.c		mpn/generic/divrem_1.c \
	pow_1.c			mpn/generic/pow_1.c \
	bdiv_q.c		mpn/generic/bdiv_q.c \
	gcdext_lehmer.c		mpn/generic/gcdext_lehmer.c \

gen-trialdivtab: gen-trialdivtab.c
	${HOST_CC} -o ${.OBJDIR}/gen-trialdivtab ${.ALLSRC} -lm

GMP_LIMB_BITS=32

DPSRCS+=trialdivtab.h
trialdivtab.h: gen-trialdivtab
	${.OBJDIR}/gen-trialdivtab $(GMP_LIMB_BITS) 8000 > ${.OBJDIR}/trialdivtab.h

CLEANFILES+=	trialdivtab.h

# XXX stuff looks for "../config.m4"
dummy:
	mkdir dummy
${.OBJDIR}/config.m4: ${.CURDIR}/arch/${MACHINE_ARCH}/config.m4
	cp ${.CURDIR}/arch/${MACHINE_ARCH}/config.m4 ${.TARGET}

.for _build _src in ${C_SRCS_LIST}
mpn_${_build}: Makefile
	echo '#include "${_src}"' > ${.TARGET}

CPPFLAGS.mpn_${_build}+= -DOPERATION_${_build:R}

SRCS+=		mpn_${_build}
CLEANFILES+=	mpn_${_build}
.endfor
