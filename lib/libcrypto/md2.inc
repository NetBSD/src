#	$NetBSD: md2.inc,v 1.3 2000/07/16 07:16:22 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md2

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/md2

SRCS+=md2_dgst.c md2_one.c


