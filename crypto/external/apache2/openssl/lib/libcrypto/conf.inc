#	$NetBSD: conf.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/conf

CONF_SRCS += \
conf_api.c \
conf_def.c \
conf_err.c \
conf_lib.c \
conf_mall.c \
conf_mod.c \
conf_sap.c \
conf_ssl.c \

SRCS += ${CONF_SRCS}

.for cryptosrc in ${CONF_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/conf ${CONFCPPFLAGS}
.endfor
