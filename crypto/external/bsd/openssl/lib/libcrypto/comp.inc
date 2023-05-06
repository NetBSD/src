#	$NetBSD: comp.inc,v 1.3 2023/05/06 17:07:22 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/comp

COMP_SRCS += \
c_zlib.c \
comp_err.c \
comp_lib.c \

SRCS += ${COMP_SRCS}

.for cryptosrc in ${COMP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/comp ${COMPCPPFLAGS}
.endfor
