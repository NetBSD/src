#	$NetBSD: statem.inc,v 1.2.10.1 2023/08/11 13:42:15 martin Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/statem

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/statem

SRCS+= \
extensions.c \
extensions_clnt.c \
extensions_cust.c \
extensions_srvr.c \
statem.c \
statem_clnt.c \
statem_dtls.c \
statem_lib.c \
statem_srvr.c
