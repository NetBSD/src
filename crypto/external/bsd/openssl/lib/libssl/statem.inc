#	$NetBSD: statem.inc,v 1.1 2018/02/08 21:57:29 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/statem

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/statem

SRCS+= \
statem.c \
statem_clnt.c \
statem_dtls.c \
statem_lib.c \
statem_srvr.c 
