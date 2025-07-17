#	$NetBSD: pkcs7.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/pkcs7

PKCS7_SRCS += \
bio_pk7.c \
pk7_asn1.c \
pk7_attr.c \
pk7_doit.c \
pk7_lib.c \
pk7_mime.c \
pk7_smime.c \
pkcs7err.c \

SRCS += ${PKCS7_SRCS}

.for cryptosrc in ${PKCS7_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/pkcs7 ${PKCS7CPPFLAGS}
.endfor
