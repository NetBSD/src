#	$NetBSD: Makefile,v 1.11 2003/10/27 13:27:41 uwe Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	sed
SRCS=	compile.c main.c misc.c process.c
WARNS=	2

.if ${MACHINE_CPU} == "sh3" && ${HAVE_GCC3} != "yes"
COPTS+= -O0
.endif

.include <bsd.prog.mk>
