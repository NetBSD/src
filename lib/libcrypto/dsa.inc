#	$NetBSD: dsa.inc,v 1.6.10.1 2008/01/09 01:34:31 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/dsa/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/dsa


DSA_SRCS = dsa_gen.c dsa_key.c dsa_lib.c dsa_asn1.c dsa_vrf.c dsa_sign.c \
	dsa_err.c dsa_ossl.c dsa_depr.c
SRCS += ${DSA_SRCS}

.for cryptosrc in ${DSA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dsa
.endfor
