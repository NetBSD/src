#	$NetBSD: comp.inc,v 1.2.2.2 2000/06/23 16:16:21 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/comp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/comp

CFLAGS+=	-I${OPENSSLSRC}/crypto/comp

SRCS+=	comp_lib.c \
	c_rle.c c_zlib.c


