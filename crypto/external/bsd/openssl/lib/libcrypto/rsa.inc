#	$NetBSD: rsa.inc,v 1.5 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/rsa

RSA_SRCS += \
rsa_ameth.c \
rsa_asn1.c \
rsa_backend.c \
rsa_chk.c \
rsa_crpt.c \
rsa_depr.c \
rsa_err.c \
rsa_gen.c \
rsa_lib.c \
rsa_meth.c \
rsa_mp.c \
rsa_mp_names.c \
rsa_none.c \
rsa_oaep.c \
rsa_ossl.c \
rsa_pk1.c \
rsa_pmeth.c \
rsa_prn.c \
rsa_pss.c \
rsa_saos.c \
rsa_schemes.c \
rsa_sign.c \
rsa_sp800_56b_check.c \
rsa_sp800_56b_gen.c \
rsa_x931.c \
rsa_x931g.c \

SRCS += ${RSA_SRCS}

.for cryptosrc in ${RSA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/rsa ${RSACPPFLAGS}
.endfor
