#	$NetBSD: conf.inc,v 1.2.10.1 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/conf

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
