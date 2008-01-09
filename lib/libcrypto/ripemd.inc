#	$NetBSD: ripemd.inc,v 1.5.10.1 2008/01/09 01:34:39 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ripemd/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ripemd


RIPEMD_SRCS = rmd_dgst.c rmd_one.c
SRCS += ${RIPEMD_SRCS}

.for cryptosrc in ${RIPEMD_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ripemd
.endfor
