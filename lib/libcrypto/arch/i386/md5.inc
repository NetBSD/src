#	$NetBSD: md5.inc,v 1.1 2000/07/31 19:08:02 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md5/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md5
.PATH:	${.CURDIR}/arch/i386

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/md5
CPPFLAGS+=	-DMD5_ASM

SRCS+=md5_dgst.c md5_dgst_586.S md5_one.c
