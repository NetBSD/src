#	$NetBSD: objects.inc,v 1.5.10.1 2008/01/09 01:34:37 matt Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/objects/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/objects


OBJECTS_SRCS = o_names.c obj_dat.c obj_lib.c obj_err.c
SRCS += ${OBJECTS_SRCS}

.for cryptosrc in ${OBJECTS_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/objects
.endfor
