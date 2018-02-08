#	$NetBSD: comp.inc,v 1.2 2018/02/08 21:57:24 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/comp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/comp


COMP_SRCS = comp_lib.c comp_err.c c_zlib.c
SRCS += ${COMP_SRCS}

.for cryptosrc in ${COMP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/comp
.endfor
