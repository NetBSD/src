# $NetBSD: Makefile,v 1.14 2001/12/12 00:05:11 tv Exp $
# Build a smaller dd (i.e. for boot media)

SRCDIR=		${.CURDIR}/../../../bin/dd

PROG=		dd
NOMAN=		# defined

SRCS=		args.c conv.c dd.c misc.c position.c strsuftoull.c
CPPFLAGS+=	-DNO_CONV -I${SRCDIR}

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
