#	$NetBSD: Makefile,v 1.4 2009/04/14 22:15:26 lukem Exp $

NOMAN=	1

PROG=	spellprog
SRCS=	spellprog.c look.c
BINDIR=	/usr/libexec

LDADD+=-lutil
DPADD+=${LIBUTIL}

.include <bsd.prog.mk>
