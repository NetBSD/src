#	$NetBSD: store.inc,v 1.1 2005/11/25 19:15:08 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/store

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/store

SRCS+=	str_err.c str_lib.c str_locl.h str_mem.c str_meth.c 
