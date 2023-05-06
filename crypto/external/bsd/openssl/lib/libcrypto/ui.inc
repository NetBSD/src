#	$NetBSD: ui.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

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
