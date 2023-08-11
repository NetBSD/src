#	$NetBSD: providers_implementations_kdfs.inc,v 1.1.2.2 2023/08/11 13:42:15 martin Exp $

.PATH:	${OPENSSLSRC}/providers/implementations/kdfs

PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS += \
pbkdf1.c \

SRCS += ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}

.for legacysrc in ${PROVIDERS_IMPLEMENTATIONS_KDFS_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/providers/implementations/kdfs ${PROVIDERS_IMPLEMENTATIONS_KDFSCPPFLAGS}
.endfor
