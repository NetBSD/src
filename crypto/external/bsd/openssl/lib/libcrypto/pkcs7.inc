#	$NetBSD: pkcs7.inc,v 1.2.24.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/pkcs7

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
