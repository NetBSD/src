#	$NetBSD: dsa.inc,v 1.3 2000/07/16 07:16:22 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dsa/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dsa

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dsa

SRCS+= dsa_gen.c dsa_key.c dsa_lib.c dsa_asn1.c dsa_vrf.c dsa_sign.c \
	dsa_err.c dsa_ossl.c


