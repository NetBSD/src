#	$NetBSD: Makefile.inc,v 1.3 2002/07/10 04:29:08 thorpej Exp $

KMINCLUDES=	arch/sh3/SYS.h
CPPFLAGS+=      -DSOFTFLOAT

.include <softfloat/Makefile.inc>

#KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
#	htonl.S htons.S ntohl.S ntohs.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S
