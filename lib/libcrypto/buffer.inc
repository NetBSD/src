#	$NetBSD: buffer.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/buffer/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/buffer

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/buffer

SRCS+= buffer.c buf_err.c


