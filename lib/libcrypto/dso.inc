#	$NetBSD: dso.inc,v 1.1 2001/04/12 07:48:04 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dso/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dso

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/dso

SRCS+=	dso_dl.c dso_dlfcn.c dso_err.c dso_lib.c dso_null.c \
	dso_openssl.c dso_win32.c dso_vms.c
