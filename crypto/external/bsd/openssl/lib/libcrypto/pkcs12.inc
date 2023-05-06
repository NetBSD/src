#	$NetBSD: pkcs12.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/pkcs12

PKCS12_SRCS += \
p12_add.c \
p12_asn.c \
p12_attr.c \
p12_crpt.c \
p12_crt.c \
p12_decr.c \
p12_init.c \
p12_key.c \
p12_kiss.c \
p12_mutl.c \
p12_npas.c \
p12_p8d.c \
p12_p8e.c \
p12_sbag.c \
p12_utl.c \
pk12err.c \

SRCS += ${PKCS12_SRCS}

.for cryptosrc in ${PKCS12_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/pkcs12 ${PKCS12CPPFLAGS}
.endfor
