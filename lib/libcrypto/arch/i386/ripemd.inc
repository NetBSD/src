#	$NetBSD: ripemd.inc,v 1.1 2000/07/31 19:22:04 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ripemd/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ripemd
.PATH:	${.CURDIR}/arch/i386

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ripemd
CPPFLAGS+=	-DRMD160_ASM

SRCS+=rmd_dgst.c rmd_dgst_586.S rmd_one.c
