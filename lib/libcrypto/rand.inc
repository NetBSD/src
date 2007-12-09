#	$NetBSD: rand.inc,v 1.7 2007/12/09 22:44:20 adrianp Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/rand/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/rand


RAND_SRCS = md_rand.c randfile.c rand_lib.c rand_err.c rand_egd.c \
	rand_win.c rand_unix.c rand_os2.c
SRCS += ${RAND_SRCS}

.for cryptosrc in ${RAND_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rand
.endfor
