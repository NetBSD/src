#	$NetBSD: dsa.inc,v 1.2.2.2 2000/06/23 16:16:22 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/dsa/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dsa

CFLAGS+=	-I${OPENSSLSRC}/crypto/dsa

SRCS+= dsa_gen.c dsa_key.c dsa_lib.c dsa_asn1.c dsa_vrf.c dsa_sign.c dsa_err.c


