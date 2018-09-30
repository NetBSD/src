#	$NetBSD: statem.inc,v 1.1.2.1 2018/09/30 01:45:00 pgoyette Exp $
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
