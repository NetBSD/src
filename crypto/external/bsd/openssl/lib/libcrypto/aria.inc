#	$NetBSD: aria.inc,v 1.1.4.2 2019/06/10 21:41:55 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/aria/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/aria


ARIA_SRCS += aria.c

SRCS += ${ARIA_SRCS}

.for cryptosrc in ${ARIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aria ${ARIACPPFLAGS}
.endfor
