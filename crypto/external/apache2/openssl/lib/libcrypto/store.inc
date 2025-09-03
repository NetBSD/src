#	$NetBSD: store.inc,v 1.1 2025/07/17 14:25:27 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/store

STORE_SRCS += \
store_err.c \
store_init.c \
store_lib.c \
store_meth.c \
store_register.c \
store_result.c \
store_strings.c \

SRCS += ${STORE_SRCS}

.for cryptosrc in ${STORE_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/store ${STORECPPFLAGS}
.endfor
