#	$NetBSD: dsa.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/dsa/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/dsa


DSA_SRCS = \
dsa_ameth.c \
dsa_asn1.c \
dsa_depr.c \
dsa_err.c \
dsa_gen.c \
dsa_key.c \
dsa_lib.c \
dsa_meth.c \
dsa_ossl.c \
dsa_pmeth.c \
dsa_prn.c \
dsa_sign.c \
dsa_vrf.c

SRCS += ${DSA_SRCS}

.for cryptosrc in ${DSA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dsa
.endfor
