# $NetBSD: Makefile,v 1.2.8.1 2013/02/25 00:27:42 tls Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO}
LDADD+=		-lsqlite3 -ledit -lterminfo

BINDIR=		/usr/bin

.include <bsd.prog.mk>
