#	$NetBSD: md4.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/md4

MD4_SRCS += \
md4_dgst.c \
md4_one.c \

SRCS += ${MD4_SRCS}

.for cryptosrc in ${MD4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/md4 ${MD4CPPFLAGS}
.endfor
