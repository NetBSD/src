#	$NetBSD: err.inc,v 1.3 2000/07/16 07:16:22 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/err/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/err

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/err

SRCS+=err.c err_all.c err_prn.c


