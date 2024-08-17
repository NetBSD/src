#	$NetBSD: sha.inc,v 1.8 2024/08/17 15:18:30 rin Exp $

.PATH:	${OPENSSLSRC}/crypto/sha


SHA_SRCS += sha1dgst.c sha3.c

# Replaced OpenSSL version to avoid overlap with libc
SHA_SRCS+= libc-sha1.c libc-sha512.c libc-sha256.c libc-sha2xx.c

.if !defined(KECCAKNI)
SHA_SRCS+= keccak1600.c
.endif
#SHA_SRCS += \
#sha1_one.c \
#sha1dgst.c \
#sha256.c \
#sha3.c \
#sha512.c \

SRCS += ${SHA_SRCS}

.for cryptosrc in ${SHA_SRCS}
CPPFLAGS.${cryptosrc} += -I${OPENSSLSRC}/crypto/sha ${SHACPPFLAGS}
.endfor

.if (${MACHINE_ARCH} == "m68k" || ${MACHINE_ARCH} == "m68000") && \
    ${HAVE_GCC:U0} == 12
COPTS.libc-sha2xx.c+=	-fno-stack-protector
.endif
