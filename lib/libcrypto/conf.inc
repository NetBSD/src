#	$NetBSD: conf.inc,v 1.1.1.1.2.3 2002/08/29 16:39:27 he Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/conf/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/conf

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/conf

SRCS+=	conf_err.c conf_lib.c conf_api.c conf_def.c
