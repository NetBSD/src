#	$NetBSD: buffer.inc,v 1.1.1.1.2.2 2000/07/17 05:59:58 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/buffer/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/buffer

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/buffer

SRCS+= buffer.c buf_err.c


