#	$NetBSD: ripemd.inc,v 1.1.1.1.2.1 2000/06/22 06:55:05 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/ripemd/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ripemd

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ripemd

SRCS+=rmd_dgst.c rmd_one.c $(RMD160_ASM_OBJ)


