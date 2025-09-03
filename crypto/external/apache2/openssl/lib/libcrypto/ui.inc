#	$NetBSD: ui.inc,v 1.1 2025/07/17 14:25:27 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ui

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
