#	$NetBSD: pem.inc,v 1.2.2.2 2000/06/23 16:16:24 minoura Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/pem/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pem

CFLAGS+=	-I${OPENSSLSRC}/crypto/pem

SRCS+=	pem_sign.c pem_seal.c pem_info.c pem_lib.c pem_all.c pem_err.c


