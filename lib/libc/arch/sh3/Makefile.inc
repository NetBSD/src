#	$NetBSD: Makefile.inc,v 1.4 2003/11/23 23:13:11 uwe Exp $

KMINCLUDES=	arch/sh3/SYS.h
CPPFLAGS+=      -DSOFTFLOAT # -DSOFTFLOAT_NEED_FIXUNS

.include <softfloat/Makefile.inc>

#KMSRCS=	bcmp.S bzero.S ffs.S strcat.S strcmp.S strcpy.S strlen.S \
#	htonl.S htons.S ntohl.S ntohs.S

SRCS+=	__sigaction14_sigtramp.c __sigtramp1.S __sigtramp2.S
