#	$NetBSD: srp.inc,v 1.1.2.2 2011/06/23 14:18:09 cherry Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/srp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/srp


SRP_SRCS = srp_lib.c srp_vfy.c
SRCS += ${SRP_SRCS}

.for cryptosrc in ${SRP_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/srp
.endfor
