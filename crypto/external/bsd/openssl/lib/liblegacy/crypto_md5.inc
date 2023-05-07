#	$NetBSD: crypto_md5.inc,v 1.1 2023/05/07 16:22:10 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/md5

CRYPTO_MD5_SRCS += \
md5_dgst.c \
md5_one.c \
md5_sha1.c \

SRCS += ${CRYPTO_MD5_SRCS}

.for legacysrc in ${CRYPTO_MD5_SRCS}
CPPFLAGS.${legacysrc} += -I${OPENSSLSRC}/crypto/md5 ${CRYPTO_MD5CPPFLAGS}
.endfor
