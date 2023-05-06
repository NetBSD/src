#	$NetBSD: providers_common.inc,v 1.1 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/providers/common

PROVIDERS_COMMON_SRCS += \
bio_prov.c \
capabilities.c \
digest_to_nid.c \
provider_seeding.c \
provider_util.c \
securitycheck.c \
securitycheck_default.c \

SRCS += ${PROVIDERS_COMMON_SRCS}

.for defaultsrc in ${PROVIDERS_COMMON_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/common ${PROVIDERS_COMMONCPPFLAGS}
.endfor
