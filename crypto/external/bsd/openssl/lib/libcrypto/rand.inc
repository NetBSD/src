#	$NetBSD: rand.inc,v 1.3.4.1 2019/06/10 21:41:55 christos Exp $
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
