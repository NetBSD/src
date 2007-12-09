#	$NetBSD: md4.inc,v 1.3 2007/12/09 22:44:18 adrianp Exp $
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
