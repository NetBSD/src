# $NetBSD: Makefile,v 1.2 2011/01/19 11:02:00 plunky Exp $

PROG=		luac
SRCS=		luac.c print.c
BINDIR=		/usr/bin

DPADD+=		${LIBLUA} ${LIBM}
LDADD+=		${LIBLUA} -lm

.include <bsd.own.mk>

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src \
		${NETBSDSRCDIR}/external/mit/lua/dist/doc

.include <bsd.prog.mk>
