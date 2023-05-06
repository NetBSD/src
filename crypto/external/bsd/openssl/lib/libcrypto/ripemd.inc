#	$NetBSD: ripemd.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/ripemd

RIPEMD_SRCS += \
rmd_dgst.c \
rmd_one.c \

SRCS += ${RIPEMD_SRCS}

.for cryptosrc in ${RIPEMD_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ripemd ${RIPEMDCPPFLAGS}
.endfor
