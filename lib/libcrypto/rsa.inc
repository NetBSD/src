#	$NetBSD: rsa.inc,v 1.7.10.1 2008/01/09 01:34:40 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/rsa/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/rsa


RSA_SRCS = rsa_eay.c rsa_gen.c rsa_lib.c rsa_sign.c rsa_saos.c rsa_err.c \
	rsa_pk1.c rsa_ssl.c rsa_none.c rsa_oaep.c rsa_chk.c rsa_null.c \
	rsa_asn1.c rsa_x931.c rsa_depr.c
SRCS += ${RSA_SRCS}

.for cryptosrc in ${RSA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rsa
.endfor
