#	$NetBSD: sm3.inc,v 1.1.1.1 2023/04/18 14:19:04 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/sm3/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/sm3

SM3_SRCS += m_sm3.c sm3.c 

SRCS += ${SM3_SRCS}

.for cryptosrc in ${SM3_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sha ${SM3CPPFLAGS}
.endfor
