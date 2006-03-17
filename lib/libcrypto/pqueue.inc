#	$NetBSD: pqueue.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/evp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pqueue


PQUEUE_SRCS = pqueue.c
SRCS += ${PQUEUE_SRCS}

.for cryptosrc in ${PQUEUE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/pqueue
.endfor
