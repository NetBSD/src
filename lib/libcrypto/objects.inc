#	$NetBSD: objects.inc,v 1.4 2003/07/24 14:16:32 itojun Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/objects/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/objects

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/objects

SRCS+=	o_names.c obj_dat.c obj_lib.c obj_err.c
