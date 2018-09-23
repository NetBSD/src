#	$NetBSD: curve448.inc,v 1.2 2018/09/23 21:43:00 christos Exp $
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
COPTS.curve448.c+=-Wno-error=stack-protector
COPTS.eddsa.c+=-Wno-error=stack-protector
COPTS.f_generic.c+=-Wno-error=stack-protector

.for cryptosrc in ${CURVE448_SRCS}
CPPFLAGS.${cryptosrc} = -I${CURVE448} -I${CURVE448}/arch_32 ${CURVE448CPPFLAGS}
.endfor
