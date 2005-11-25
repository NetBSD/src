#	$NetBSD: ecdsa.inc,v 1.1 2005/11/25 19:15:08 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ecdsa

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ecdsa

SRCS+=	ecs_asn1.c ecs_err.c ecs_lib.c ecs_locl.h ecs_ossl.c ecs_sign.c \
	ecs_vrf.c 
