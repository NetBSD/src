#	$NetBSD: buffer.inc,v 1.2.2.2 2000/06/23 16:16:21 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/buffer/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/buffer

CFLAGS+=	-I${OPENSSLSRC}/crypto/buffer

SRCS+= buffer.c buf_err.c


