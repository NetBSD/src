#	$NetBSD: md4.inc,v 1.2.10.1 2008/01/09 01:34:36 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/md4/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/md4


MD4_SRCS = md4_dgst.c md4_one.c
SRCS += ${MD4_SRCS}

.for cryptosrc in ${MD4_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/md4
.endfor
