#	$NetBSD: rsa.inc,v 1.2 2000/06/16 06:16:39 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /usr/src/openssl/crypto/rsa/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rsa

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rsa

SRCS+= rsa_chk.c rsa_eay.c rsa_gen.c rsa_lib.c rsa_sign.c rsa_saos.c \
	rsa_err.c rsa_pk1.c rsa_ssl.c rsa_none.c rsa_oaep.c


