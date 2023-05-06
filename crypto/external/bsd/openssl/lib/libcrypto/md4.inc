#	$NetBSD: md4.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/md4

MD4_SRCS += \
md4_dgst.c \
md4_one.c \

SRCS += ${MD4_SRCS}

.for cryptosrc in ${MD4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/md4 ${MD4CPPFLAGS}
.endfor
