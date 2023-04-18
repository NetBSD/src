#	$NetBSD: store.inc,v 1.1.1.2 2023/04/18 14:19:04 christos Exp $
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
