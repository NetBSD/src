# $NetBSD: Makefile,v 1.2 2001/12/12 00:05:12 tv Exp $
# Build an ed without crypt support

SRCDIR=		${.CURDIR}/../../../bin/ed

PROG=		ed
NOMAN=		# defined

SRCS=		buf.c cbc.c glbl.c io.c main.c re.c sub.c undo.c
CPPFLAGS+=	-DBACKWARDS

.include <bsd.prog.mk>

.PATH:		${SRCDIR}
