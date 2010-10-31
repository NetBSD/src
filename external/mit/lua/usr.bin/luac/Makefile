# $NetBSD: Makefile,v 1.1 2010/10/31 11:26:02 mbalmer Exp $

PROG=		luac
SRCS=		luac.c print.c
BINDIR=		/usr/bin

DPADD+=		${LIBLUA}
LDADD+=		${LIBLUA} -lm

.include <bsd.own.mk>

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src \
		${NETBSDSRCDIR}/external/mit/lua/dist/doc

.include <bsd.prog.mk>
