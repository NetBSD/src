#	$NetBSD: aria.inc,v 1.1.1.1 2023/04/18 14:19:03 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/aria/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/aria


ARIA_SRCS += aria.c

SRCS += ${ARIA_SRCS}

.for cryptosrc in ${ARIA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/aria ${ARIACPPFLAGS}
.endfor
