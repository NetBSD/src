#	$NetBSD: ripemd.inc,v 1.3 2000/07/16 07:16:24 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ripemd/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ripemd

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ripemd

SRCS+=rmd_dgst.c rmd_one.c $(RMD160_ASM_OBJ)


