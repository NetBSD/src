#	$NetBSD: Makefile,v 1.20 1999/02/23 10:47:39 christos Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/19/93

WARNS=1
PROG=	routed
SRCS=	if.c input.c main.c output.c parms.c radix.c rdisc.c table.c trace.c
MAN=	routed.8
SUBDIR=	rtquery

# By popular demand...
MLINKS=	routed.8 rdisc.8
#COPTS+=-g -DDEBUG -Wall

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
