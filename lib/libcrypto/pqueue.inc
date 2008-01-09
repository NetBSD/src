#	$NetBSD: pqueue.inc,v 1.2.10.1 2008/01/09 01:34:38 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/evp/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/pqueue


PQUEUE_SRCS = pqueue.c
SRCS += ${PQUEUE_SRCS}

.for cryptosrc in ${PQUEUE_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/pqueue
.endfor
