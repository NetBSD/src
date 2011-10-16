# $NetBSD: Makefile,v 1.2 2011/10/16 21:31:15 he Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO}
LDADD+=		-lsqlite3 -ledit -lterminfo

NOMAN=

BINDIR=		/usr/bin

.include <bsd.prog.mk>
