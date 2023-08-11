#	$NetBSD: ui.inc,v 1.2.10.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/ui

UI_SRCS += \
ui_err.c \
ui_lib.c \
ui_null.c \
ui_openssl.c \
ui_util.c \

SRCS += ${UI_SRCS}

.for cryptosrc in ${UI_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ui ${UICPPFLAGS}
.endfor
