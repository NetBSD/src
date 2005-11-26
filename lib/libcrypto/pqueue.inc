#	$NetBSD: pqueue.inc,v 1.1 2005/11/26 00:35:58 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/evp/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/pqueue

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/pqueue

SRCS+=	pqueue.c
