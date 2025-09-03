#	$NetBSD: rio.inc,v 1.1 2025/07/17 14:26:12 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/ssl/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/ssl/rio

CPPFLAGS+=	-I${OPENSSLSRC}/ssl/rio

SRCS+= \
poll_builder.c \
poll_immediate.c \
rio_notifier.c \
