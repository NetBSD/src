#	$NetBSD: ssl.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/ssl

SSL_SRCS += \
s3_cbc.c \

SRCS += ${SSL_SRCS}

.for defaultsrc in ${SSL_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/ssl ${SSLCPPFLAGS}
.endfor
