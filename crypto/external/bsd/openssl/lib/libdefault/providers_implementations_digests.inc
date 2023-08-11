#	$NetBSD: providers_implementations_digests.inc,v 1.1.2.2 2023/08/11 13:42:14 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/digests

PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS += \
blake2_prov.c \
blake2b_prov.c \
blake2s_prov.c \
md5_prov.c \
md5_sha1_prov.c \
null_prov.c \
ripemd_prov.c \
sha2_prov.c \
sha3_prov.c \
sm3_prov.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}

.for defaultsrc in ${PROVIDERS_IMPLEMENTATIONS_DIGESTS_SRCS}
CPPFLAGS.${defaultsrc} += -I${OPENSSLSRC}/providers/implementations/digests ${PROVIDERS_IMPLEMENTATIONS_DIGESTSCPPFLAGS}
.endfor
