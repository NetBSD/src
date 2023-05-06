#	$NetBSD: md5.inc,v 1.3 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/md5

MD5_SRCS += \
md5_dgst.c \
md5_one.c \
md5_sha1.c \

SRCS += ${MD5_SRCS}

.for cryptosrc in ${MD5_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/md5 ${MD5CPPFLAGS}
.endfor
