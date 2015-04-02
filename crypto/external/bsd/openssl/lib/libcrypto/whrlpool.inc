#	$NetBSD: whrlpool.inc,v 1.1.2.3 2015/04/02 17:47:51 snj Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/whrlpool/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/whrlpool

WHRLPOOL_SRCS = wp_block.c wp_dgst.c # wp_test.c   

SRCS += ${WHRLPOOL_SRCS}

.for cryptosrc in ${WHRLPOOL_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/whrlpool
.endfor
CPPFLAGS.m_wp.c += -I${OPENSSLSRC}/crypto/whrlpool
