#	$NetBSD: Makefile,v 1.10 2002/06/14 00:37:29 wiz Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	sed
SRCS=	compile.c main.c misc.c process.c
WARNS=	2

.if (${MACHINE_CPU} == "sh3")
COPTS+= -O0
.endif

.include <bsd.prog.mk>
