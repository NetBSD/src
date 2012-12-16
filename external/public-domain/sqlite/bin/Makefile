# $NetBSD: Makefile,v 1.3 2012/12/16 20:31:07 christos Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO}
LDADD+=		-lsqlite3 -ledit -lterminfo

BINDIR=		/usr/bin

.include <bsd.prog.mk>
