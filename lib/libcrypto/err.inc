#	$NetBSD: err.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/err/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/err


ERR_SRCS = err.c err_all.c err_prn.c
SRCS += ${ERR_SRCS}

.for cryptosrc in ${ERR_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/err
.endfor
