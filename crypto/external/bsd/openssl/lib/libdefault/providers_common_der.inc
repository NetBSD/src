#	$NetBSD: providers_common_der.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

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
