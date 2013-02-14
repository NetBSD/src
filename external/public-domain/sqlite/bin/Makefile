# $NetBSD: Makefile,v 1.4 2013/02/14 17:12:23 martin Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO}
LDADD+=		-lsqlite3 -ledit -lterminfo -lm

BINDIR=		/usr/bin

.include <bsd.prog.mk>
