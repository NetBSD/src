#	$NetBSD: comp.inc,v 1.1.1.1.2.2 2000/07/17 05:59:58 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/comp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/comp

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/comp

SRCS+=	comp_lib.c \
	c_rle.c c_zlib.c


