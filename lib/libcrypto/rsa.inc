#	$NetBSD: rsa.inc,v 1.3 2000/07/16 07:16:24 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rsa/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rsa

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/rsa

SRCS+= rsa_eay.c rsa_gen.c rsa_lib.c rsa_sign.c rsa_saos.c rsa_err.c \
	rsa_pk1.c rsa_ssl.c rsa_none.c rsa_oaep.c rsa_chk.c rsa_null.c


