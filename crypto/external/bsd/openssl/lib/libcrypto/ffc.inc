#	$NetBSD: ffc.inc,v 1.1.2.2 2023/08/11 13:41:55 martin Exp $

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
