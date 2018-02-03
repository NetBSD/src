#	$NetBSD: srp.inc,v 1.1.1.1 2018/02/03 22:43:37 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/srp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/srp


SRP_SRCS = srp_lib.c srp_vfy.c
SRCS += ${SRP_SRCS}

.for cryptosrc in ${SRP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/srp
.endfor
