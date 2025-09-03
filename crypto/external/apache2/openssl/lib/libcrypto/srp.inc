#	$NetBSD: srp.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/srp

SRP_SRCS += \
srp_lib.c \
srp_vfy.c \

SRCS += ${SRP_SRCS}

.for cryptosrc in ${SRP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/srp ${SRPCPPFLAGS}
.endfor
