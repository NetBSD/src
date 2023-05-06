#	$NetBSD: kdf.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/kdf

KDF_SRCS += \
kdf_err.c \

SRCS += ${KDF_SRCS}

.for cryptosrc in ${KDF_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/kdf ${KDFCPPFLAGS}
.endfor
