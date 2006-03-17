#	$NetBSD: md4.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md4/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md4


MD4_SRCS = md4_dgst.c md4_one.c
SRCS += ${MD4_SRCS}

.for cryptosrc in ${MD4_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/md4
.endfor
