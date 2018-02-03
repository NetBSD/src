#	$NetBSD: objects.inc,v 1.1.1.1 2018/02/03 22:43:37 christos Exp $
#
#	@(#) Copyright (c) 1995 Simon J. Gerraty
#
#	SRCS extracted from src/crypto/dist/openssl/crypto/objects/Makefile
#

.PATH:	${OPENSSLSRC}/crypto/objects


OBJECTS_SRCS = o_names.c obj_dat.c obj_lib.c obj_err.c obj_xref.c
SRCS += ${OBJECTS_SRCS}

.for cryptosrc in ${OBJECTS_SRCS}
CPPFLAGS.${cryptosrc} = -I${OPENSSLSRC}/crypto/objects
.endfor
