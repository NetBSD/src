#	$NetBSD: objects.inc,v 1.3 2000/07/16 07:16:23 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/objects/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/objects

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/objects

SRCS+= o_names.c obj_dat.c obj_lib.c obj_err.c


