#	$NetBSD: md2.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/md2/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/md2


MD2_SRCS = md2_dgst.c md2_one.c
SRCS += ${MD2_SRCS}

.for cryptosrc in ${MD2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/md2
.endfor
