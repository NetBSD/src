#	$NetBSD: sm4.inc,v 1.1.4.2 2019/06/10 21:41:55 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/sm4/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/sm4

SM4_SRCS += sm4.c 

SRCS += ${SM4_SRCS}

.for cryptosrc in ${SM4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sha ${SM4CPPFLAGS}
.endfor
