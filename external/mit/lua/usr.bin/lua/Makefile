# $NetBSD: Makefile,v 1.1 2010/10/31 11:26:02 mbalmer Exp $

PROG=		lua
SRCS=		lua.c
BINDIR=		/usr/bin

CPPFLAGS+=	-DLUA_USE_READLINE

LDADD+=		${LIBLUA} -lm -ledit
DPADD+=		${LIBLUA}

.include <bsd.own.mk>

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src \
		${NETBSDSRCDIR}/external/mit/lua/dist/doc

.include <bsd.prog.mk>
