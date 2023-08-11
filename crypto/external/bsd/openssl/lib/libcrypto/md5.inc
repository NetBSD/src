#	$NetBSD: md5.inc,v 1.2.26.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/md5

MD5_SRCS += \
md5_dgst.c \
md5_one.c \
md5_sha1.c \

SRCS += ${MD5_SRCS}

.for cryptosrc in ${MD5_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/md5 ${MD5CPPFLAGS}
.endfor
