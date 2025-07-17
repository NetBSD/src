#	$NetBSD: rand.inc,v 1.1 2025/07/17 14:25:26 christos Exp $

.PATH.c:	${OPENSSLSRC}/crypto/rand

RAND_SRCS += \
prov_seed.c \
rand_deprecated.c \
rand_err.c \
rand_lib.c \
rand_meth.c \
rand_pool.c \
rand_uniform.c \
randfile.c \

SRCS += ${RAND_SRCS}

.for cryptosrc in ${RAND_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/rand ${RANDCPPFLAGS}
.endfor
