#	$NetBSD: pkcs7.inc,v 1.1.1.1.2.2 2000/07/17 06:00:00 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/pkcs7/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pkcs7

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/pkcs7

SRCS+= pk7_lib.c pkcs7err.c pk7_doit.c pk7_smime.c pk7_attr.c pk7_mime.c


