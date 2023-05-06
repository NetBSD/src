#	$NetBSD: ssl.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/ssl

SSL_SRCS += \
s3_cbc.c \

SRCS += ${SSL_SRCS}

.for defaultsrc in ${SSL_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/ssl ${SSLCPPFLAGS}
.endfor
