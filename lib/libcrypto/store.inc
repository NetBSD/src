#	$NetBSD: store.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/store


STORE_SRCS = str_err.c str_lib.c str_locl.h str_mem.c str_meth.c 
SRCS += ${STORE_SRCS}

.for cryptosrc in ${STORE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/store
.endfor
