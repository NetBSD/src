#	$NetBSD: pem.inc,v 1.2.14.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/pem

PEM_SRCS += \
pem_all.c \
pem_err.c \
pem_info.c \
pem_lib.c \
pem_oth.c \
pem_pk8.c \
pem_pkey.c \
pem_sign.c \
pem_x509.c \
pem_xaux.c \
pvkfmt.c \

SRCS += ${PEM_SRCS}

.for cryptosrc in ${PEM_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/pem ${PEMCPPFLAGS}
.endfor
