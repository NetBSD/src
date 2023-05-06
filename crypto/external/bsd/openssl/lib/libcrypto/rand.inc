#	$NetBSD: rand.inc,v 1.5 2023/05/06 17:07:23 christos Exp $

.PATH:	${OPENSSLSRC}/crypto/rand

RAND_SRCS += \
prov_seed.c \
rand_deprecated.c \
rand_err.c \
rand_lib.c \
rand_meth.c \
rand_pool.c \
randfile.c \

SRCS += ${RAND_SRCS}

.for cryptosrc in ${RAND_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/rand ${RANDCPPFLAGS}
.endfor
