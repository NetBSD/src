#	$NetBSD: ffc.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/ffc

FFC_SRCS += \
ffc_backend.c \
ffc_dh.c \
ffc_key_generate.c \
ffc_key_validate.c \
ffc_params.c \
ffc_params_generate.c \
ffc_params_validate.c \

SRCS += ${FFC_SRCS}

.for cryptosrc in ${FFC_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ffc ${FFCCPPFLAGS}
.endfor
