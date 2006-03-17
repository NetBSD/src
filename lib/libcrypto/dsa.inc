#	$NetBSD: dsa.inc,v 1.6 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dsa/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dsa


DSA_SRCS = dsa_gen.c dsa_key.c dsa_lib.c dsa_asn1.c dsa_vrf.c dsa_sign.c \
	dsa_err.c dsa_ossl.c dsa_depr.c
SRCS += ${DSA_SRCS}

.for cryptosrc in ${DSA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dsa
.endfor
