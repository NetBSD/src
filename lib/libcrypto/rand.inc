#	$NetBSD: rand.inc,v 1.6 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/rand/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/rand


RAND_SRCS = md_rand.c randfile.c rand_lib.c rand_err.c rand_egd.c \
	rand_win.c rand_unix.c rand_os2.c
SRCS += ${RAND_SRCS}

.for cryptosrc in ${RAND_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/rand
.endfor
