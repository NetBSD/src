#	$NetBSD: providers_implementations_kdfs.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kdfs

PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS += \
pbkdf1.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}

.for legacysrc in ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/implementations/kdfs ${PROVIDERS_IMPLEMENTATIONS_KDFSCPPFLAGS}
.endfor
