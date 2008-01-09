#	$NetBSD: rc2.inc,v 1.5.10.1 2008/01/09 01:34:39 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/rc2/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/rc2


RC2_SRCS = rc2_ecb.c rc2_skey.c rc2_cbc.c rc2cfb64.c rc2ofb64.c
SRCS += ${RC2_SRCS}

.for cryptosrc in ${RC2_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rc2
.endfor
