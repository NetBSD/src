#	$NetBSD: md4.inc,v 1.1.4.2 2002/08/29 16:49:35 he Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md4/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md4

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/md4

SRCS+=	md4_dgst.c md4_one.c
