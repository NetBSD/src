#	$NetBSD: dsa.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/dsa/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dsa

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dsa

SRCS+= dsa_gen.c dsa_key.c dsa_lib.c dsa_asn1.c dsa_vrf.c dsa_sign.c dsa_err.c


