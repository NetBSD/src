#	$NetBSD: Makefile,v 1.9 2002/04/23 20:21:03 thorpej Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	sed
SRCS=	compile.c main.c misc.c process.c

.if (${MACHINE_CPU} == "sh3")
COPTS+= -O0
.endif

.include <bsd.prog.mk>
