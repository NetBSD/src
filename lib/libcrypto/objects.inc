#	$NetBSD: objects.inc,v 1.1.1.1.2.2 2000/07/17 06:00:00 mrg Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/objects/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/objects

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/objects

SRCS+= o_names.c obj_dat.c obj_lib.c obj_err.c


