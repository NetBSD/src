#	$NetBSD: ui.inc,v 1.1 2003/07/24 14:16:33 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/ui/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/ui

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/ui

SRCS+=	ui_err.c ui_lib.c ui_openssl.c ui_util.c $(COMPATSRC)
