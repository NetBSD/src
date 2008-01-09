#	$NetBSD: ecdh.inc,v 1.2.10.1 2008/01/09 01:34:32 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ec/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/ecdh


ECDH_SRCS = ech_err.c ech_key.c ech_lib.c ech_ossl.c 
SRCS += ${ECDH_SRCS}

.for cryptosrc in ${ECDH_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ecdh
.endfor
