#	$NetBSD: store.inc,v 1.1.48.1 2019/06/10 21:41:55 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/store/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/store


STORE_SRCS += \
loader_file.c \
store_init.c \
store_err.c \
store_lib.c \
store_register.c \
store_strings.c

SRCS += ${STORE_SRCS}

.for cryptosrc in ${STORE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/store
.endfor
