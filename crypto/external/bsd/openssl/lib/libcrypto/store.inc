#	$NetBSD: store.inc,v 1.1.46.1 2018/09/30 01:44:52 pgoyette Exp $
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
