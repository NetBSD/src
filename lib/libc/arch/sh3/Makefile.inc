#	$NetBSD: Makefile.inc,v 1.2 2000/06/06 17:28:37 tsubai Exp $

KMINCLUDES=	arch/sh3/SYS.h
CPPFLAGS+=      -DSOFTFLOAT

.include <softfloat/Makefile.inc>

#KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
#	htonl.S htons.S ntohl.S ntohs.S
