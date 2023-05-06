#	$NetBSD: srp.inc,v 1.2 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/srp

SRP_SRCS += \
srp_lib.c \
srp_vfy.c \

SRCS += ${SRP_SRCS}

.for cryptosrc in ${SRP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/srp ${SRPCPPFLAGS}
.endfor
