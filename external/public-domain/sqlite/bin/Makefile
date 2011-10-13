# $NetBSD: Makefile,v 1.1 2011/10/13 21:40:27 joerg Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT}
LDADD+=		-lsqlite3 -ledit

NOMAN=

BINDIR=		/usr/bin

.include <bsd.prog.mk>
