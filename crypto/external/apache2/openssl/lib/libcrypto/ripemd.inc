#	$NetBSD: ripemd.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/ripemd

RIPEMD_SRCS += \
rmd_dgst.c \
rmd_one.c \

SRCS += ${RIPEMD_SRCS}

.for cryptosrc in ${RIPEMD_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/ripemd ${RIPEMDCPPFLAGS}
.endfor
