#	$NetBSD: bn.inc,v 1.10 2023/05/31 20:57:31 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/bn

BN_SRCS ?= bn_asm.c

BN_SRCS += \
bn_add.c \
bn_blind.c \
bn_const.c \
bn_conv.c \
bn_ctx.c \
bn_depr.c \
bn_dh.c \
bn_div.c \
bn_err.c \
bn_exp.c \
bn_exp2.c \
bn_gcd.c \
bn_gf2m.c \
bn_intern.c \
bn_kron.c \
bn_lib.c \
bn_mod.c \
bn_mont.c \
bn_mpi.c \
bn_mul.c \
bn_nist.c \
bn_prime.c \
bn_print.c \
bn_rand.c \
bn_recp.c \
bn_rsa_fips186_4.c \
bn_shift.c \
bn_sqr.c \
bn_sqrt.c \
bn_srp.c \
bn_word.c \
bn_x931p.c \
rsaz_exp.c \
rsaz_exp_x2.c \

SRCS += ${BN_SRCS}

.for cryptosrc in ${BN_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/bn ${BNCPPFLAGS}
.endfor


LINTFLAGS.bn_nist.c+=	-X 132	# conversion from 'unsigned long' to 'int'
LINTFLAGS.bn_nist.c+=	-X 161	# constant in conditional context
LINTFLAGS.bn_nist.c+=	-X 275	# cast discards 'const' from type 'pointer to const unsigned long'
