#	$NetBSD: comp.inc,v 1.1 2025/07/17 14:25:24 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/comp

COMP_SRCS += \
c_brotli.c \
c_zlib.c \
c_zstd.c \
comp_err.c \
comp_lib.c \

SRCS += ${COMP_SRCS}

.for cryptosrc in ${COMP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/comp ${COMPCPPFLAGS}
.endfor
