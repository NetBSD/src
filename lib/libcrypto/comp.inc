#	$NetBSD: comp.inc,v 1.1.1.1.2.1 2000/06/22 06:55:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/comp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/comp

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/comp

SRCS+=	comp_lib.c \
	c_rle.c c_zlib.c


