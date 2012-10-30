#	$NetBSD: whrlpool.inc,v 1.1.6.2 2012/10/30 18:47:28 yamt Exp $
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
