#	$NetBSD: objects.inc,v 1.2 2000/06/21 19:34:25 thorpej Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from ../../dist/openssl/crypto/objects/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/objects

CPPFLAGS+=	-I${OPENSSLSRC}/crypto/objects

SRCS+= o_names.c obj_dat.c obj_lib.c obj_err.c


