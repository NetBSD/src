#	$NetBSD: Makefile.inc,v 1.5 2005/09/15 21:39:15 uwe Exp $

KMINCLUDES=	arch/sh3/SYS.h
CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS

.include <softfloat/Makefile.inc>

#KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
#	htonl.S htons.S ntohl.S ntohs.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp2.S
