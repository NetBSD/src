#	$NetBSD: rand.inc,v 1.4 2018/09/23 13:33:04 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/rand/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/rand


RAND_SRCS = \
drbg_ctr.c \
drbg_lib.c \
rand_egd.c \
rand_err.c \
rand_lib.c \
rand_unix.c \
rand_vms.c \
rand_win.c \
randfile.c
SRCS += ${RAND_SRCS}

.for cryptosrc in ${RAND_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rand
.endfor
