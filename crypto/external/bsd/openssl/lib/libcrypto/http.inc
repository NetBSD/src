#	$NetBSD: http.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/http

HTTP_SRCS += \
http_client.c \
http_err.c \
http_lib.c \

SRCS += ${HTTP_SRCS}

.for cryptosrc in ${HTTP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/http ${HTTPCPPFLAGS}
.endfor
