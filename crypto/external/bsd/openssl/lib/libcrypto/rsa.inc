#	$NetBSD: rsa.inc,v 1.3 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/rsa/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/rsa


RSA_SRCS = \
rsa_ameth.c \
rsa_asn1.c \
rsa_chk.c \
rsa_crpt.c \
rsa_depr.c \
rsa_err.c \
rsa_gen.c \
rsa_lib.c \
rsa_meth.c \
rsa_none.c \
rsa_null.c \
rsa_oaep.c \
rsa_ossl.c \
rsa_pk1.c \
rsa_pmeth.c \
rsa_prn.c \
rsa_pss.c \
rsa_saos.c \
rsa_sign.c \
rsa_ssl.c \
rsa_x931.c \
rsa_x931g.c

SRCS += ${RSA_SRCS}

.for cryptosrc in ${RSA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rsa
.endfor
