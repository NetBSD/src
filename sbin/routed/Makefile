#	$NetBSD: Makefile,v 1.18 1997/10/11 09:34:36 mycroft Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/19/93

PROG=	routed
SRCS=	if.c input.c main.c output.c parms.c radix.c rdisc.c table.c trace.c
MAN=	routed.8
SUBDIR=	rtquery

# By popular demand...
MLINKS=	routed.8 rdisc.8
#COPTS=-g -DDEBUG -Wall

.include <bsd.prog.mk>
.include <bsd.subdir.mk>
