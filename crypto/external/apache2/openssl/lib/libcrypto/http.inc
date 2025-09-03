#	$NetBSD: http.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/http

HTTP_SRCS += \
http_client.c \
http_err.c \
http_lib.c \

SRCS += ${HTTP_SRCS}

.for cryptosrc in ${HTTP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/http ${HTTPCPPFLAGS}
.endfor
