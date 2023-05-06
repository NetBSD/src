#	$NetBSD: curve448.inc,v 1.3 2023/05/06 17:07:22 christos Exp $
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/ec/curve448
#

CURVE448=${OPENSSLSRC}/crypto/ec/curve448
.PATH:	${CURVE448} ${CURVE448}/arch_32 ${CURVE448}/arch_64


CURVE448_SRCS += \
curve448.c \
curve448_tables.c \
eddsa.c \
f_generic.c \
f_impl32.c \
f_impl64.c \
scalar.c

SRCS += ${CURVE448_SRCS}
COPTS.curve448.c+=-Wno-error=stack-protector
COPTS.eddsa.c+=-Wno-error=stack-protector
COPTS.f_generic.c+=-Wno-error=stack-protector

.for cryptosrc in ${CURVE448_SRCS}
CPPFLAGS.${cryptosrc} = -I${CURVE448} -I${CURVE448}/arch_32 ${CURVE448CPPFLAGS}
.endfor
