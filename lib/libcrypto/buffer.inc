#	$NetBSD: buffer.inc,v 1.2 2000/06/21 19:34:24 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/buffer/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/buffer

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/buffer

SRCS+= buffer.c buf_err.c


