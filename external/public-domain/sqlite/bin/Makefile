# $NetBSD: Makefile,v 1.2.2.1 2013/01/23 00:05:19 yamt Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO}
LDADD+=		-lsqlite3 -ledit -lterminfo

BINDIR=		/usr/bin

.include <bsd.prog.mk>
