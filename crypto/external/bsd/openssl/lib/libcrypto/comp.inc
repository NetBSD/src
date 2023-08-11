#	$NetBSD: comp.inc,v 1.2.14.1 2023/08/11 13:41:54 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/comp

COMP_SRCS += \
c_zlib.c \
comp_err.c \
comp_lib.c \

SRCS += ${COMP_SRCS}

.for cryptosrc in ${COMP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/comp ${COMPCPPFLAGS}
.endfor
