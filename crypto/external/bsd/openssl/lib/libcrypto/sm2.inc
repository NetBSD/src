#	$NetBSD: sm2.inc,v 1.1.2.2 2018/09/30 01:44:52 pgoyette Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/sm2/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/sm2

SM2_SRCS += \
sm2_crypt.c \
sm2_err.c \
sm2_pmeth.c \
sm2_sign.c 

SRCS += ${SM2_SRCS}

.for cryptosrc in ${SM2_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sha ${SM3CPPFLAGS}
.endfor
