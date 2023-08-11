#	$NetBSD: md4.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/md4

MD4_SRCS += \
md4_dgst.c \
md4_one.c \

SRCS += ${MD4_SRCS}

.for cryptosrc in ${MD4_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/md4 ${MD4CPPFLAGS}
.endfor
