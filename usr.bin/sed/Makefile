#	$NetBSD: Makefile,v 1.12 2006/04/07 19:38:59 mrg Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	sed
SRCS=	compile.c main.c misc.c process.c
WARNS=	2

.if ${MACHINE_CPU} == "sh3" && ${HAVE_GCC} == "2"
COPTS+= -O0
.endif

.include <bsd.prog.mk>
