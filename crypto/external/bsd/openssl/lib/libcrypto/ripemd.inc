#	$NetBSD: ripemd.inc,v 1.1.58.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/ripemd

RIPEMD_SRCS += \
rmd_dgst.c \
rmd_one.c \

SRCS += ${RIPEMD_SRCS}

.for cryptosrc in ${RIPEMD_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ripemd ${RIPEMDCPPFLAGS}
.endfor
