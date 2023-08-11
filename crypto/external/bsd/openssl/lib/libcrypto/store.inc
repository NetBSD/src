#	$NetBSD: store.inc,v 1.2.10.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/store

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
