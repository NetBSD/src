#	$NetBSD: buffer.inc,v 1.3 2000/07/16 07:16:21 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/buffer/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/buffer

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/buffer

SRCS+= buffer.c buf_err.c


