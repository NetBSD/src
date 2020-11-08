# $NetBSD: Makefile,v 1.7 2020/11/08 21:56:48 nia Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO} ${LIBM} ${LIBPTHREAD}
LDADD+=		-lsqlite3 -ledit -lterminfo -lpthread -lm

BINDIR=		/usr/bin

.include <bsd.prog.mk>
