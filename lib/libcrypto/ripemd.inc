#	$NetBSD: ripemd.inc,v 1.2.2.2 2000/06/23 16:16:25 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/ripemd/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ripemd

CFLAGS+=	-I${OPENSSLSRC}/crypto/ripemd

SRCS+=rmd_dgst.c rmd_one.c $(RMD160_ASM_OBJ)


