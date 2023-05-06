#	$NetBSD: providers_common_der.inc,v 1.1 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common/der

PROVIDERS_COMMON_DER_SRCS += \
der_ec_gen.c \
der_digests_gen.c \
der_dsa_gen.c \
der_dsa_key.c \
der_dsa_sig.c \
der_ec_key.c \
der_ec_sig.c \
der_ecx_gen.c \
der_ecx_key.c \
der_rsa_gen.c \
der_rsa_key.c \
der_wrap_gen.c \

SRCS += ${PROVIDERS_COMMON_DER_SRCS}

.for commonsrc in ${PROVIDERS_COMMON_DER_SRCS}
CPPFLAGS.${commonsrc} += -I${OPENSSLSRC}/providers/common/der ${PROVIDERS_COMMON_DERCPPFLAGS}
.endfor
