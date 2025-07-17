#	$NetBSD: kdf.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/kdf

KDF_SRCS += \
kdf_err.c \

SRCS += ${KDF_SRCS}

.for cryptosrc in ${KDF_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/kdf ${KDFCPPFLAGS}
.endfor
