#	$NetBSD: dso.inc,v 1.2 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/dso/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/dso


DSO_SRCS = dso_dl.c dso_dlfcn.c dso_err.c dso_lib.c dso_null.c \
	dso_openssl.c dso_win32.c dso_vms.c
SRCS += ${DSO_SRCS}

.for cryptosrc in ${DSO_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/dso
.endfor
