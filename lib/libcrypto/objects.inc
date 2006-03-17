#	$NetBSD: objects.inc,v 1.5 2006/03/17 20:47:45 dsl Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from /home/current/src/lib/libcrypto/../../crypto/dist/openssl/crypto/objects/Makefile.ssl
#

.PATH:	${OPENSSLSRC}/crypto/objects


OBJECTS_SRCS = o_names.c obj_dat.c obj_lib.c obj_err.c
SRCS += ${OBJECTS_SRCS}

.for cryptosrc in ${OBJECTS_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/objects
.endfor
