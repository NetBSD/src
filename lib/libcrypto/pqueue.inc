#	$NetBSD: pqueue.inc,v 1.3 2007/12/09 22:44:20 adrianp Exp $
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
