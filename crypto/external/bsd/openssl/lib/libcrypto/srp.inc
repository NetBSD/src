#	$NetBSD: srp.inc,v 1.1.56.1 2023/08/11 13:41:55 martin Exp $

.PATH:	${OPENSSLSRC}/crypto/srp

SRP_SRCS += \
srp_lib.c \
srp_vfy.c \

SRCS += ${SRP_SRCS}

.for cryptosrc in ${SRP_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/srp ${SRPCPPFLAGS}
.endfor
