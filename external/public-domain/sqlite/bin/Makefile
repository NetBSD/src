# $NetBSD: Makefile,v 1.6 2020/10/30 08:41:58 martin Exp $

PROG=		sqlite3

SRCS=		shell.c

DPADD+=		${LIBSQLITE3} ${LIBEDIT} ${LIBTERIMINFO} ${LIBM}
LDADD+=		-lsqlite3 -ledit -lterminfo -lm

BINDIR=		/usr/bin

.include <bsd.prog.mk>
