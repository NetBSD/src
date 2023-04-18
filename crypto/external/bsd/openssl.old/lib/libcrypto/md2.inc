#	$NetBSD: md2.inc,v 1.1.1.2 2023/04/18 14:19:04 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/md2/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/md2


#MD2_SRCS = md2_dgst.c md2_one.c
#SRCS += ${MD2_SRCS}

.for cryptosrc in ${MD2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/md2
.endfor
