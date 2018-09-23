#	$NetBSD: aria.inc,v 1.1 2018/09/23 13:33:04 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/aria/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/aria


ARIA_SRCS += aria.c

SRCS += ${ARIA_SRCS}

.for cryptosrc in ${ARIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aria ${ARIACPPFLAGS}
.endfor
