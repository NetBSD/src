#	$NetBSD: pem.inc,v 1.3 2000/07/16 07:16:23 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/pem/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pem

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/pem

SRCS+=	pem_sign.c pem_seal.c pem_info.c pem_lib.c pem_all.c pem_err.c


