#	$NetBSD: Makefile,v 1.4.2.1 1997/11/04 21:42:34 thorpej Exp $

.PATH:	${.CURDIR}/../lint1

PROG=	lint2
SRCS=	main2.c hash.c read.c mem.c mem2.c chk.c msg.c emit.c emit2.c
NOMAN=
CPPFLAGS+=-I${.CURDIR}/../lint1
LINTFLAGS=-abehrz

BINDIR=	/usr/libexec

.include <bsd.prog.mk>
