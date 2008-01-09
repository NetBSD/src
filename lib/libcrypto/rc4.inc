#	$NetBSD: rc4.inc,v 1.5.10.1 2008/01/09 01:34:39 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/rc4/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/rc4


RC4_SRCS = rc4_skey.c rc4_enc.c 
SRCS += ${RC4_SRCS}

.for cryptosrc in ${RC4_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rc4
.endfor
