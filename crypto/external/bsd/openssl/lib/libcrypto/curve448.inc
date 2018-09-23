#	$NetBSD: curve448.inc,v 1.1 2018/09/23 13:33:04 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ec/curve448
#

CURVE448=${OPENSSLSRC}/crypto/ec/curve448
.PATH:	${CURVE448} ${CURVE448}/arch_32


CURVE448_SRCS += \
curve448.c \
curve448_tables.c \
eddsa.c \
f_generic.c \
f_impl.c \
scalar.c

SRCS += ${CURVE448_SRCS}

.for cryptosrc in ${CURVE448_SRCS}
CPPFLAGS.${cryptosrc} = -I${CURVE448} -I${CURVE448}/arch_32 ${CURVE448CPPFLAGS}
.endfor
