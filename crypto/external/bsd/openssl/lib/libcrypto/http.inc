#	$NetBSD: http.inc,v 1.1.2.2 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/http

HTTP_SRCS += \
http_client.c \
http_err.c \
http_lib.c \

SRCS += ${HTTP_SRCS}

.for cryptosrc in ${HTTP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/http ${HTTPCPPFLAGS}
.endfor
