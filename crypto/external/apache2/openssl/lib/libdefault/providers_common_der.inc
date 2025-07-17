#	$NetBSD: providers_common_der.inc,v 1.1 2025/07/17 14:26:09 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common/der

PROVIDERS_COMMON_DER_SRCS += \
der_rsa_sig.c \
der_sm2_gen.c \
der_sm2_key.c \
der_sm2_sig.c \

SRCS += ${PROVIDERS_COMMON_DER_SRCS}

.for defaultsrc in ${PROVIDERS_COMMON_DER_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/common/der ${PROVIDERS_COMMON_DERCPPFLAGS}
.endfor
