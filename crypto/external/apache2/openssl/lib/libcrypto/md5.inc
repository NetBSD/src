#	$NetBSD: md5.inc,v 1.1 2025/07/17 14:25:25 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/md5

MD5_SRCS += \
md5_dgst.c \
md5_one.c \
md5_sha1.c \

SRCS += ${MD5_SRCS}

.for cryptosrc in ${MD5_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/md5 ${MD5CPPFLAGS}
.endfor
