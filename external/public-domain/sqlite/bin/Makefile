# $NetBSD: Makefile,v 1.8 2020/11/12 17:53:43 nia Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO} ${LIBM}
LDADD+=		-lsqlite3 -ledit -lterminfo -lm

BINDIR=		/usr/bin

.include <bsd.prog.mk>
