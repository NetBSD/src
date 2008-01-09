#	$NetBSD: comp.inc,v 1.5.10.1 2008/01/09 01:34:30 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/comp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/comp


COMP_SRCS = comp_lib.c comp_err.c \
	c_rle.c c_zlib.c
SRCS += ${COMP_SRCS}

.for cryptosrc in ${COMP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/comp
.endfor
