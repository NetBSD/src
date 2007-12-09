#	$NetBSD: ecdh.inc,v 1.3 2007/12/09 22:44:16 adrianp Exp $
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
