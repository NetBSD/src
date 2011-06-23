#	$NetBSD: Makefile.inc,v 1.1 2011/06/23 12:09:30 mrg Exp $

SRCS+= \
	bdiv_q.c \
	bdiv_qr.c \
	div_q.c \
	pow_1.c \
	powm.c \
	powm_sec.c \
	powlo.c \
	toom_eval_pm2.c \
	toom_eval_pm2exp.c \
	toom_eval_pm2rexp.c \
	mu_bdiv_qr.c \
	mu_div_qr.c \
	sqr.c \
	gcd_lehmer.c \
	gcd_subdiv_step.c \
	gcdext_lehmer.c \
	gcdext_subdiv_step.c \
	perfpow.c \
	gcdext_1.c \
	mu_bdiv_q.c \
	mu_bdiv_qr.c \
	mu_div_q.c \
	mu_div_qr.c \
	mu_divappr_q.c \
	redc_1_sec.c \
	redc_2.c \
	redc_n.c \
	subcnd_n.c \
	trialdiv.c

ASM_SRCS_LIST= \
	add_n			mpn/x86_64/aors_n.asm \
	addaddmul_1msb0		mpn/x86_64/addaddmul_1msb0.asm \
	addlsh1_n		mpn/x86_64/aorrlsh1_n.asm \
	addlsh2_n		mpn/x86_64/aorrlsh2_n.asm \
	addlsh_n		mpn/x86_64/aorrlsh_n.asm \
	addmul_2		mpn/x86_64/addmul_2.asm \
	bdiv_q_1		mpn/x86_64/bdiv_q_1.asm \
	bdiv_dbm1c		mpn/x86_64/bdiv_dbm1c.asm \
	lshiftc			mpn/x86_64/lshiftc.asm \
	mul_1			mpn/x86_64/mul_1.asm \
	mul_2			mpn/x86_64/mul_2.asm \
	rsblsh1_n		mpn/x86_64/aorrlsh1_n.asm \
	rsblsh2_n		mpn/x86_64/aorrlsh2_n.asm \
	rsblsh_n		mpn/x86_64/aorrlsh_n.asm \
	rsh1add_n		mpn/x86_64/rsh1aors_n.asm \
	rsh1sub_n		mpn/x86_64/rsh1aors_n.asm \
	rshift			mpn/x86_64/rshift.asm \
	sub_n			mpn/x86_64/aors_n.asm \
	sublsh1_n		mpn/x86_64/sublsh1_n.asm \
	invert_limb		mpn/x86_64/invert_limb.asm \
	mode1o			mpn/x86_64/mode1o.asm \
	and_n			mpn/x86_64/logops_n.asm \

# XXX using redc_1.c
#	redc_1			mpn/x86_64/redc_1.asm
SRCS+=	redc_1.c

gen-trialdivtab: gen-trialdivtab.c
	${HOST_CC} -o ${.OBJDIR}/gen-trialdivtab ${.ALLSRC} -lm

# XXX
GMP_LIMB_BITS=64

trialdiv.c: trialdivtab.h
trialdivtab.h: gen-trialdivtab
	${.OBJDIR}/gen-trialdivtab $(GMP_LIMB_BITS) 8000 > ${.OBJDIR}/trialdivtab.h

CLEANFILES+=	trialdivtab.h

# XXX stuff looks for "../config.m4"
dummy:
	mkdir dummy
${.OBJDIR}/config.m4: ${.CURDIR}/arch/${MACHINE_ARCH}/config.m4
	cp ${.CURDIR}/arch/${MACHINE_ARCH}/config.m4 ${.TARGET}

.for _build _src in ${ASM_SRCS_LIST}
${_build}.s: Makefile dummy ${.OBJDIR}/config.m4
	${TOOL_M4} -DCONFIG_TOP_SRCDIR=${DIST} -I${.CURDIR} -I${.OBJDIR}/dummy -I${DIST}/ -I${DIST}/mpn -DOPERATION_${.TARGET:R} ${DIST}/${_src} > ${.TARGET}

SRCS+=	${_build}.s

.endfor
