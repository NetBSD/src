#	$NetBSD: kdf.inc,v 1.1.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/kdf

KDF_SRCS += \
kdf_err.c \

SRCS += ${KDF_SRCS}

.for cryptosrc in ${KDF_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/kdf ${KDFCPPFLAGS}
.endfor
