#	$NetBSD: ssl.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/ssl/record/methods

SSL_SRCS += \
ssl3_cbc.c \

SRCS += ${SSL_SRCS}

.for defaultsrc in ${SSL_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/ssl ${SSLCPPFLAGS}
.endfor
