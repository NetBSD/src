#	$NetBSD: Makefile.inc,v 1.1 2000/01/05 14:07:29 msaitoh Exp $

KMINCLUDES=	arch/sh3/SYS.h
CPPFLAGS+=      -DSOFTFLOAT

.include <fplib/Makefile.inc>
#KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
#	htonl.S htons.S ntohl.S ntohs.S
