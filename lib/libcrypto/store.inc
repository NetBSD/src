#	$NetBSD: store.inc,v 1.2.10.1 2008/01/09 01:34:41 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ec/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/store


STORE_SRCS = str_err.c str_lib.c str_locl.h str_mem.c str_meth.c 
SRCS += ${STORE_SRCS}

.for cryptosrc in ${STORE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/store
.endfor
