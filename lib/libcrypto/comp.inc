#	$NetBSD: comp.inc,v 1.4 2003/07/24 14:16:31 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/comp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/comp

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/comp

SRCS+=	comp_lib.c comp_err.c \
	c_rle.c c_zlib.c
