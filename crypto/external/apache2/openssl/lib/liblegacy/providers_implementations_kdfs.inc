#	$NetBSD: providers_implementations_kdfs.inc,v 1.1 2025/07/17 14:26:12 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kdfs

PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS += \
pbkdf1.c \
pvkkdf.c \


SRCS += ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}

.for legacysrc in ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/implementations/kdfs ${PROVIDERS_IMPLEMENTATIONS_KDFSCPPFLAGS}
.endfor
