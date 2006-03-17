#	$NetBSD: ecdsa.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ec/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ecdsa


ECDSA_SRCS = ecs_asn1.c ecs_err.c ecs_lib.c ecs_locl.h ecs_ossl.c ecs_sign.c \
	ecs_vrf.c 
SRCS += ${ECDSA_SRCS}

.for cryptosrc in ${ECDSA_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/ecdsa
.endfor
