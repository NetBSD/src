#	$NetBSD: ffc.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ffc

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
